#version 420 core
#extension GL_ARB_shading_language_include : enable

#include "/fs_utils.glsl"

uniform sampler2D u_texAlbedo;
uniform sampler2D u_texNormalMap;
uniform sampler2D u_texAoRoughnessMetallic;


in fsInput {
	vec2 texCoord;
	vec3 normal;
	vec3 position; //in view-space
} interpolated;

layout (location = 0) out vec4 FragColor;
//layout (location = 1) out vec4 FragNormal;


//SRGB -> RGB
vec3 to_linear(in vec3 color)
{
    const float gamma = 2.2f;
    return pow(color, vec3(gamma));
}

void main()
{
	//vec3 normalFromMap = (texture(u_texNormalMap, interpolated.texCoord).rbg * 2.0 - 1.0)*vec3(1.0, 1.0, -1.0);

	vec2 texCoord = interpolated.texCoord;

    vec4 albedo = texture(u_texAlbedo, texCoord);
    albedo.rgb = to_linear(albedo.rgb);

    vec4 aorm_sample = texture(u_texAoRoughnessMetallic, texCoord);

    float ao = aorm_sample.r;
    float roughness = aorm_sample.g;
    float metallic = aorm_sample.b;

    vec4 final_color = albedo;
    final_color.rgb *= ao;

	FragColor.rgba = final_color;

	vec3 normal = u_matNormal * interpolated.normal;
	//FragNormal = vec4(normal, 1.0);
}