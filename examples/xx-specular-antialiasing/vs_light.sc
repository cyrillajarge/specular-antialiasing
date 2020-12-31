$input a_position
/*
 * Copyright 2011-2020 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

uniform vec4 u_time;
uniform vec4 u_lightAnimation;

void main()
{
	vec3 position = a_position.xyz;
	if(u_lightAnimation[0])
	{
		position.x = sin(u_time * 4.0);
	}

	gl_Position = mul(u_modelViewProj, vec4(position, 1.0) );
}
