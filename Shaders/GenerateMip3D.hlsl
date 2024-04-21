
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli

RWTexture3D<float4> OutMip1 : register(u0);
RWTexture3D<float4> OutMip2 : register(u1);
RWTexture3D<float4> OutMip3 : register(u2);
RWTexture3D<float4> OutMip4 : register(u3);

Texture3D<float4> SrcMip    : register(t0);

SamplerState gsamLinearClamp     : register(s0);
SamplerState gsamPointClamp      : register(s1);

float SrcMipLevelf  : register(b0);	// Texture level of source mip
float rcpWidth      : register(b1);
float rcpHeight     : register(b2);	// 1.0 / OutMip1.Dimensions
float rcpDepth      : register(b3);

groupshared float gs_R[512];
groupshared float gs_G[512];
groupshared float gs_B[512];
groupshared float gs_A[512];

void StoreColor( uint Index, float4 Color )
{
    gs_R[Index] = Color.r;
    gs_G[Index] = Color.g;
    gs_B[Index] = Color.b;
    gs_A[Index] = Color.a;
}

float4 LoadColor( uint Index )
{
    return float4( gs_R[Index], gs_G[Index], gs_B[Index], gs_A[Index]);
}

uint GetID(uint x, uint y, uint z)
{
    return x + y*8 + z*64;
}

[numthreads( 8, 8, 8 )]
void CS( uint GroupIndex : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
    uint SrcMipLevel = uint(SrcMipLevelf);
    float3 TexelSize = float3(rcpWidth, rcpHeight, rcpDepth);
    
    float3 UVW = TexelSize * (DTid.xyz + 0.5);
    float4 Src1 = SrcMip.SampleLevel(gsamLinearClamp, UVW, SrcMipLevel);
    OutMip1[DTid.xyz] = Src1;
    
    StoreColor(GroupIndex, Src1);

    GroupMemoryBarrierWithGroupSync();

    // this bit mask (binary: 001001001) checks that are even.
    if ((GroupIndex & 73) == 0) 
    {
        float4 Src2 = LoadColor(GroupIndex + 1);
        float4 Src3 = LoadColor(GroupIndex + 8);
        float4 Src4 = LoadColor(GroupIndex + 9);
        float4 Src5 = LoadColor(GroupIndex + 64);
        float4 Src6 = LoadColor(GroupIndex + 65);
        float4 Src7 = LoadColor(GroupIndex + 72);
        float4 Src8 = LoadColor(GroupIndex + 73);
        //Src1 = 0.125 * (Src1 + Src2 + Src3 + Src4 + Src5 + Src6 + Src7 + Src8);
        float alpha = Src1.a + Src2.a + Src3.a + Src4.a + Src5.a + Src6.a + Src7.a + Src8.a;
        float3 calc = Src1.a*Src1.rgb + Src2.a*Src2.rgb + Src3.a*Src3.rgb + Src4.a*Src4.rgb + Src5.a*Src5.rgb + Src6.a*Src6.rgb + Src7.a*Src7.rgb + Src8.a*Src8.rgb;
        Src1.rgb = alpha>0 ? calc*rcp(alpha) : 0;
        Src1.a = min(SrcMipLevel<1?alpha*0.5:0.125*alpha, 1);
        //Src1.a = min(0.125 * alpha, 1);
        


        OutMip2[DTid.xyz / 2] = Src1;
        StoreColor(GroupIndex, Src1);
    }

    GroupMemoryBarrierWithGroupSync();

    // This bit mask (binary: 011011011) checks that are multiples of four.
    if ((GroupIndex & 219) == 0)
    {
        float4 Src2 = LoadColor(GroupIndex + 2);
        float4 Src3 = LoadColor(GroupIndex + 16);
        float4 Src4 = LoadColor(GroupIndex + 18);
        float4 Src5 = LoadColor(GroupIndex + 128);
        float4 Src6 = LoadColor(GroupIndex + 130);
        float4 Src7 = LoadColor(GroupIndex + 144);
        float4 Src8 = LoadColor(GroupIndex + 146);
        //Src1 = 0.125 * (Src1 + Src2 + Src3 + Src4 + Src5 + Src6 + Src7 + Src8);
        float alpha = Src1.a + Src2.a + Src3.a + Src4.a + Src5.a + Src6.a + Src7.a + Src8.a;
        float3 calc = Src1.a*Src1.rgb + Src2.a*Src2.rgb + Src3.a*Src3.rgb + Src4.a*Src4.rgb + Src5.a*Src5.rgb + Src6.a*Src6.rgb + Src7.a*Src7.rgb + Src8.a*Src8.rgb;
        Src1.rgb = alpha>0 ? calc*rcp(alpha) : 0;
        Src1.a = min(0.125*alpha, 1);

        OutMip3[DTid.xyz / 4] = Src1;
        StoreColor(GroupIndex, Src1);
    }

    GroupMemoryBarrierWithGroupSync();

    // This bit mask would be 111111111 (X & Y & Z multiples of 8)
    if (GroupIndex == 0)
    {
        float4 Src2 = LoadColor(GroupIndex + 4);
        float4 Src3 = LoadColor(GroupIndex + 32);
        float4 Src4 = LoadColor(GroupIndex + 36);
        float4 Src5 = LoadColor(GroupIndex + 256);
        float4 Src6 = LoadColor(GroupIndex + 260);
        float4 Src7 = LoadColor(GroupIndex + 288);
        float4 Src8 = LoadColor(GroupIndex + 292);
        //Src1 = 0.125 * (Src1 + Src2 + Src3 + Src4 + Src5 + Src6 + Src7 + Src8);
        float alpha = Src1.a + Src2.a + Src3.a + Src4.a + Src5.a + Src6.a + Src7.a + Src8.a;
        float3 calc = Src1.a*Src1.rgb + Src2.a*Src2.rgb + Src3.a*Src3.rgb + Src4.a*Src4.rgb + Src5.a*Src5.rgb + Src6.a*Src6.rgb + Src7.a*Src7.rgb + Src8.a*Src8.rgb;
        Src1.rgb = alpha>0 ? calc*rcp(alpha) : 0;
        Src1.a = min(0.125*alpha, 1);

        OutMip4[DTid.xyz / 8] = Src1;
    }
}