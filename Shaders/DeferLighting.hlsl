
// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// #include "fullScreenVS.hlsli"
#include "Disney_BRDF.hlsli"

cbuffer cbPass : register(b0) {
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gViewProjTex;
    float4x4 gShadowTransform;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    Light gLights[MaxLights];
};

Texture2D gProbeND                : register(t0);
TextureCube gCubeMap              : register(t1);
Texture2D gShadowMap              : register(t2);
//Texture2D gSsaoMap                : register(t2); 
Texture2D gBaseColorMap           : register(t3);
Texture2D gRoughnessMetallicMap   : register(t4);
Texture2D gNormalMap              : register(t5);
Texture2D gDepthMap               : register(t6);

Texture2D gSH0                    : register(t7);
Texture2D gSH1                    : register(t8);
Texture2D gSH2                    : register(t9);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerComparisonState gsamShadow : register(s4);

 
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
	float2 TexC : TEXCOORD0;
};
float NdcDepthToViewDepth(float z_ndc)
{
    // z_ndc = A + B/viewZ, where gProj[2,2]=A and gProj[3,2]=B.
    float viewZ = gProj[3][2] / (z_ndc - gProj[2][2]);
    return viewZ;
}

// PCF for shadow mapping.
//#define SMAP_SIZE = (2048.0f)
//#define SMAP_DX = (1.0f / SMAP_SIZE)
float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    [unroll]
    for(int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}
float4 GetY01(float3 p){
    return float4(0.28209479, 0.488602512 * p.y, 0.488602512 * p.z, 0.488602512 * p.x);
}
float3 GetSH(uint2 uv, float3 normal)
{
    float4 sh0 = gSH0[uv];
    float4 sh1 = gSH1[uv];
    float4 sh2 = gSH2[uv];
    float4 shc01 = GetY01(normal);
    float3 res =
        float3(sh0.x, sh1.x, sh2.x) * shc01.x + 
        float3(sh0.y, sh1.y, sh2.y) * shc01.y + 
        float3(sh0.z, sh1.z, sh2.z) * shc01.z + 
        float3(sh0.w, sh1.w, sh2.w) * shc01.w ;
    return res;
}
float3 GetSHf(float2 uv, float3 normal)
{
    float4 sh0 = gSH0.SampleLevel(gsamLinearClamp, uv, 0);
    float4 sh1 = gSH1.SampleLevel(gsamLinearClamp, uv, 0);
    float4 sh2 = gSH2.SampleLevel(gsamLinearClamp, uv, 0);
    float4 shc01 = GetY01(normal);
    float3 res =
        float3(sh0.x, sh1.x, sh2.x) * shc01.x +
        float3(sh0.y, sh1.y, sh2.y) * shc01.y +
        float3(sh0.z, sh1.z, sh2.z) * shc01.z +
        float3(sh0.w, sh1.w, sh2.w) * shc01.w;
    return res;
}
bool isLegal(uint2 uv, float2 uvf)
{
    float a = gProbeND.SampleLevel(gsamPointClamp, uv/float2(160, 90), 0).a;
    float b = gDepthMap.SampleLevel(gsamPointClamp, uvf, 0).r;
    return abs(a-b)<0.5;
}

float4 PS(VertexOut pin) : SV_Target
{
    float ambientAccess = 1;

	float4 baseColor = gBaseColorMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f);

	float3 normalVS = normalize(gNormalMap.SampleLevel(gsamLinearWrap, pin.TexC, 0.0f).rgb);
    // normalVS.z *= -1;
    float3 normalWS = normalize(mul(normalVS, (float3x3)gInvView));

    float depth = gDepthMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f).r;
    float pz = NdcDepthToViewDepth(depth);

    float2 uvf = pin.TexC*float2(160, 90)-0.5;
    uint l = floor(uvf.x);
    uint r = l + 1;
    uint t = floor(uvf.y);
    uint b = t + 1;
    uint2 lt = uint2(l, t);
    uint2 rt = uint2(r, t);
    uint2 lb = uint2(l, b);
    uint2 rb = uint2(r, b);
    float x = 1 - uvf.x + lt.x;
    float y = 1 - uvf.y + lt.y;
    float weight = 0;
    
    float3 sh0 = GetSH(lt, normalWS)*x*y;
    if(isLegal(lt, pin.TexC)) weight += x*y;
    else sh0 = 0;
    float3 sh1 = GetSH(rt, normalWS)*(1-x)*y;
    if(isLegal(rt, pin.TexC)) weight += (1-x)*y;
    else sh1 = 0;
    float3 sh2 = GetSH(lb, normalWS)*x*(1-y);
    if(isLegal(lb, pin.TexC)) weight += x*(1-y);
    else sh2 = 0;
    float3 sh3 = GetSH(rb, normalWS)*(1-x)*(1-y);
    if(isLegal(rb, pin.TexC)) weight += (1-x)*(1-y);
    else sh3 = 0;

    weight = max(weight, 0.01);
    float3 gi = (sh0+sh1+sh2+sh3)/weight;
    
    // float3 gi = GetSHf(pin.TexC, normalWS);
    float4 FRM = gRoughnessMetallicMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f);
	float3 fresnelR0 = FRM.rgb;
	float  roughness = FRM.a;
	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;

    // Light terms.
    float4 ambient = ambientAccess*gAmbientLight*baseColor*0.0001;
    float3 toEyeW = normalize(gEyePosW - posWS);

    // Only the first light casts a shadow.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    float4 ShadowPosH = mul(float4(posWS, 1), gShadowTransform);
    shadowFactor[0] = CalcShadowFactor(ShadowPosH);

    const float shininess = (1.0f - roughness);
    Material mat = { baseColor, fresnelR0, roughness };
    float4 directLight = ComputeLighting(gLights, mat, posWS, normalWS, toEyeW, shadowFactor);

    float4 litColor = ambientAccess*directLight;
    float4 result = float4(gi*baseColor.rgb + litColor.rgb*1.5, 1);
    return result*1.5;
	// Add in specular reflections.
    // float3 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r).rgb;
    // float3 fresnelFactor = SchlickFresnel(fresnelR0, normalWS, r);
    // litColor += shininess * fresnelFactor * reflectionColor;

    float contrast = 1.1;
    result.rgb = ((result.rgb - 0.5) * contrast + 0.5) * 0.95;

    return result;
}


