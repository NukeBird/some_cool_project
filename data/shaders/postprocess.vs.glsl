#version 420 core
#extension GL_ARB_shading_language_include : enable

#include "/common.glsl"

layout (location = 0) in vec2 a_vertex;

out vec2 v_texCoord;

void main()
{
    v_texCoord = a_vertex;
    gl_Position = vec4(a_vertex, 0.0, 1.0);
}
