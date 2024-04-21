
Texture2D gSrcMap       : register(t0);
Texture2D gOffscreenMap : register(t1);
Texture2D gDepthMap     : register(t2);

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
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
	float2 TexC : TEXCOORD0;
};

float4 PS(VertexOut pin) : SV_Target
{
    float x = gInvRenderTargetSize.x*2;
    float y = gInvRenderTargetSize.y*2;
    float4 c = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC, 0);
    // return pow(c, 0.3);
    float4 l = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC+float2(x, y), 0);
    float4 r = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC+float2(-x, y), 0);
    float4 t = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC+float2(x, -y), 0);
    float4 b = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC+float2(-x, -y), 0);
    float4 lt = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC + float2(0, y), 0);
    float4 rt = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC + float2(-x, 0), 0);
    float4 lb = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC + float2(0, -y), 0);
    float4 rb = gSrcMap.SampleLevel(gsamPointClamp, pin.TexC + float2(x, 0), 0);
    float4 res = 0.1111111*(c+l+r+t+b+lt+lb+rt+rb);
    // return pow(res, 0.3);
    float4 scene = gOffscreenMap.SampleLevel(gsamPointClamp, pin.TexC, 0);
    // return pow(scene, 0.45);

	float depth = gDepthMap.SampleLevel(gsamPointClamp, pin.TexC, 0);
    // if(depth)
    // scene = depth>0.99?scene:sqrt(scene);
    float4 result = depth > 0.99 ? 0 : pow(scene*c.a+ res, 0.35)*1.2;

    float contrast = 1.1;
    result.rgb = ((result.rgb - 0.5) * contrast + 0.5)*0.95;
    return result;
}
