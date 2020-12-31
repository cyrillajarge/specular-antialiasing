/*
 * Copyright 2011-2020 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include <vector>
#include <string>
#include <iostream>

#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"


#include <bx/readerwriter.h>
#include <bx/string.h>

namespace
{

struct Light
{
	Light()
	{
		m_pos[0] = 1.0f;
		m_pos[1] = 1.0f;
		m_pos[2] = -1.0f;
		m_power = 1.0f;
	}

	Light(float pos[3], float power)
	{
		m_pos[0] = pos[0];
		m_pos[1] = pos[1];
		m_pos[2] = pos[2];
		m_power = power;
	}


	float m_pos[3];
	float m_power;

};

struct Camera
{
	Camera()
	{
		reset();
	}

	void reset()
	{
		m_target.curr = { 0.0f, 0.0f, 0.0f };
		m_target.dest = { 0.0f, 0.0f, 0.0f };

		m_pos.curr = { 0.0f, 0.0f, -3.0f };
		m_pos.dest = { 0.0f, 0.0f, -3.0f };

		m_orbit[0] = 0.0f;
		m_orbit[1] = 0.0f;
	}

	void mtxLookAt(float* _outViewMtx)
	{
		bx::mtxLookAt(_outViewMtx, m_pos.curr, m_target.curr);
	}

	void orbit(float _dx, float _dy)
	{
		m_orbit[0] += _dx;
		m_orbit[1] += _dy;
	}

	void dolly(float _dz)
	{
		const float cnear = 1.0f;
		const float cfar = 100.0f;

		const bx::Vec3 toTarget = bx::sub(m_target.dest, m_pos.dest);
		const float toTargetLen = bx::length(toTarget);
		const float invToTargetLen = 1.0f / (toTargetLen + bx::kFloatMin);
		const bx::Vec3 toTargetNorm = bx::mul(toTarget, invToTargetLen);

		float delta = toTargetLen * _dz;
		float newLen = toTargetLen + delta;
		if ((cnear < newLen || _dz < 0.0f)
			&& (newLen < cfar || _dz > 0.0f))
		{
			m_pos.dest = bx::mad(toTargetNorm, delta, m_pos.dest);
		}
	}

	void consumeOrbit(float _amount)
	{
		float consume[2];
		consume[0] = m_orbit[0] * _amount;
		consume[1] = m_orbit[1] * _amount;
		m_orbit[0] -= consume[0];
		m_orbit[1] -= consume[1];

		const bx::Vec3 toPos = bx::sub(m_pos.curr, m_target.curr);
		const float toPosLen = bx::length(toPos);
		const float invToPosLen = 1.0f / (toPosLen + bx::kFloatMin);
		const bx::Vec3 toPosNorm = bx::mul(toPos, invToPosLen);

		float ll[2];
		bx::toLatLong(&ll[0], &ll[1], toPosNorm);
		ll[0] += consume[0];
		ll[1] -= consume[1];
		ll[1] = bx::clamp(ll[1], 0.02f, 0.98f);

		const bx::Vec3 tmp = bx::fromLatLong(ll[0], ll[1]);
		const bx::Vec3 diff = bx::mul(bx::sub(tmp, toPosNorm), toPosLen);

		m_pos.curr = bx::add(m_pos.curr, diff);
		m_pos.dest = bx::add(m_pos.dest, diff);
	}

	void update(float _dt)
	{
		const float amount = bx::min(_dt / 0.12f, 1.0f);

		consumeOrbit(amount);

		m_target.curr = bx::lerp(m_target.curr, m_target.dest, amount);
		m_pos.curr = bx::lerp(m_pos.curr, m_pos.dest, amount);
	}

	struct Interp3f
	{
		bx::Vec3 curr;
		bx::Vec3 dest;
	};

	Interp3f m_target;
	Interp3f m_pos;
	float m_orbit[2];
};

struct Mouse
{
	Mouse()
		: m_dx(0.0f)
		, m_dy(0.0f)
		, m_prevMx(0.0f)
		, m_prevMy(0.0f)
		, m_scroll(0)
		, m_scrollPrev(0)
	{
	}

	void update(float _mx, float _my, int32_t _mz, uint32_t _width, uint32_t _height)
	{
		const float widthf = float(int32_t(_width));
		const float heightf = float(int32_t(_height));

		// Delta movement.
		m_dx = float(_mx - m_prevMx) / widthf;
		m_dy = float(_my - m_prevMy) / heightf;

		m_prevMx = _mx;
		m_prevMy = _my;

		// Scroll.
		m_scroll = _mz - m_scrollPrev;
		m_scrollPrev = _mz;
	}

	float m_dx; // Screen space.
	float m_dy;
	float m_prevMx;
	float m_prevMy;
	int32_t m_scroll;
	int32_t m_scrollPrev;
};

class ExampleSpecularAntialiasing : public entry::AppI
{
public:
	ExampleSpecularAntialiasing(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		// Init
		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_NONE;
		m_reset  = 0
			| BGFX_RESET_VSYNC
			| BGFX_RESET_MSAA_X16
			;

		bgfx::Init init;
		init.type     = args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
				);

		// Uniform variable creation for time value for shader
		u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
		u_camPos = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4);
		u_anisotropic = bgfx::createUniform("u_anisotropic", bgfx::UniformType::Vec4);
		u_antialiasing = bgfx::createUniform("u_antialiasing", bgfx::UniformType::Vec4);
		u_roughness = bgfx::createUniform("u_roughness", bgfx::UniformType::Vec4);
		u_roughness_aniso = bgfx::createUniform("u_roughness_aniso", bgfx::UniformType::Vec4);
		u_reflectance = bgfx::createUniform("u_reflectance", bgfx::UniformType::Vec4);
		u_metallic = bgfx::createUniform("u_metallic", bgfx::UniformType::Vec4);
		u_ndf = bgfx::createUniform("u_ndf", bgfx::UniformType::Vec4);
		u_baseCol = bgfx::createUniform("u_baseCol", bgfx::UniformType::Vec4);
		u_lightAnimation = bgfx::createUniform("u_lightAnimation", bgfx::UniformType::Vec4);
		u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);
		u_lightPower = bgfx::createUniform("u_lightPower", bgfx::UniformType::Vec4);


		// Create program from shaders.
		m_program_mesh = loadProgram("vs_specularantialiasing", "fs_specularantialiasing");
		m_program_light = loadProgram("vs_light", "fs_light");

		// Loads mesh
		m_mesh_bunny = meshLoad("meshes/bunny.bin");
		m_mesh_sphere = meshLoad("meshes/sphere.bin");
		m_mesh_orb = meshLoad("meshes/orb.bin");
		m_mesh_ogre = meshLoad("meshes/ogre.bin");
		m_mesh_light = meshLoad("meshes/sphere.bin");

		m_timeOffset = bx::getHPCounter();

		imguiCreate();

		float lightPos[3] = { 0.0f, 0.0f,-2.0f };
		m_light1 = Light(lightPos, 8.0f);

		// Uniforms init.
		m_ndf = true;
		m_anisotropic = false;
		m_antialiasing = false;
		m_roughness = 0.5f;
		m_roughness_aniso[0] = 0.5f;
		m_roughness_aniso[1] = 0.5f;
		m_reflectance = 0.5f;
		m_metallic = false;
		m_lightAnimation = false;
		m_baseCol[0] = 1.0f;
		m_baseCol[1] = 0.0f;
		m_baseCol[2] = 0.0f;
		//m_lightPos[0] = 0.0f;
		//m_lightPos[1] = 0.0f;
		//m_lightPos[2] = -2.0f;
		//m_lightPower = 8.0f;

	}

	int shutdown() override
	{
		imguiDestroy();

		meshUnload(m_mesh_bunny);
		meshUnload(m_mesh_sphere);
		meshUnload(m_mesh_orb);
		meshUnload(m_mesh_ogre);
		meshUnload(m_mesh_light);
		//m_light1.destroy();

		// Cleanup.
		bgfx::destroy(m_program_mesh);
		bgfx::destroy(m_program_light);

		bgfx::destroy(u_time);
		bgfx::destroy(u_camPos);
		bgfx::destroy(u_anisotropic);
		bgfx::destroy(u_antialiasing);
		bgfx::destroy(u_roughness);
		bgfx::destroy(u_roughness_aniso);
		bgfx::destroy(u_reflectance);
		bgfx::destroy(u_metallic);
		bgfx::destroy(u_ndf);
		bgfx::destroy(u_baseCol);
		bgfx::destroy(u_lightAnimation);
		bgfx::destroy(u_lightPos);
		bgfx::destroy(u_lightPower);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);

			ImGui::SetNextWindowPos(
				ImVec2(m_width - m_width / 3.5f - 10.0f, 10.0f)
				, ImGuiCond_FirstUseEver
			);
			ImGui::SetNextWindowSize(
				ImVec2(m_width / 3.5f, m_height / 1.5f)
				, ImGuiCond_FirstUseEver
			);

			ImGui::Begin("Settings"
				, NULL
				, 0
			);
			ImGui::PushItemWidth(180.0f);
			ImGui::Indent();
			ImGui::Checkbox("NDF (Checked=GGX, Unchecked=Beckmann)", &m_ndf);
			ImGui::Checkbox("Anisotropic", &m_anisotropic);
			ImGui::Checkbox("Specular Antialaising", &m_antialiasing);
			ImGui::Indent();
			if (!m_anisotropic) {
				ImGui::SliderFloat("Roughness", &m_roughness, 0.01f, 0.9f);
			}
			else {
				ImGui::SliderFloat("Roughness X", &m_roughness_aniso[0], 0.01f, 0.9f);
				ImGui::SliderFloat("Roughness Y", &m_roughness_aniso[1], 0.01f, 0.9f);
			}
			ImGui::Unindent();
			ImGui::SliderFloat("Reflectance", &m_reflectance, 0.3f, 1.0f);
			ImGui::Checkbox("Metallic", &m_metallic);
			ImGui::Unindent();

			ImGui::Separator();

			ImGui::Text("Light settings:");
			ImGui::Indent();
			ImGui::SliderFloat("Light power", &m_light1.m_power, 0.0f, 100.0f);
			ImGui::ColorWheel("Color:", m_baseCol, 0.6f);

			ImGui::Checkbox("Animate Light", &m_lightAnimation);


			ImGui::SliderFloat("Light position X", &m_light1.m_pos[0], -5.0f, 5.0f);
			ImGui::SliderFloat("Light position Y", &m_light1.m_pos[1], -5.0f, 5.0f);
			ImGui::SliderFloat("Light position Z", &m_light1.m_pos[2], -5.0f, 5.0f);

			ImGui::End();

			ImGui::SetNextWindowPos(
				ImVec2(10.0f, 300.0f)
				, ImGuiCond_FirstUseEver
			);
			ImGui::SetNextWindowSize(
				ImVec2(m_width / 4.0f, 100.0f)
				, ImGuiCond_FirstUseEver
			);
			ImGui::Begin("Mesh"
				, NULL
				, 0
			);

			static const char* meshes[]{"Bunny", "Orb", "Sphere", "Ogre"};
			static int selectedMesh = 2;

			ImGui::Combo("Mesh", &selectedMesh, meshes, IM_ARRAYSIZE(meshes));
			ImGui::End();

			imguiEndFrame();

			bgfx::setUniform(u_roughness, &m_roughness);
			bgfx::setUniform(u_roughness_aniso, &m_roughness_aniso);
			bgfx::setUniform(u_reflectance, &m_reflectance);
			float value_metallic, value_lightAnimation, value_ndf, value_anisotropic, value_antialiasing;
			value_metallic = float(m_metallic);
			value_lightAnimation = float(m_lightAnimation);
			value_ndf = float(m_ndf);
			value_anisotropic = float(m_anisotropic);
			value_antialiasing = float(m_antialiasing);
			bgfx::setUniform(u_antialiasing, &value_antialiasing);
			bgfx::setUniform(u_metallic, &value_metallic);
			bgfx::setUniform(u_ndf, &value_ndf);
			bgfx::setUniform(u_anisotropic, &value_anisotropic);
			bgfx::setUniform(u_baseCol, &m_baseCol);
			bgfx::setUniform(u_lightPos, &m_light1.m_pos);
			bgfx::setUniform(u_lightPower, &m_light1.m_power);
			bgfx::setUniform(u_lightAnimation, &value_lightAnimation);

			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency());
			const float deltaTimeSec = float(double(frameTime) / freq);

			// Camera.
			const bool mouseOverGui = ImGui::MouseOverArea();
			m_mouse.update(float(m_mouseState.m_mx), float(m_mouseState.m_my), m_mouseState.m_mz, m_width, m_height);
			if (!mouseOverGui)
			{
				if (m_mouseState.m_buttons[entry::MouseButton::Left])
				{
					m_camera.orbit(m_mouse.m_dx, m_mouse.m_dy);
				}
				else if (m_mouseState.m_buttons[entry::MouseButton::Right])
				{
					m_camera.dolly(m_mouse.m_dx + m_mouse.m_dy);
				}
				else if (0 != m_mouse.m_scroll)
				{
					m_camera.dolly(float(m_mouse.m_scroll) * 0.05f);
				}
			}
			m_camera.update(deltaTimeSec);

			// Ces deux lignes ne servent pas.
			/*float m_cameraPos[4];
			bx::memCopy(m_cameraPos, &m_camera.m_pos.curr.x, 3 * sizeof(float));*/
			// Il manquait la mise à jour de la position de la caméra du côté GPU
			bgfx::setUniform(u_camPos, &m_camera.m_pos.curr);

			float view[16];
			bx::mtxIdentity(view);

			const bgfx::Caps* caps = bgfx::getCaps();

			// Set view and projection matrix for view 0.
			float proj[16];
			m_camera.mtxLookAt(view);
			bx::mtxProj(proj, 45.0f, float(m_width) / float(m_height), 0.1f, 100.0f, caps->homogeneousDepth);
			bgfx::setViewTransform(0, view, proj);

			// Set view 0 default viewport.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			//bgfx::touch(0);

			// Set uniform time variable
			float time = (float)( (bx::getHPCounter()-m_timeOffset)/double(bx::getHPFrequency() ) );
			bgfx::setUniform(u_time, &time);
			float mtx[16];
			switch (selectedMesh) {
				case 0:
					bx::mtxSRT(mtx, 1.0f, 1.0f, 1.0f, 0.0f, bx::kPi, 0.0f, 0.0f, -0.8f, 0.0f);
					meshSubmit(m_mesh_bunny, 0, m_program_mesh, mtx);
					break;
				case 1:
					bx::mtxSRT(mtx, 1.0f, 1.0f, 1.0f, 0.0f ,0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
					meshSubmit(m_mesh_orb, 0, m_program_mesh, mtx);
					break;
				case 2:
					bx::mtxSRT(mtx, 5.0f, 5.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
					meshSubmit(m_mesh_sphere, 0, m_program_mesh, mtx);
					break;
				case 3:
					bx::mtxSRT(mtx, 1.0f, 1.0f, 1.0f, 0.0f, bx::kPi, 0.0f, 0.0f, 0.0f, 0.0f);
					meshSubmit(m_mesh_ogre, 0, m_program_mesh, mtx);
					break;
				default:
					bx::mtxSRT(mtx, 5.0f, 5.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
					meshSubmit(m_mesh_sphere, 0, m_program_mesh, mtx);
			}

			bx::mtxSRT(mtx, .5f, .5f, .5f, 0.0f, 0.0f, 0.0f, m_light1.m_pos[0], m_light1.m_pos[1], m_light1.m_pos[2]);
			meshSubmit(m_mesh_light, 0, m_program_light, mtx);

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	entry::MouseState m_mouseState;

	int64_t m_timeOffset;
	Mesh* m_mesh_bunny;
	Mesh* m_mesh_sphere;
	Mesh* m_mesh_orb;
	Mesh* m_mesh_ogre;
	Mesh* m_mesh_light;

	bgfx::ProgramHandle m_program_mesh;
	bgfx::ProgramHandle m_program_light;

	bgfx::UniformHandle u_time;
	bgfx::UniformHandle u_camPos;

	bgfx::UniformHandle u_anisotropic;
	bgfx::UniformHandle u_antialiasing;
	bgfx::UniformHandle u_roughness;
	bgfx::UniformHandle u_roughness_aniso;
	bgfx::UniformHandle u_reflectance;
	bgfx::UniformHandle u_metallic;
	bgfx::UniformHandle u_ndf;

	bgfx::UniformHandle u_baseCol;

	bgfx::UniformHandle u_lightAnimation;

	bgfx::UniformHandle u_lightPos;
	bgfx::UniformHandle u_lightPower;

	bool m_anisotropic;
	bool m_antialiasing;
	float m_roughness;
	float m_roughnessX;
	float m_roughness_aniso[2];
	float m_reflectance;
	bool m_metallic;
	bool m_ndf;
	bool m_lightAnimation;
	float m_baseCol[3];

	Light m_light1;
	Camera m_camera;
	Mouse m_mouse;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	  ExampleSpecularAntialiasing
	, "xx-specular-antialiasing"
	, "Microfacet-based BRDF with specular antialiasing"
	, "/"
	);
