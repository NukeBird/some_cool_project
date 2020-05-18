#version 420 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Bitangent;
layout(location = 4) in vec3 a_TexCoord;


uniform mat4 u_matProj, u_matInverseProj;
uniform mat4 u_matModelView, u_matInverseModelView;
uniform mat3 u_matNormal;


out fsInput {
	vec3 texCoord;
	vec3 normal;
	vec3 position; //in view-space
} result;

void main()
{

	vec4 viewSpacePos = u_matModelView * vec4(a_Position, 1.0);

	result.texCoord = a_TexCoord;
	result.normal = a_Normal;
	result.position = viewSpacePos.xyz;

	gl_Position = u_matProj * viewSpacePos;
}
