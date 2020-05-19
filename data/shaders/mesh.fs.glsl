#version 420 core

uniform mat4 u_matProj, u_matInverseProj;
uniform mat4 u_matModelView, u_matInverseModelView;
uniform mat3 u_matNormal;

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

mat3 cotangent_frame(in vec3 normal, in vec3 pos, in vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( pos );
    vec3 dp2 = dFdy( pos );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, normal );
    vec3 dp1perp = cross( normal, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, normal );
}

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