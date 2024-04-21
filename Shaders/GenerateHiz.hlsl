
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli

RWTexture2D<float> OutMip1 : register(u0);
RWTexture2D<float> OutMip2 : register(u1);
RWTexture2D<float> OutMip3 : register(u2);
RWTexture2D<float> OutMip4 : register(u3);

Texture2D<float> SrcMip    : register(t0);

SamplerState gsamLinearClamp     : register(s0);
SamplerState gsamPointClamp      : register(s1);

float SrcMipLevelf  : register(b0);
float NumMipLevels  : register(b1);
float rcpWidth      : register(b2);
float rcpHeight     : register(b3);

groupshared float gs_D[64];

[numthreads( 8, 8, 1 )]
void CS( uint GroupIndex : SV_GroupIndex, uint3 dispatchThreadID : SV_DispatchThreadID )
{
    uint SrcMipLevel = uint(SrcMipLevelf);
    float2 TexelSize = float2(rcpWidth, rcpHeight);
    
    float2 UV = TexelSize * dispatchThreadID.xy;
    float Src1 = SrcMip.SampleLevel(gsamPointClamp, UV, SrcMipLevel);
    OutMip1[dispatchThreadID.xy] = Src1;

    // A scalar (constant) branch can exit all threads coherently.
    if (NumMipLevels < 2)
        return;

    gs_D[GroupIndex] = Src1;

    GroupMemoryBarrierWithGroupSync();

    if ((GroupIndex & 0x9) == 0)
    {
        float Src2 = gs_D[GroupIndex + 0x01];
        float Src3 = gs_D[GroupIndex + 0x08];
        float Src4 = gs_D[GroupIndex + 0x09];
        Src1 = min(Src1, min(Src2, min(Src3, Src4)));

        OutMip2[dispatchThreadID.xy / 2] = Src1;
        gs_D[GroupIndex] = Src1;
    }

    if (NumMipLevels < 3)
        return;

    GroupMemoryBarrierWithGroupSync();

    if ((GroupIndex & 0x1B) == 0)
    {
        float Src2 = gs_D[GroupIndex + 0x02];
        float Src3 = gs_D[GroupIndex + 0x10];
        float Src4 = gs_D[GroupIndex + 0x12];
        Src1 = min(Src1, min(Src2, min(Src3, Src4)));

        OutMip3[dispatchThreadID.xy / 4] = Src1;
        gs_D[GroupIndex] = Src1;
    }

    if (NumMipLevels < 4)
        return;

    GroupMemoryBarrierWithGroupSync();

    if (GroupIndex == 0)
    {
        float Src2 = gs_D[GroupIndex + 0x04];
        float Src3 = gs_D[GroupIndex + 0x20];
        float Src4 = gs_D[GroupIndex + 0x24];
        Src1 = min(Src1, min(Src2, min(Src3, Src4)));

        OutMip4[dispatchThreadID.xy / 8] = Src1;
    }
}