
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

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

Texture2D gTextureMaps[10]   : register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
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

// Transforms a normal map sample to world space.
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	// Uncompress each component from [0,1] to [-1,1].
	float3 normalT = 2.0f*normalMapSample - 1.0f;

	// Build orthonormal basis.
	float3 N = unitNormalW;
	float3 T = normalize(tangentW - dot(tangentW, N)*N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent space to world space.
	float3 bumpedNormalW = mul(normalT, TBN);

	return bumpedNormalW;
}

struct VertexIn
{
    float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC    : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH     : SV_POSITION;
    float3 NormalW  : NORMAL;
    float2 TexC     : TEXCOORD;
    float3 TangentW : TANGENT;
};

struct PixelOutput
{
    float4 BaseColor                    : SV_TARGET0;
    float4 RoughnessMetallic	        : SV_TARGET1;
    float4 Normal		                : SV_TARGET2;
    // float2 Velocity		            : SV_TARGET3;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    // vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
	
	vout.TangentW = mul(normalize(vin.TangentU), (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

PixelOutput PS(VertexOut pin)
{
    PixelOutput Out;

	MaterialData matData = gMaterialData[gMaterialIndex];

    uint Width = 0, Height = 0;
    
    // BaseColor
    // BaseColorTexture.GetDimensions(Width, Height);
    // if(Width == 1)
	// {
        // Out.BaseColor = matData.DiffuseAlbedo;
	// }
	// else
	// {
        Out.BaseColor = matData.DiffuseAlbedo * gTextureMaps[matData.DiffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
	// }

    // Normal
	// NormalTexture.GetDimensions(Width, Height);
	// if (Width == 1)
	// {
		// Out.Normal = float4(mul(normalize(pin.NormalW), (float3x3)gView), 0.0f);
	// }
	// else
	// {
        float4 normalMapSample = gTextureMaps[matData.NormalMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
        float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
		float3 NormalWS = float4(normalize(bumpedNormalW), 1.0f).xyz;
        Out.Normal = float4(mul(normalize(NormalWS*0.5 + pin.NormalW), (float3x3)gView), 0.0f);
        // Out.Normal = float4(mul(normalize(pin.NormalW), (float3x3)gView), 0.0f);
	// }

    // Metallic
    float3 Metallic = matData.FresnelR0;
 	// MetallicTexture.GetDimensions(Width, Height);  
	// if (Width > 1)
	// {
	// 	Metallic = MetallicTexture.Sample(gsamAnisotropicWrap, pin.TexC).r;
	// }   
    
    // Roughness
    float Roughness = matData.Roughness;
	// RoughnessTexture.GetDimensions(Width, Height);
	// if (Width > 1)
    // {
	// 	Roughness = RoughnessTexture.Sample(gsamAnisotropicWrap, pin.TexC).r;
	// }
    
    Out.RoughnessMetallic = float4(Metallic, Roughness);
    
    // // Velocity
    // float4 CurPos = pin.CurPosH;
    // CurPos /= CurPos.w; // Complete projection division
    // CurPos.xy = NDCToUV(CurPos);
    
    // float4 PrevPos = pin.PrevPosH;
    // PrevPos /= PrevPos.w; // Complete projection division
    // PrevPos.xy = NDCToUV(PrevPos);
    
    // Out.Velocity = CurPos.xy - PrevPos.xy;
    
    return Out;
}


