/*
 * Copyright 2011-2020 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

uniform vec4 u_time;

void main()
{
	gl_FragColor.xyz = vec3(1.0, 1.0, 1.0);
	gl_FragColor.w = 1.0;
}
