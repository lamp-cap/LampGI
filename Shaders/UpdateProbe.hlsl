#include "fullScreenVS.hlsli"

TextureCube gCubeMap    : register(t0);
Texture2D gHisWProbe    : register(t1);

Texture3D gVoxelColor	: register(t2);
Texture3D gVoxelMat	    : register(t3);
Texture3D gVoxelNormal	: register(t4);

Texture3D gProbeSH0     : register(t5);
Texture3D gProbeSH1     : register(t6);
Texture3D gProbeSH2     : register(t7);

static const float3 _MaxDis = float3(12.5, 12.3046875, 12.5);
static const float3 _MinDis = float3(-12.5, -0.1953124, -12.5);

float Random(float3 i)
{
    float f = dot(float2(12.9898, 78.233), float2(i.x + i.z + gTotalTime, i.y + gTotalTime*0.1));
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
uint3 m2dTo3d(uint2 uv)
{
    uint x = uv.x>>3;
    uint y = uv.y>>3;
    uint u = x & 31;
    uint w = y & 31;
    uint v = (x >> 5)* 4 + (y >> 5);
    return uint3(u, v, w);
}
float3 positionToUVWf(float3 position) {
    float3 uvw = position - _MinDis;
    uvw += 0.00001;
    uvw.x /= max(_MaxDis.x - _MinDis.x, 0.01);
    uvw.y /= max(_MaxDis.y - _MinDis.y, 0.01);
    uvw.z /= max(_MaxDis.z - _MinDis.z, 0.01);
    return uvw;
}
float3 uvToVector3(float2 uv)
{
    float3 N = float3( uv, 1 - abs(uv.x)-abs(uv.y) );
    if( N.z < 0 )
    {
        N.xy = ( 1 - abs(N.yx) ) * ( N.xy >= 0 ? float2(1,1) : float2(-1,-1) );
    }
    return N;
}
float4 GetY01(float3 p) {
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
        float3(sh0.w, sh1.w, sh2.w) * shc01.w;
    return res;
}
float4 sample3DColorLod(float3 position, int lod) {
    float3 uvw = positionToUVWf(position);
    lod = min(lod, 6);
    return gVoxelColor.SampleLevel(gsamLinearClamp, uvw, lod);
}
float3 trace(uint3 start, float3 sampleDir)
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
    for(i=0; i<32; i++)
    {
        travelled += pow2(lod);
        current += offset * pow2(lod);
        if (abs(current.x)>_MaxDis.x || current.y>_MaxDis.y || current.y<_MinDis.y || abs(current.z)>_MaxDis.z) 
        {
            // result = gCubeMap.SampleLevel(gsamLinearClamp, sampleDir, 0);
            // result *= result;
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
                
                float3 normal = gVoxelNormal[uint3(current)].rgb*2-1;
                result.rgb += saturate(GetSH(uvw, sampleDir));
                result *= (dot(-sampleDir, normal) > 0.05 ? 1 : 0);
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

    return result.rgb;
}

float4 PS(VertexOut i) : SV_Target
{
    int2 uvi = i.TexC * 1024;
    uint3 uvw = m2dTo3d(uvi);
    if( !(gVoxelColor.SampleLevel(gsamLinearClamp, uvw/float3(32, 16, 32), 3).a>0 ) ) {  return 0; }
    else 
    {
        float3 history = gHisWProbe[uvi].rgb;
        float lumin = dot(history, float3(0.2126729f, 0.7151522f, 0.0721750f));
        if (lumin < 0.01) history = 0;
        else if (lumin < 0.04) history *= 0.7;
        float2 uv = float2((uvi.x & 7)/8.0, (uvi.y & 7)/8.0);

        float3 dir = uvToVector3(uv*2-1);
        float3 rand = RandomVector(float3(i.TexC, i.TexC.x));
        dir = normalize(dir+rand*0.07);
        float3 res = trace(uvw*8, dir);
        float3 color = lerp(history, res, 0.1);
        return float4(color, 1);
    }
}