#include "fullScreenVS.hlsli"

TextureCube gCubeMap            : register(t0);

Texture2D gNormalMap            : register(t1);
Texture2D gDepthMap             : register(t2);

Texture3D gVoxelColor	        : register(t3);
Texture3D gVoxelMat	            : register(t4);
Texture3D gVoxelNormal	        : register(t5);

Texture3D gProbeSH0             : register(t6);
Texture3D gProbeSH1             : register(t7);
Texture3D gProbeSH2             : register(t8);

Texture2D gHiz                  : register(t9);

Texture2D gDiffuse              : register(t10);

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
    for(i=0; i<32; i++)
    {
        travelled += pow2(lod);
        current += offset * pow2(lod);
        if (abs(current.x)>_MaxDis.x || current.y>_MaxDis.y || current.y<_MinDis.y || abs(current.z)>_MaxDis.z) 
        {    
            // result = gCubeMap.SampleLevel(gsamLinearClamp, sampleDir, 0);
            // result *= result *0.7;
            result = 0.15;
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
float3 TraceScreenColor(float3 sampleDir, float3 posWS, float2 uv, float depth, float roughness, out bool isHit)
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
        count+=1;
        uv_s += offset * pow2(level);
        depth_n += dz * pow2(level);
        if( uv_s.x*uv_s.y<=0 || uv_s.x>1 || uv_s.y>1 || depth_n < 0.01|| depth_n>45) 
        {
            if(level<2)
            {
                break;
            }
            
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
        if(level < 6)level++;
    }

    if(isHit) 
    {
        if(depth_n - NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamPointClamp, uv_s, 0).r)<0.11f)
        {
            float3 normal_s = normalize(mul(gNormalMap.SampleLevel(gsamPointClamp, uv_s, 0).rgb, (float3x3)gInvView));
            if(dot(normal_s, -sampleDir)>0.1f)
            {
                return gDiffuse.SampleLevel(gsamLinearClamp, uv_s, log2(count)*roughness).rgb;
            }
        }
        isHit = false;
    }
    return color;
}

float4 PS(VertexOut pin) : SV_Target
{
    float pz = gDepthMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);
    if(pz > 30) return 0;

	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;

	float4 SceneColor = float4(0,0,0,0);

    if (abs(posWS.x)>_MaxDis.x || posWS.y>_MaxDis.y || posWS.y<_MinDis.y || abs(posWS.z)>_MaxDis.z)
        return SceneColor;

	float3 normalVS = gNormalMap.SampleLevel(gsamLinearWrap, pin.TexC, 0.0f).rgb;
    float3 normalWS = normalize(mul(normalVS, (float3x3)gInvView));
    float3 viewDirWS = normalize(gEyePosW - posWS);
    float3 reflectWS = reflect(-viewDirWS, normalWS);

    // float3 randomVec = RandomVector(posVS.xyz);
    bool isHit = false;
    float fresnell = 1-max(dot(normalWS, viewDirWS),0);
    // return fresnell;
    fresnell *= fresnell;
    float3 reflectColor = TraceScreenColor(reflectWS, posWS, pin.TexC, pz, 0, isHit);
    float a=0;
    if(!isHit)
    {
        reflectColor = TraceVoxelColor(posWS+fresnell*0.1f+normalWS, reflectWS, isHit);
        a = 0.5;
    }
    if(!isHit) a = 1;
    return float4(reflectColor, a);
}


