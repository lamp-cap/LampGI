
Texture2D gHistory                : register(t0);
Texture2D gOffScreenMap           : register(t1);
Texture2D gDepthMap               : register(t2);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);

cbuffer cbPass : register(b0) {
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gLastViewProj;
    float2 _Offsets[9];
    float2 gInvRenderTargetSize;
    float gInflunce;
    float gTotalTime;
};

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};
 
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
	float2 TexC : TEXCOORD0;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = gTexCoords[vid];
    vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);
    float4 ph = mul(vout.PosH, gInvProj);
    vout.PosV = ph.xyz / ph.w;
    return vout;
}

float NdcDepthToViewDepth(float z_ndc)
{
    float viewZ = gProj[3][2] / (z_ndc - gProj[2][2]);
    return viewZ;
}

float SampleLinearEyeDepth(float2 uv)
{
    float depth = gDepthMap.SampleLevel(gsamPointClamp, uv, 0).r;
    return NdcDepthToViewDepth(depth);
}

float3 RGBToYCoCg(float3 color)
{
    return float3(  0.25*color.r + 0.5*color.g + 0.25*color.b,
                    -0.25*color.r + 0.5*color.g - 0.25*color.b,
                    0.5*color.r - 0.5*color.b);
}
float3 YCoCgToRGB(float3 color)
{
    return float3(color.r - color.g + color.b,  color.r + color.g, color.r - color.g - color.b);
}
void GetColorRange(float2 uv, inout float3 minColor, inout float3 maxColor)
{
    [unroll]
    for(int i=0; i<9; i++)
    {
        float3 color = RGBToYCoCg(gOffScreenMap.SampleLevel(gsamPointClamp, uv + _Offsets[i] * 2, 0).rgb);
        minColor = (minColor.r > color.r * 0.98)? color * 0.98 : minColor;
        maxColor = (maxColor.r < color.r * 1.02)? color * 1.02 : maxColor;
    }
    return;
}
float3 ClampColor(float3 color, float3 minColor, float3 maxColor)
{
    float3 historyYCoCg = RGBToYCoCg(color);
    if(historyYCoCg.r<minColor.r) historyYCoCg = minColor;
    if(historyYCoCg.r>maxColor.r) historyYCoCg = maxColor;
    // historyYCoCg = max(historyYCoCg, minColor);
    // historyYCoCg = min(historyYCoCg, maxColor);
    return YCoCgToRGB(historyYCoCg);
}
float2 ComputeCameraVelocity(float2 uv, float3 PosWS)
{
    float4 posWS = float4(PosWS, 1);

    float4 posCSCur = mul(posWS, gViewProj);
    float4 posCSPre = mul(posWS, gLastViewProj);
    posCSCur.xyz *= rcp(posCSCur.w); posCSCur.x *= -1;
    posCSPre.xyz *= rcp(posCSPre.w); posCSPre.x *= -1;
    float2 posNDCCur = 0.5 - posCSCur.xy * 0.5;
    float2 posNDCPre = 0.5 - posCSPre.xy * 0.5;
    float2 velocity = posNDCCur - posNDCPre;
    return velocity;
}
float2 AdjustBestDepthOffset(float2 uv)
{
    float bestDepth = 1.0f;
    float2 uvOffset = 0.0f;

    [unroll]
    for (int k = 0; k < 9; k++) {
        float depth = gDepthMap.SampleLevel(gsamPointClamp, uv + _Offsets[k] * 3, 0).r;

        if (depth > bestDepth) {
            bestDepth = depth;
            uvOffset = _Offsets[k] * 3;
        }
    }
    return uvOffset;
}
float RGBToLuminance(float3 color)
{
    return dot(color, float3(0.2126729f,  0.7151522f, 0.0721750f));
}
float4 PS (VertexOut pin) : SV_Target
{
    float2 uv = pin.TexC;
    float4 color = gOffScreenMap.SampleLevel(gsamPointClamp, uv, 0);
    float depth = SampleLinearEyeDepth(uv);
    if(depth>40)
        return color;

    float4 posVS = float4((depth/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;

    // 根據深度差調整
    float2 offsetUV = AdjustBestDepthOffset(uv);
    // 計算uv偏移
    float2 velocity = ComputeCameraVelocity(uv + offsetUV, posWS);
    // return float4(velocity*15, 0, 1);
    float2 historyUV = uv - velocity;
    float4 accum = gHistory.SampleLevel(gsamLinearClamp, historyUV, 0);
    float3 minColor = 1.0;
    float3 maxColor = 0.0;
    GetColorRange(uv, minColor, maxColor);
    // accum.rgb = ClampColor(accum.rgb, minColor, maxColor);
    // return accum;
    float4 res = lerp(accum, color, 0.1);
    return res;
}


