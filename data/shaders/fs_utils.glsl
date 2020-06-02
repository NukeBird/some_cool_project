#extension GL_ARB_shading_language_include : enable
#include "/common.glsl"

//screen coord is in range -1..1
vec3 getEyeDir(in vec2 screenCoord)
{
    vec4 device_normal = vec4(screenCoord, 0.0, 1.0);
    vec3 eye_normal = normalize((u_matInverseProj * device_normal).xyz);
    vec3 world_normal = normalize(mat3(u_matInverseModelView) * eye_normal);
    
    return world_normal;
}

//screen coord is in range -1..1
vec3 getFragPos(in vec3 screenCoord)
{
    vec4 pos = u_matInverseProj * vec4(screenCoord, 1.0);
    return pos.xyz/pos.w;
}

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
