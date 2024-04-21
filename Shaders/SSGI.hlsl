
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

#include "fullScreenVS.hlsli"

TextureCube gCubeMap            : register(t0);
Texture2D gOffScreenMap         : register(t1);
Texture2D gBaseColorMap         : register(t2);
Texture2D gRMMap                : register(t3);
Texture2D gNormalMap            : register(t4);
Texture2D gDepthMap             : register(t5);
Texture2D gHiz   	            : register(t6);

int log2(int x)
{
    int rst = 0;
    if (x & 240) rst +=  4, x >> 4;
    if (x & 12) rst +=  2, x >> 2;
    if (x & 2) rst +=  1;
    return rst;
}
float Random(float3 i)
{
    float f = dot(float2(12.9898, 78.233), float2(i.x + gTotalTime*0.01*i.z, i.y*gTotalTime));
    return frac(43758.5453 * sin(f));
}
float2 CosSin(float theta)
{
    float sn, cs;
    sincos(theta, sn, cs);
    return float2(cs, sn);
}
float3 RandomVector(float3 i)
{
    float u = Random(i) * 2.0 - 1.0;
    float v = Random(i.zxy) * 3.14159265 * 2;
    float3 res = float3(CosSin(v) * sqrt(1.0 - u * u), u);
    return res;
}
int pow2(int x)
{
    return x>7?255 : 1 << x;
}

float4 SampleReflectColor(float3 normalWS, float3 sampleDir, float3 posWS, float2 uv, float depth, float roughness, float metallic)
{
    float3 pos_w1 = posWS + sampleDir;
    float4 pos_s1 = mul(float4(pos_w1, 1), gViewProj);
    pos_s1.xyz /= pos_s1.w;
    float depth1 = pos_s1.w;
    pos_s1.x = -pos_s1.x;
    
    float3 delta = float3(0.5-pos_s1.xy*0.5 - uv, depth1 - depth);
    delta = (abs(delta.y)>abs(delta.x)) ? delta/abs(delta.y*768) : delta/abs(delta.x*1280);

    float2 offset = float2(delta.x, delta.y);
    float dz = delta.z;

    const int sampleNum = 32;
    int count = 0;
    
    float2 uv_s = uv + offset; // 當前 uv 
    float depth_n = depth + dz*0.5; // 當前深度
    float depth_s = 0;
    float3 color = 0;
    bool ishit = false;
    int level = 0;
    [loop]
    for(int i=0; i<64; i++)
    {
        count+=1;
        uv_s += offset * pow2(level);
        depth_n += dz * pow2(level);
        if( uv_s.x*uv_s.y<=0 || uv_s.x>1 || uv_s.y>1 || depth_n < 0.01|| depth_n>45) 
        {
            if(level<2)
                break;
            
            uv_s -= offset * pow(2, level);
            depth_n -= dz * pow(2, level);
            level--;
            continue;
        }
        depth_s = NdcDepthToViewDepth(gHiz.SampleLevel(gsamPointClamp, uv_s, level).r);
        if(depth_n > depth_s)
        {
            if(level<1)
            {
                ishit = true;
                break;
            }
            else
            {
                uv_s -= offset * pow2(level);
                depth_n -= dz * pow2(level);
                level--;
                continue;
            }
        }
        if(level < 6)level++;
    }

    if(ishit && depth_n - NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamPointClamp, uv_s, 0).r)<0.11f) 
    {
        // color *= lerp(dot(sampleDir, normalWS)*0.4+0.6, 1, metallic);
        float3 normal_s = normalize(mul(gNormalMap.SampleLevel(gsamPointClamp, uv_s, 0).rgb, (float3x3)gInvView));
        if(dot(normal_s, -sampleDir)>0.1f)
        {
            // float2 offsetUV = AdjustBestDepthOffset(uv);
            // float2 velocity = ComputeCameraVelocity(uv + offsetUV);
            // float2 historyUV = uv - velocity;
            color = gOffScreenMap.SampleLevel(gsamLinearClamp, uv_s, log2(count)*roughness).rgb;
        }
        ishit = false;
    }
    // color = (float)count/sampleNum;
    // color = log2(count)*roughness*0.16;
    return float4(color, 1);

}

float4 PS(VertexOut pin) : SV_Target
{
    float3 SceneColor = gOffScreenMap.Sample(gsamLinearWrap, pin.TexC).rgb;
    float depth = gDepthMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f).r;
    float pz = NdcDepthToViewDepth(depth);
    if(pz>40) return float4(SceneColor, 1);

	float4 baseColor = gBaseColorMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f);
	float3 normalVS = normalize(gNormalMap.SampleLevel(gsamLinearWrap, pin.TexC, 0.0f).rgb);
    float3 normalWS = normalize(mul(normalVS, (float3x3)gInvView));
    float4 FRM = gRMMap.SampleLevel(gsamLinearClamp, pin.TexC, 0.0f);
	float3 fresnelR0 = FRM.rgb;
	float  roughness = FRM.a * FRM.a * 0.8;
	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;

	// roughness = 0;
    // Light terms.
    float3 ViewDirWS = normalize(posWS - gEyePosW);
    float3 reflectWS = normalize(reflect(ViewDirWS, normalWS));
    float4 reflectColor = 0;
    float3 randomVec = posWS;
    //[unroll]
    //for(int i=0; i<4; i++)
    //{
        randomVec = RandomVector(randomVec);
        float3 sampleDir = normalize(reflectWS);
        // sampleDir = reflectWS;
        reflectColor += SampleReflectColor(normalWS, sampleDir, posWS, pin.TexC, pz, roughness, fresnelR0.r);
    //}
    //reflectColor.rgb = reflectColor.a>0 ? reflectColor.rgb / reflectColor.a : 0;
    //reflectColor.a *= 0.25;

    float fresnell = 1 - max(dot(-ViewDirWS, normalWS), 0);
    // fresnell*=fresnell;
    fresnell = min(fresnell*fresnell*fresnell + fresnelR0.r, 1);
    // return fresnell;
    float weight = reflectColor.a * fresnell;
    // weight *= frac(gTotalTime*0.6);
    float3 res = lerp(SceneColor, reflectColor.rgb, weight);

    return float4(reflectColor.rgb, weight);
    float contrast = 1.2;
    res.rgb = (res.rgb - 0.5) * contrast + 0.5 + 0.1;
    const float3 luminanceWeighting = float3(0.2125, 0.7154, 0.0721);
    float luminance = dot(res, luminanceWeighting);
    //构造灰度校准向量
    float3 greyScaleColor = luminance;
    float saturation = 1.1;
    res = lerp(greyScaleColor, res, saturation);
    float we = max(depth - 0.9, 0)*12;
    we = we*we;
    float3 back = float3(0.5, 0.55, 0.65);
    // return we;
    return float4(lerp(res, back, we), 1);
}


