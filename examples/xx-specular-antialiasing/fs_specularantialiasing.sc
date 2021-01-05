$input v_pos, t_view, v_normal, v_tangent, v_bitangent, w_light

/*
 * Copyright 2011-2020 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

uniform vec4 u_time;

uniform vec4 u_ndf;
uniform vec4 u_anisotropic;
uniform vec4 u_antialiasing;
uniform vec4 u_roughness;
uniform vec4 u_roughness_aniso;
uniform vec4 u_reflectance;
uniform vec4 u_metallic;

uniform vec4 u_baseCol;

uniform vec4 u_lightPower;
uniform vec4 u_lightPos;

CONST(float PI = 3.14159265359);

float cos_theta(vec3 w)
{
	return w.z;
}

float cos_theta2(vec3 w)
{
	return cos_theta(w) * cos_theta(w);
}

float cos_theta4(vec3 w)
{
	return cos_theta2(w) * cos_theta2(w);
}

float sin_theta2(vec3 w)
{
	return (1.0 - cos_theta2(w));
}

float sin_theta(vec3 w)
{
	return sqrt(sin_theta2(w));
}

float tan_theta(vec3 w)
{
	return (sin_theta(w) / cos_theta(w));
}

float cos_phi(vec3 w)
{
	return w.x / sin_theta(w);
}

float sin_phi(vec3 w)
{
	return w.y / sin_theta(w);
}

mat3 mtx3FromCols(vec3 c0, vec3 c1, vec3 c2)
{
	#ifdef BGFX_SHADER_LANGUAGE_GLSL
		return mat3(c0, c1, c2);
	#else
		return transpose(mat3(c0, c1, c2));
	#endif
}

mat3 orthonormal_basis(vec3 n)
{
    vec3 f,r;
    if(n.z < -0.999999)
    {
        f = vec3(0 , -1, 0);
        r = vec3(-1, 0, 0);
    }
    else
    {
        float a = 1./(1. + n.z);
        float b = -n.x*n.y*a;
        f = normalize(vec3(1. - n.x*n.x*a, b, -n.x));
        r = normalize(vec3(b, 1. - n.y*n.y*a , -n.y));
    }
    return( mat3(f,r,n) );
}

float P22_GGX(vec3 m, float roughness){
	float xm_tilde = -m.x / m.z;
	float ym_tilde = -m.y / m.z;

	float roughness2 = pow(roughness, 2.0);
	float xm_tilde2 = pow(xm_tilde, 2.0);
	float ym_tilde2 = pow(ym_tilde, 2.0);

	float denom = PI * roughness2 * pow(1.0 + (xm_tilde2 + ym_tilde2)/roughness2, 2.0); 
	return 1.0 / denom;

}

float P22_Beckmann(vec3 m, float roughness){
	float xm_tilde = -m.x / m.z;
	float ym_tilde = -m.y / m.z;

	float roughness2 = pow(roughness, 2.0);
	float xm_tilde2 = pow(xm_tilde, 2.0);
	float ym_tilde2 = pow(ym_tilde, 2.0);

	return (1.0 / (PI * roughness2)) * exp(-((xm_tilde2 + ym_tilde2)/roughness2));
}

float P22_GGX_Aniso(vec3 m, vec2 roughness)
{
	float xm_tilde = -m.x / m.z;
	float ym_tilde = -m.y / m.z;

	float roughnessX2 = pow(roughness[0], 2.0);
	float roughnessY2 = pow(roughness[1], 2.0);
	float xm_tilde2 = pow(xm_tilde, 2.0);
	float ym_tilde2 = pow(ym_tilde, 2.0);

	float denom = PI * roughness[0] * roughness[1] * pow(1.0 + xm_tilde2/roughnessX2 + ym_tilde2/roughnessY2, 2.0); 
	return 1.0 / denom;

}

float P22_Beckmann_Aniso(vec3 m, vec2 roughness)
{
	float xm_tilde = -m.x / m.z;
	float ym_tilde = -m.y / m.z;

	float roughnessX2 = pow(roughness[0], 2.0);
	float roughnessY2 = pow(roughness[1], 2.0);
	float xm_tilde2 = pow(xm_tilde, 2.0);
	float ym_tilde2 = pow(ym_tilde, 2.0);

	return (1.0 / (PI * roughness[0] * roughness[1])) * exp(-xm_tilde2/roughnessX2 - ym_tilde2/roughnessY2);
}

float LambdaGGX_Aniso(vec3 omega, vec2 alpha)
{
	float tantheta = tan_theta(omega);
	float cos2 = cos_phi(omega) * cos_phi(omega);
	float sin2 = sin_phi(omega) * sin_phi(omega);
	float nalpha = sqrt(cos2 * alpha[0] * alpha[0] + sin2 * alpha[1] * alpha[1]);
	float a = 1.0 / (nalpha * tantheta);
	return 0.5 * (-1.0 + sqrt(1 + 1/(a*a)));
}

float LambdaBeckmann_Aniso(vec3 omega, vec2 alpha)
{
	float tantheta = tan_theta(omega);
	float cos2 = cos_phi(omega) * cos_phi(omega);
	float sin2 = sin_phi(omega) * sin_phi(omega);
	float nalpha = sqrt(cos2 * alpha[0] * alpha[0] + sin2 * alpha[1] * alpha[1]);
	float a = 1.0 / (nalpha * tantheta);
	if(a > 1.6)
	{
		return (1.0 - 1.259*a + 0.396*a*a)/(3.535 + 2.181*a*a);
	}
	else{
		return 0.0;
	}
}

float LambdaGGX(vec3 omega, float alpha){
	float tantheta = tan_theta(omega);
	float a = 1.0 / (alpha * tantheta);
	return 0.5 * (-1.0 + sqrt(1 + 1/(a*a)));
}

float LambdaBeckmann(vec3 omega, float alpha)
{
	float tantheta = tan_theta(omega);
	float a = 1.0 / (alpha * tantheta);
	if(a > 1.6)
	{
		return (1.0 - 1.259*a + 0.396*a*a)/(3.535 + 2.181*a*a);
	}
	else{
		return 0.0;
	}
}

void specularAnitalisasing(vec3 h, inout vec2 roughness)
{
	vec2 hxy = h.xy;

	vec2 dhu = dFdx(hxy);
	vec2 dhv = dFdy(hxy);

	vec2 pFootprint = abs(dhu) + abs(dhv);
	
	vec2 cov = pFootprint * pFootprint;

	roughness = sqrt(cov + roughness * roughness);
}

float V_SmithGGX(vec3 l, vec3 v, float roughness) {
	float denoml = LambdaGGX(l, roughness);
	float denomv = LambdaGGX(v, roughness);
	return 1.0 / (1.0 + denomv + denoml);
}

float V_SmithBeckmann(vec3 l, vec3 v, float roughness)
{
	float denoml = LambdaBeckmann(l, roughness);
	float denomv = LambdaBeckmann(v, roughness);
	return 1.0 / (1.0 + denomv + denoml);
}

float V_SmithGGX_Aniso(vec3 l, vec3 v, vec2 roughness) {
	float denoml = LambdaGGX_Aniso(l, roughness);
	float denomv = LambdaGGX_Aniso(v, roughness);
	return 1.0 / (1.0 + denomv + denoml);
}

float V_SmithBeckmann_Aniso(vec3 l, vec3 v, vec2 roughness)
{
	float denoml = LambdaBeckmann_Aniso(l, roughness);
	float denomv = LambdaBeckmann_Aniso(v, roughness);
	return 1.0 / (1.0 + denomv + denoml);
}

vec3 F_Schlick(float ldoth, vec3 f0) {
    float f = pow(1.0 - ldoth, 5.0);
    return f0 + (1 - f0) * f;
}

float lambert(){
	return 1.0 / PI;
}


void main()
{
	mat3 tbn = mtx3FromCols(v_tangent, v_bitangent, v_normal);

	// Light dir in tangent space
	vec3 t_light = normalize(mul(tbn, w_light - v_pos));

	// Half-vector in tangent space
	vec3 h = normalize(t_view + t_light);

	// Dots
	float ldoth = clamp(dot(t_light, h), 0.0, 1.0); // between light dir and halfdir

	
	float smith, ndf;

	if(!u_anisotropic[0]){
		float roughness = u_roughness[0] * u_roughness[0];
		
		// NDF
		float p22_ggx = P22_GGX(h, roughness);
		float p22_beck = P22_Beckmann(h, roughness);
		
		ndf = ((u_ndf[0]) ? p22_ggx : p22_beck) / cos_theta4(h);

		// Smith (Masking-shadowing)
		smith = (u_ndf[0]) ? V_SmithGGX(t_light, t_view, roughness) : V_SmithBeckmann(t_light, t_view, roughness);
	}
	else{

		vec2 roughness = u_roughness_aniso * u_roughness_aniso;

		if(u_antialiasing[0])
		{
			specularAnitalisasing(h, roughness);
		}

		float p22_ggx_aniso = P22_GGX_Aniso(h, roughness);
		float p22_beck_aniso = P22_Beckmann_Aniso(h, roughness);

		ndf = ((u_ndf[0]) ? p22_ggx_aniso : p22_beck_aniso) / cos_theta4(h);

		// Smith (Masking-shadowing)
		smith = (u_ndf[0]) ? V_SmithGGX_Aniso(t_light, t_view, roughness) : V_SmithBeckmann_Aniso(t_light, t_view, roughness);	
	}

	if(t_light.z < 0. || t_view.z < 0.)
	{
		smith = 0.0f;
	}

	// Fresnel
	vec3 f0 = 0.16 * u_reflectance[0] * u_reflectance[0] * (1.0 - u_metallic[0]) + u_baseCol.xyz * u_metallic[0];
	vec3 fschlick = F_Schlick(ldoth, f0);

	// Specular
	vec3 spec = (ndf * smith * fschlick) / (4 * cos_theta(t_light) * cos_theta(t_view));
	
	// Diffuse
	vec3 diff = (1.0 - u_metallic[0]) * u_baseCol.xyz * lambert();

	// Attenuation factor
	float attenuation_factor = 1.0/pow(length(u_lightPos.xyz - v_pos), 2.0);

	// Final color
	vec3 color = cos_theta(t_light) * attenuation_factor * u_lightPower[0] * (spec + diff);

	gl_FragColor.xyz = color;
	gl_FragColor.w = 1.0;
}
