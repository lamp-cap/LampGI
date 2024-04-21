
// Include structures and functions for lighting.
#include "LightingUtil.hlsli"

struct MaterialData
{
	float4   DiffuseAlbedo;
	float3   FresnelR0;
	float    Roughness;
	float4x4 MatTransform;
	uint     DiffuseMapIndex;
	uint     NormalMapIndex;
	uint     MatPad1;
	uint     MatPad2;
};

Texture2D gHistory                : register(t0);
Texture2D gOffScreenMap           : register(t1);
Texture2D gTempMap1               : register(t2);
Texture2D gTempMap2               : register(t3);
Texture2D gCubeMap                : register(t4);
Texture2D gShadowMap              : register(t5);
Texture2D gColorLSMap             : register(t6);
Texture2D gNormalLSMap            : register(t7);
Texture2D gSsaoMap1               : register(t8);
Texture2D gSsaoMap2  		  	  : register(t9);
Texture2D gNormalMap              : register(t10);
Texture2D gDepthMap               : register(t11);
Texture2D gRandomVecMap 		  : register(t12);
Texture2D gBaseColorMap           : register(t13);
Texture2D gRoughnessMetallicMap   : register(t14);
Texture2D gHiz   	              : register(t15);
Texture2D gSobelMap               : register(t16);
Texture2D gSobelMap1              : register(t17);
Texture3D gVoxelMap               : register(t18);
Texture3D gVoxelMap1              : register(t19);
Texture2D gVoxelTemp              : register(t20);

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
	float4x4 gTexTransform;
	uint gMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};
// Constant data that varies per material.
cbuffer cbPass : register(b1)
{
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

struct VertexIn
{
	float3 PosL    : POSITION;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosV    : POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);
	
	vout.TexC = vin.TexC;
    
    float4 ph = mul(float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f), gInvProj);
    vout.PosV = ph.xyz / ph.w;
	
    return vout;
}

float4 DecodeRGBAuint(uint value)
{
    float r = (value>>24) / 256.0;
    float g = ((value >> 16) & 0x000000FF) / 256.0;
    float b = ((value >> 16) & 0x000000FF) / 256.0;
    float a = ((value >> 16) & 0x000000FF) / 256.0;

    return float4(r, g, b, a);
}

float NdcDepthToViewDepth(float z_ndc)
{
    // z_ndc = A + B/viewZ, where gProj[2,2]=A and gProj[3,2]=B.
    float viewZ = gProj[3][2] / (z_ndc - gProj[2][2]);
    return viewZ;
}
 
float4 PS(VertexOut pin) : SV_Target
{
    float depth = gDepthMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f).r;
    float pz = NdcDepthToViewDepth(depth);
	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;
    // float4 color = 0;
    // [unroll]
    // for(int i=0; i<250; i++)
    // {
    //     color = gVoxelMap.Sample(gsamLinearWrap, float3(pin.TexC, i/256.0));
    //     if(color.a > 0) break;
    // }
    float3 his = gShadowMap.SampleLevel(gsamPointClamp, pin.TexC, 0).rgb;
    // float3 res = gCubeMap.SampleLevel(gsamLinearWrap, posVS, 0).rgb;
    // return 0;
    return float4(his, 1.0f);
}


