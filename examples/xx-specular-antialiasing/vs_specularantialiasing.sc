$input a_position, a_normal, a_tangent
$output v_pos, t_view, v_normal, v_tangent, v_bitangent, w_light
/*
 * Copyright 2011-2020 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

mat3 mtx3FromCols(vec3 c0, vec3 c1, vec3 c2)
{
	#ifdef BGFX_SHADER_LANGUAGE_GLSL
		return mat3(c0, c1, c2);
	#else
		return transpose(mat3(c0, c1, c2));
	#endif
}

uniform vec4 u_time;
uniform vec4 u_camPos;
uniform vec4 u_lightPos;
uniform vec4 u_lightAnimation;

void main()
{


	// Transformations from model coordinates to world coordinates
	v_pos = mul(u_model[0], vec4(a_position.xyz, 1.0) ).xyz;
	v_normal = normalize(mul(u_model[0], vec4(a_normal.xyz, 0.0)).xyz);
	v_tangent = normalize(mul(u_model[0], vec4(a_tangent.xyz, 0.0)).xyz);
	
	// Calculation of tangent according to Gram-Schmidt process
	float ndott = dot(v_normal, v_tangent);
	v_tangent = normalize(v_tangent - ndott * v_normal);

	// Calculation of bitangent
	v_bitangent = normalize(cross(v_tangent, v_normal));

	// Creation of TBN transformation matrix
	mat3 tbn = mtx3FromCols(v_tangent, v_bitangent, v_normal);
	
	// View dir in world space
	vec3 v_view = u_camPos.xyz - v_pos;
	
	// View dir in tangent space
	t_view = normalize(mul(tbn, v_view));

	w_light = u_lightPos.xyz;
	if(u_lightAnimation[0])
	{
		w_light.x = sin(u_time * 4.0);
	}

	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
}
