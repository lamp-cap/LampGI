#include "fullScreenVS.hlsli"

Texture2D gNormalMap            : register(t0);
Texture2D gDepthMap             : register(t1);

float3 uvToVector3(float2 uv)
{
    uv = uv-0.5;
    uv = float2(uv.x+uv.y, -uv.x+uv.y);
    float3 N = float3( uv, 1 - abs(uv.x)-abs(uv.y));
    return N;
}
float UVRandom(float u, float v)
{
    float f = dot(float2(12.9898, 78.233), float2(u + gTotalTime * 0.05, v + gTotalTime * 0.1));
    return frac(43758.5453 * sin(f));
}
float2 CosSin(float theta)
{
    float sn, cs;
    sincos(theta, sn, cs);
    return float2(cs, sn);
}
float3 RandomVector(float2 i)
{
    float u = UVRandom(i.x, i.y) * 2.0 - 1.0;
    float v = UVRandom(i.y, i.x) * 3.14159265 * 2;
    float3 res = float3(CosSin(v) * sqrt(1.0 - u * u), u);
    return res;
}

float4 PS(VertexOut pin) : SV_Target
{
    int2 uvi = pin.TexC * float2(160, 90);
    float2 uvs = (uvi + 0.5) / float2(160, 90);

    float2 uvp = frac(pin.TexC * float2(160, 90));
    float3 dir = uvToVector3(uvp);
    dir = normalize(dir + float3(0, 0, 0.1f) + RandomVector(pin.TexC)*0.1f);

    float depth = gDepthMap.SampleLevel(gsamLinearClamp, uvs, 0).r - 0.005f;

	float3 normalVS = normalize(gNormalMap.SampleLevel(gsamLinearWrap, uvs, 0).rgb);
    float3 normalWS = normalize(mul(normalVS, (float3x3)gInvView));
    float3 up = abs(normalWS.y) < 0.999f ? float3(0, 1, 0) : float3(0, 0, 1);
    float3 tangent = normalize(cross(normalWS, up));
    float3 binormal = cross(normalWS, tangent);
    float3x3 TBN = float3x3(tangent, binormal, normalWS);

    dir = mul(dir, TBN);
    
    return float4(dir, depth);
}
