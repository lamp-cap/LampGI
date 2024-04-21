#include "fullScreenVS.hlsli"

Texture2D gSrcMap               : register(t0);
Texture2D gDepthMap             : register(t1);

struct PixelOutput
{
    float4 color                    : SV_TARGET0;
    float depth		                : SV_TARGET1;
};

PixelOutput PS(VertexOut pin)
{   
    PixelOutput o;
    o.color = gSrcMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
    o.depth = gDepthMap.SampleLevel(gsamLinearClamp, pin.TexC, 0).r;
    return o;
}


