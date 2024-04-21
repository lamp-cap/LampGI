#include "fullScreenVS.hlsli"

Texture2D gDiffuse                : register(t0);
Texture2D gSpecualr               : register(t1);

Texture2D gBaseColorMap           : register(t2);
Texture2D gRoughnessMetallicMap   : register(t3);
Texture2D gNormalMap              : register(t4);
Texture2D gDepthMap               : register(t5);

float4 PS(VertexOut pin) : SV_Target
{
    float4 FRM = gRoughnessMetallicMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f);
	float3 fresnelR0 = FRM.rgb;
	float  roughness = FRM.a;
    
    float pz = gDepthMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);
    if (pz > 40) return 0.6;

	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;

	float4 SceneColor = float4(0,0,0,0);

	float3 normalVS = gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).rgb;
    float3 normalWS = normalize(mul(normalVS, (float3x3)gInvView));
    float3 viewDirWS = normalize(gEyePosW - posWS);
    float3 reflectWS = reflect(viewDirWS, normalWS);

    float fresnell = 1-max(dot(normalWS, viewDirWS),0);
    fresnell *= fresnell;
    fresnell = min(fresnell + fresnelR0.r, 1);
    fresnell *= fresnell;
    // return fresnell;
    float4 diffuse = gDiffuse.SampleLevel(gsamPointClamp, pin.TexC, 0);
    float4 specular = gSpecualr.SampleLevel(gsamLinearClamp, pin.TexC, 0);
    float4 result = lerp(diffuse, specular, fresnell);
    result.rgb = sqrt(result.rgb);
    float contrast = 1.1;
    result.rgb = ((result.rgb - 0.5) * contrast + 0.5) * 0.95;

    return result;
}


