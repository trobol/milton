// Copyright (c) 2015 Sergio Gonzalez. All rights reserved.
// License: https://github.com/serge-rgb/milton#license

in vec2 v_sizes;

uniform int u_radius;
uniform bool u_fill;
uniform vec4 u_color;


void
main()
{
    float r = length(v_sizes);

    float girth = u_fill ? 3.0 : 1.5;
    const float ring_alpha = 0.4;

	float outer_alpha = smoothstep(u_radius-girth, u_radius, r);
	float inner_alpha = smoothstep(u_radius+girth, u_radius, r);
	if ( alpha >= 0.0 ) {
        out_color = vec4(alpha,alpha,alpha,alpha);
    }
    else if ( u_fill && r < u_radius ) {
        out_color = u_color;
    }
    else {
        //discard;
    }
}

