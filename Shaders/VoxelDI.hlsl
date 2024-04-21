#include "fullScreenVS.hlsli"

TextureCube gCubeMap            : register(t0);

Texture2D gProbeND              : register(t1);

Texture3D gVoxelColor	        : register(t2);
Texture3D gVoxelMat	            : register(t3);
Texture3D gVoxelNormal	        : register(t4);

Texture3D gProbeSH0             : register(t5);
Texture3D gProbeSH1             : register(t6);
Texture3D gProbeSH2             : register(t7);

Texture2D gCurProbe             : register(t8);

static const float3 _MaxDis = float3(12.5, 12.3046875, 12.5);
static const float3 _MinDis = float3(-12.5, -0.1953124, -12.5);

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
float4 GetY01(float3 p){
    return float4(0.28209479, 0.488602512 * p.y, 0.488602512 * p.z, 0.488602512 * p.x);
}
float3 GetSH(uint3 uvw, float3 normal)
{
    float4 sh0 = gProbeSH0[uvw];
    float4 sh1 = gProbeSH1[uvw];
    float4 sh2 = gProbeSH2[uvw];
    float4 shc01 = GetY01(normal);
    float3 res =
        float3(sh0.x, sh1.x, sh2.x) * shc01.x + 
        float3(sh0.y, sh1.y, sh2.y) * shc01.y + 
        float3(sh0.z, sh1.z, sh2.z) * shc01.z + 
        float3(sh0.w, sh1.w, sh2.w) * shc01.w ;
    return res;

}
float3 positionToUVWf(float3 position) {
    float3 uvw = position - _MinDis;
    uvw += 0.00001;
    uvw.x /= max(_MaxDis.x - _MinDis.x, 0.01);
    uvw.y /= max(_MaxDis.y - _MinDis.y, 0.01);
    uvw.z /= max(_MaxDis.z - _MinDis.z, 0.01);
    return uvw;
}
float4 sample3DColorLod(float3 position, int lod) {
    float3 uvw = positionToUVWf(position);
    lod = min(lod, 6);
    return gVoxelColor.SampleLevel(gsamLinearClamp, uvw, lod);
}

float3 TraceVoxelColor(float3 start, float3 sampleDir, out bool hit)
{
    float axis = max(abs(sampleDir.z), max(abs(sampleDir.x), abs(sampleDir.y))); 
    float3 offset = sampleDir / axis;
    offset *= 0.09765625;

    float k = Random(start)+1;
    float3 current = start + offset*k;
    float4 result = 0;
    float travelled = 0;
    int lod = 0;
    int i=0;
    [loop]
    for(i=0; i<24; i++)
    {
        travelled += pow2(lod);
        current += offset * pow2(lod);
        if (abs(current.x)>_MaxDis.x || current.y>_MaxDis.y || current.y<_MinDis.y || abs(current.z)>_MaxDis.z) 
        {    
            // result = gCubeMap.SampleLevel(gsamLinearClamp, sampleDir, 0);
            // result *= result;
            result = 0.6;
            break;
        }
        
        float4 temp = sample3DColorLod(current, lod);
        if(temp.a>0)
        {
            if(lod<1)
            {
                // float4 shadowCoord = TransformWorldToShadowCoord(current);
                // result += float4(temp.rgb*((6-lod) * (6-lod)/36.0)*(MainLightRealtimeShadow(shadowCoord)*0.9+0.1), temp.a);
                result += float4(temp.rgb/temp.a, 1);
                float3 uvw = positionToUVWf(current)*float3(32, 16, 32);
                result.rgb += saturate(GetSH(uvw, sampleDir)*3);
                lod = max(lod-1, 0);
                break;
            }
            travelled -= pow2(lod);
            current -= offset * pow2(lod);
            lod--;
            continue;
        }
        else lod = min(lod+1, 3);
        if(result.a>0.95) break;
    }
    float3 sky = float3(0.2, 0.3, 0.4);
    sky = 0;
    if(result.a>0.9) hit = true;
    return result.rgb;
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

    // float3 randomVec = RandomVector(posVS.xyz);
    bool isHit = false;
    int2 uvi = pin.TexC * float2(160, 90);
    float2 uvs = (uvi + 0.5) / float2(160, 90);
    float3 normalWS = gProbeND.SampleLevel(gsamLinearWrap, uvs, 0);
    float3 viewDirWS = normalize(gEyePosW - posWS);
    float fresnell = 1-max(dot(normalWS, viewDirWS), 0);
    fresnell *= fresnell;

    float3 VXGIColor = TraceVoxelColor(posWS+fresnell*normalWS, sampleDir, isHit);
    float4 calc = gCurProbe.SampleLevel(gsamPointClamp, pin.TexC, 0);
    float3 res = lerp(VXGIColor, calc.rgb, calc.a*0.9);   
    return float4(res, max(isHit?1:0, calc.a));
}


