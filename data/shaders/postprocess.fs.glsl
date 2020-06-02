#version 420 core
#extension GL_ARB_shading_language_include : enable

#include "/fs_utils.glsl"

precision mediump float;

in vec2 v_texCoord;

uniform float u_phase;
uniform sampler3D u_texNoise;
uniform sampler2DMS u_colorMap;
uniform sampler2DMS u_depthMap;

uniform float u_tmGamma = 2.2;
uniform float u_tmExposure = 0.5;

layout (location = 0) out vec4 FragColor;

vec4 toneMapping(vec4 color)
{
	vec3 mapped_color = vec3(1.0) - exp(-color.rgb * u_tmExposure);
	mapped_color = pow(mapped_color, vec3(1.0/u_tmGamma));

	return vec4(mapped_color, color.a); 
}

vec4 textureMultisample(sampler2DMS sampler, ivec2 coord)
{
	int texSamples = 8;
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);

    for (int i = 0; i < texSamples; i++)
        color += texelFetch(sampler, coord, i);

    color /= float(texSamples);

    return color;
}

void main()
{
	vec4 color = textureMultisample(u_colorMap, ivec2(gl_FragCoord.xy));

	// if (gl_FragCoord.y < 50)
	// {
	// 	color = vec4(v_texCoord.sss, 1.0);
	// }
	// else if (gl_FragCoord.y < 100)
	// {
	// 	FragColor = vec4(v_texCoord.sss, 1.0);
	// 	return;
	// }

	FragColor = toneMapping(color);

/*
	if (FragColor.r > 1.0 || FragColor.g > 1.0 || FragColor.b > 1.0)
		FragColor.rgb = vec3(1.0, 0.0, 0.0);
		*/
}
