//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************
#ifndef BRDF_INCLUDED
#define BRDF_INCLUDED

#define MaxLights 16
#define kDielectricSpec half4(0.04, 0.04, 0.04, 1.0 - 0.04)
#define PI 3.1415926535f
#define RCP_PI 0.318309886f

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float Pow5(float s)
{
    float s2 = s * s;
    return s2 * s2 * s;
}
// half DistributionGGX(half a2, half NdotH)
// {
//     return RCP_PI * a2 / pow(NdotH * NdotH * (a2 - 1) + 1, 2);
// }
half SchlickGGX(half NoV, half k)
{
    return 1.0 / (NoV * (1.0 - k) + k);
}
half GeometrySmith(half NoV, half NoL, half k)
{
    half ggx1 = SchlickGGX(NoV, k);
    half ggx2 = SchlickGGX(NoL, k);
    return ggx1 * ggx2 * 0.25;
    // return 0.5f / (NoL *(NoV *(1-k) + k) + NoV * (NoL * (1-k) + k) + 1e-4f);
}
float SmithG_GGX(half NoV, half k)
{
    float a = k;
    float b = NoV * NoV;
    return 1.0f / (NoV + sqrt(a+b - a*b));
}
float SmithG_GGX_aniso(float NoV, float dotVX, float dotVY, float ax, float ay)
{
    return 1.0 / (NoV + sqrt(pow(dotVX * ax, 2.0) + pow(dotVY * ay, 2.0) + pow(NoV, 2.0)));
}
half Kdirect(half a)
{
    return (a+1.0)*(a+1.0)*0.125;
}
half Kibl(half a)
{
    return a*a*0.5;
}
half3 FresnelSchlick(half VoH, half3 F0)
{
    half s = 1.0 - VoH;
    return F0 + (1.0 - F0) * Pow5(s);
}
float Diffuse_Burley_Disney(float Roughness, float NoV, float NoL, float VoH )
{
    float FD90 = 0.5 + 2 * VoH * VoH * Roughness;
    float F0 = 1.0;
    float FdV = F0 + (FD90 - F0) * Pow5( 1 - NoV );
    float FdL = F0 + (FD90 - F0) * Pow5( 1 - NoL );
    return RCP_PI * FdV * FdL;
}
float GTR1(float NdotH, float a)
{
    if (a >= 1) return RCP_PI;
    //如果a大于等于1，就返回圆周率倒数
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return (a2-1) / (PI*log(a2)*t);
}
float GTR2(float NdotH, float a)
{
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return RCP_PI * a2 / (t * t);
}
float3 Disney_Principle_BRDF(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic)
{
    float3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0);
    float NoL = max(dot(N, L), 0);
    float VoH = max(dot(V, H), 0);
    float NoH = max(dot(N, H), 0);
    float3 diffuse = albedo * Diffuse_Burley_Disney(roughness, NoV, NoL, VoH);
    float D = GTR2(NoH, roughness);
    float3 F0 = lerp(kDielectricSpec.rgb, albedo.rgb, metallic);
    float3 F = FresnelSchlick(VoH, F0);
    half k = Kdirect(roughness);
    float G = GeometrySmith(NoV, NoL, k);
    float3 specular = D * F * G;
    half3 kd = (1 - F) * (1.0 - metallic);
    return diffuse * kd + specular;
}
float3 envirGradient(float3 dir, float lod)
{
    float a = dir.y*0.8+0.2;
    half k = abs(a);
    k = pow(1-k, 10-lod);
    return a > 0 ? 
                lerp(half3(0.5,0.6,0.7), half3(0.5, 0.55, 0.6), k)
                : lerp(half3(0.015, 0.014, 0.01), half3(0.5, 0.55, 0.6), k*k);
}
float3 ImageBasedLight(float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
    float3 H = N;
    float3 R = reflect(-V, N);
    float NoV = max(dot(N, V), 0);
    // float D = GTR2(NoH, roughness);
    float3 F0 = lerp(kDielectricSpec.rgb, albedo, metallic);
    float3 F = FresnelSchlick(NoV, F0);
    // half k = Kibl(roughness);
    // float G = GeometrySmith(NoV, NoL, k);
    half3 kd = (1 - F) * (1.0 - metallic);
    float3 diffuse = (kd*0.7+0.3) * albedo * envirGradient(N, 9.5);

    float rgh = roughness * (1.7 - 0.7 * roughness);
    float lod = 9 * rgh * rgh;  // Unity 默认 6 级 mipmap
    // float2 lut = SAMPLE_TEXTURE2D(_LutTex, sampler_LutTex, float2(NoV, roughness)).rg;
    float temp = (1.0 - NoV)* (1.0 - NoV);
    float2 lut = F.xy;
    float3 specular = envirGradient(R, lod) * (F0*lut.x*0.7 + lut.y);
    return diffuse*0.7 + specular;
}

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor*roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
    return Disney_Principle_BRDF(normal, toEye, lightVec, mat.DiffuseAlbedo.rgb, mat.Shininess, mat.FresnelR0.r) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;

    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    // result += ImageBasedLight(normal, toEye,  mat.DiffuseAlbedo.rgb, mat.Shininess, mat.FresnelR0.r)*0.7;

    return float4(result, 0.0f);
}

#endif