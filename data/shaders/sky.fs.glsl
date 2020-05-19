#version 420 core

// layout (binding = 1) uniform CameraBlock
// {
// 	mat4 u_matView, u_matInverseView;
//     mat4 u_matProj, u_matInverseProj;
//     mat4 u_matModelView, u_matInverseModelView;

// 	mat3 u_matNormal;
// };

uniform mat4 u_matProj, u_matInverseProj;
uniform mat4 u_matModelView, u_matInverseModelView;
uniform mat3 u_matNormal;

uniform samplerCube source;

layout (location = 0) out vec4 fragColor;
in vec2 v_texCoord;

vec3 getEyeDir()
{
    vec4 device_normal = vec4(v_texCoord*2.0-1.0, 0.0, 1.0);
    vec3 eye_normal = normalize((u_matInverseProj * device_normal).xyz);
    vec3 world_normal = normalize(mat3(u_matInverseModelView) * eye_normal);
    
    return world_normal;
}

void main()
{
    fragColor = texture(source, getEyeDir()) * 5.0f;
}