#version 420 core

precision mediump float;

in vec2 v_texCoord;

uniform mat4 u_matProj, u_matInverseProj;
uniform mat4 u_matModelView, u_matInverseModelView;
uniform mat3 u_matNormal;


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

vec3 getFragPos(in float z)
{
    vec4 pos = u_matInverseProj * vec4(v_texCoord*2.0-1.0, z*2.0-1.0, 1.0);
    return pos.xyz/pos.w;
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
	FragColor = toneMapping(color);

/*
	if (FragColor.r > 1.0 || FragColor.g > 1.0 || FragColor.b > 1.0)
		FragColor.rgb = vec3(1.0, 0.0, 0.0);
		*/
}
