#include "fullScreenVS.hlsli"

TextureCube gCubeMap            : register(t0);
Texture2D gOffScreenMap         : register(t1);
Texture2D gNormalMap            : register(t2);
Texture2D gHiz   	            : register(t3);
Texture2D gProbeND              : register(t4);

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
/*
float2 ComputeCameraVelocity(float2 uv, float3 PosWS)
{
    float4 posWS = float4(posWS, 1);

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
*/
float3 SampleScreenColor(float3 sampleDir, float3 posWS, float2 uv, float depth, out bool isHit)
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

    int count = 0;
    
    float2 uv_s = uv + offset; // 當前 uv 
    float depth_n = depth + dz*0.5; // 當前深度
    float depth_s = 0;

    float3 color = 0;
    int level = 0;
    [loop]
    for(int i=0; i<64; i++)
    {
        count++;
        uv_s += offset * pow2(level);
        depth_n += dz * pow2(level);
        if( uv_s.x*uv_s.y<=0 || uv_s.x>1 || uv_s.y>1 || depth_n < 0.01|| depth_n>45) 
        {
            if(level<2)
            {
                // color = gCubeMap.SampleLevel(gsamLinearClamp, sampleDir, 0);
                // color*=color;
                break;
            }
            
            uv_s -= offset * pow2(level);
            depth_n -= dz * pow2(level);
            level--;
            continue;
        }
        depth_s = NdcDepthToViewDepth(gHiz.SampleLevel(gsamPointClamp, uv_s, level).r);
        if(depth_n >= depth_s)
        {
            if(level<1)
            {
                isHit = true;
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
        else if(level < 6) level++;
    }

    if(isHit) 
    {
        if(depth_n - NdcDepthToViewDepth(gHiz.SampleLevel(gsamPointClamp, uv_s, 0).r)<0.1f)
        {
            float3 normal_s = normalize(mul(gNormalMap.SampleLevel(gsamPointClamp, uv_s, 0).rgb, (float3x3)gInvView));
            if(dot(normal_s, -sampleDir)>0.1f)
            {
                // float2 offsetUV = AdjustBestDepthOffset(uv);
                // float2 velocity = ComputeCameraVelocity(uv + offsetUV);
                // float2 historyUV = uv - velocity;
                return gOffScreenMap.SampleLevel(gsamLinearClamp, uv_s, log2(count)).rgb;
            }
        }
        isHit = false;
    }
    
    return color;

}

float4 PS(VertexOut pin) : SV_Target
{
    float4 NormalDepth = gProbeND.SampleLevel(gsamLinearWrap, pin.TexC, 0);
    float3 sampleDir = NormalDepth.rgb;
    float depth = NormalDepth.w;
    float pz = NdcDepthToViewDepth(depth);
    if(pz>40) return 0;

	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;

    // float3 randomVec = RandomVector(posWS);
    // sampleDir = normalize(randomVec*0.05 + sampleDir);
    bool isHit = false;
    float3 SSGIColor = SampleScreenColor(sampleDir, posWS, pin.TexC, pz, isHit);

    return float4(SSGIColor*SSGIColor, isHit?1:0);
}


