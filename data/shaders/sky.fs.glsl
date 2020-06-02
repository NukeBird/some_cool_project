#version 420 core
#extension GL_ARB_shading_language_include : enable

#include "/fs_utils.glsl"

uniform samplerCube source;

layout (location = 0) out vec4 fragColor;
in vec2 v_texCoord;

void main()
{
    fragColor = texture(source, getEyeDir(v_texCoord)) * 5.0f;
}