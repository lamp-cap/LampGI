
// Include structures and functions for lighting.
#include "Common.hlsli"

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

Texture2D gShadowMap : register(t0);
Texture2D gTextureMaps[10] : register(t1);

RWTexture3D<float4> gVoxelColor        : register(u0);
RWTexture3D<float4> gVoxelNormal       : register(u1);
RWTexture3D<float4> gVoxelMat          : register(u2);

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
    float4 ShadowPosH : POSITION;
    float3 NormalW  : NORMAL;
    float2 TexC     : TEXCOORD;
    float3 TangentW : TANGENT;
};

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 PosCS : POSITION1;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD0;
    float3 TangentW : TANGENT;
};

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

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	MaterialData matData = gMaterialData[gMaterialIndex];

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);
    vout.PosH = float4(posW.xyz*0.08, 1.0f);
    vout.ShadowPosH = mul(posW, gShadowTransform);
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

float3 SwizzleAxis(float3 position, uint axis) 
{
    uint a = axis + 1;
    float3 p = position;
    position.x = p[(0 + a) % 3];
    position.y = p[(1 + a) % 3];
    position.z = p[(2 + a) % 3];

    return position;
}

// Restore coordinate axis back to correct position
float3 RestoreAxis(float3 position, uint axis) 
{
    uint a = 2 - axis;
    float3 p = position;
    position.x = p[(0 + a) % 3];
    position.y = p[(1 + a) % 3];
    position.z = p[(2 + a) % 3];

    return position;
}

[maxvertexcount(3)]
void GS(triangle VertexOut vert[3], inout TriangleStream<GeoOut> triStream)
{
    float3 normal = normalize(abs(cross(vert[1].PosH.xyz - vert[0].PosH.xyz, vert[2].PosH.xyz - vert[0].PosH.xyz)));

    // Choose an axis with the largest projection area
    bool towardsX = normal.x > normal.y && normal.x > normal.z;
    bool towardsY = normal.y > normal.x && normal.y > normal.z;
    uint axis = towardsX ? AXIS_X : towardsY ? AXIS_Y : AXIS_Z;

    [unroll]
    for (int j = 0; j < 3; j++) {
        GeoOut o;

        o.PosH = float4(SwizzleAxis(vert[j].PosH.xyz, axis), 1.0f);
        o.PosCS = vert[j].PosH;
        o.ShadowPosH = vert[j].ShadowPosH;
        o.NormalW = vert[j].NormalW;
        o.TangentW = vert[j].TangentW;
        o.TexC = vert[j].TexC;

        triStream.Append(o);
    }
}

uint EncodeRGBAuint(float4 color)
{
    uint result = 0;

    uint a = uint(color.a * 256);
    uint r = uint(color.r * 256);
    uint g = uint(color.g * 256);
    uint b = uint(color.b * 256);

    result += a;
    result += b << 8; 
    result += g << 16;
    result += r << 24;

    return result;
}

float4 DecodeRGBAuint(uint value)
{
    uint a = value & 0x000000FF;
    uint b = (value >> 8) & 0x000000FF;
    uint g = (value >> 16) & 0x000000FF;
    uint r = (value >> 24) & 0x000000FF;
    return float4(r, g, b, a)/256.0;
}

float4 PS(GeoOut pin) : SV_Target
{
	MaterialData matData = gMaterialData[gMaterialIndex];

    float4 BaseColor = matData.DiffuseAlbedo * gTextureMaps[matData.DiffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float4 normalMapSample = gTextureMaps[matData.NormalMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    float3 NormalWS = normalize(bumpedNormalW);
    float shadow = CalcShadowFactor(pin.ShadowPosH);
    float radiance = max(dot(normalize(-gLights[0].Direction), NormalWS),0);
    radiance *= shadow;

    // Metallic
    float Metallic = matData.FresnelR0.r;
    float Roughness = matData.Roughness;
    float Emmisive = 0;
    float4 mat = float4(Metallic, Roughness, Emmisive, 1);
    float4 Normal = float4(NormalWS*0.5+0.5, 1);
    // float4 OcclusionRoughnessMetallic = float4(Metallic, Roughness);
    // pin.PosH.xyz = RestoreAxis(pin.PosH.xyz, pin.axis);
    // pin.PosH = pin.PosCS;
    uint3 coord = uint3(uint(pin.PosCS.x*128 + 128), uint(pin.PosCS.y*128 + 2), uint(((pin.PosCS.z*128 + 128))) );
    // while (gVoxel[coord] != color)
    //float4 color = BaseColor;
    float4 color = float4(BaseColor.rgb * (radiance * 0.32 + 0.0f) * (1-Metallic*0.5), 1);
    // float4 color = float4(BaseColor.rgb, 1);
    
    gVoxelColor[coord] = gVoxelColor[coord].a > 0 ? lerp(gVoxelColor[coord], color, 0.5) : color;
    gVoxelNormal[coord] = gVoxelNormal[coord].a > 0 ? lerp(gVoxelNormal[coord], Normal, 0.5) : Normal;
    gVoxelMat[coord] = gVoxelMat[coord].a > 0 ? lerp(gVoxelMat[coord], mat, 0.5) : mat;

    // uint writeValue = EncodeRGBAuint(color);
    // uint compareValue = 0;
    // uint originalValue;

    // [allow_uav_condition]
    // while (true)
    // {
    //     InterlockedCompareExchange(gVoxel[coord], compareValue, writeValue, originalValue);
    //     if (compareValue == originalValue)
    //         break;
    //     compareValue = originalValue;
    //     float4 originalValueFloats = DecodeRGBAuint(originalValue);
    //     writeValue = EncodeRGBAuint(originalValueFloats + color);
    // }

    return pin.PosH*0.002;
}



