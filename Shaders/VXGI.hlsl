#include "fullScreenVS.hlsli"
 
// Nonnumeric values cannot be added to a cbuffer.
TextureCube gCubeMap    : register(t0);
Texture2D gBaseColorMap : register(t1);
Texture2D gRMMap        : register(t2);
Texture2D gNormalMap    : register(t3);
Texture2D gDepthMap     : register(t4);
Texture3D gVoxelColor	: register(t5);
Texture3D gVoxelMat	    : register(t6);
Texture3D gVoxelNormal	: register(t7);

Texture3D gProbeSH0     : register(t8);
Texture3D gProbeSH1     : register(t9);
Texture3D gProbeSH2     : register(t10);

static const float3 _MaxDis = float3(12.5, 12.3046875, 12.5);
static const float3 _MinDis = float3(-12.5, -0.1953124, -12.5);

float4 sample3DColorLod(float3 position, int lod) {
	float3 uvw = position - _MinDis;
	uvw.x *= 0.04;
	uvw.y *= 0.08;
	uvw.z *= 0.04;
    uvw += 0.00001;
    return gVoxelColor.SampleLevel(gsamLinearClamp, uvw, lod);
}
float4 sampleNormal3DLod(float3 position, int lod) {
	float3 uvw = position - _MinDis;
	uvw.x *= 0.04;
	uvw.y *= 0.08;
	uvw.z *= 0.04;
    uvw += 0.00001;
    return gVoxelNormal.SampleLevel(gsamLinearClamp, uvw, lod);
}
float4 sampleMatData3DLod(float3 position, int lod) {
	float3 uvw = position - _MinDis;
	uvw.x *= 0.04;
	uvw.y *= 0.08;
	uvw.z *= 0.04;
    uvw += 0.00001;
    return gVoxelMat.SampleLevel(gsamPointClamp, uvw, lod);
}
float Random(float3 i)
{
    float f = dot(float2(12.9898, 78.233), float2(i.x + gTotalTime*0.01*i.z, i.y*gTotalTime));
    return frac(43758.5453 * sin(f));
}
int log2(int x)
{
    int rst = 0;
    if (x & 240) rst +=  4, x >> 4;
    if (x & 12) rst +=  2, x >> 2;
    if (x & 2) rst +=  1;
    return rst;
}
float Pow5(float s)
{
    float s2 = s * s;
    return s2 * s2 * s;
}
int pow2(uint x)
{
    return x>7?255 : 1 << x;
}
float3 positionToUVW(float3 position) {
    float3 uvw = position - _MinDis;
    uvw += 0.00001;
    uvw.x /= max(_MaxDis.x - _MinDis.x, 0.01);
    uvw.y /= max(_MaxDis.y - _MinDis.y, 0.01);
    uvw.z /= max(_MaxDis.z - _MinDis.z, 0.01);
    return uvw*float3(32, 16, 32);
}
float4 GetY01(float3 p){
    return float4(0.28209479, 0.488602512 * p.y, 0.488602512 * p.z, 0.488602512 * p.x);
}
float3 GetSH(uint3 uvw, float3 normal)
{
    // float2 uv = UVWToUV(uvw);
    // float2 off = VectorToUV(normal)/128.0;
    // return gProbeMap.SampleLevel(gsamLinearClamp, uv + off, 1).rgb;
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
float3 trace(float3 start, float3 sampleDir, float3 normal, float roughness, float ambient)
{
    float axis = max(abs(sampleDir.z), max(abs(sampleDir.x), abs(sampleDir.y))); 
    float3 offset = sampleDir / axis;
    offset *= 0.09765625;

    float k = Random(start*sampleDir);
    float3 current = start + offset*(k*0.5);
    float4 result = 0;
    // float len = length(offset);
    float travelled = 0;
    int lod = 0;
    int count = 48;
    int i=0;
    [loop]
    for(i=0; i<count; i++)
    {
        travelled += pow2(max(lod, 0));
        current += offset * pow2(max(lod, 0));
        if (abs(current.x)>_MaxDis.x || current.y>_MaxDis.y || current.y<_MinDis.y || abs(current.z)>_MaxDis.z) 
            break;
        
        // lod = max(log2(floor(travelled*roughness)), 0);
        float4 temp = sample3DColorLod(current, lod);
        if(temp.a>0)
            result += float4(temp.rgb, temp.a);
        if(result.a>0.95) 
        {
            float3 uvw = positionToUVW(current)*float3(32, 16, 32);
            result += float4(saturate(GetSH(uvw, sampleDir)*(temp.rgb*0.7+0.3)), 0);
                        
            break;
        }
    }
    float3 sky = 0;
    // return i/64.0;
    float radiance = sqrt(max(dot(sampleDir, normal),0));
    // return radiance;
    return lerp(sky, result.rgb, result.a);
}
float3 traceReflect(float3 start, float3 sampleDir, float3 normal)
{
    float axis = max(abs(sampleDir.z), max(abs(sampleDir.x), abs(sampleDir.y))); 
    float3 offset = sampleDir / axis;
    offset *= 0.09765625;

    float k = Random(start);
    float3 current = start + offset*(k+0.5);
    float4 result = 0;
    float3 normalS = 0;
    float4 mat = 0;
    // float len = length(offset);
    float travelled = 0;
    int lod = 0;
    int i=0;
    [loop]
    for(i=0; i<64; i++)
    {
        travelled += 1;
        current += offset;
        if (abs(current.x)>_MaxDis.x || current.y>_MaxDis.y || current.y<_MinDis.y || abs(current.z)>_MaxDis.z) 
            break;
        
        float4 temp = sample3DColorLod(current, lod);
        if(temp.a>0)
        {
            result = temp;
            result.rgb/=result.a;
            float4 normal_s = sampleNormal3DLod(current, 0);
            normalS = normal_s.rgb/normal_s.a*2-1;
            mat = sampleMatData3DLod(current, 0);
            break;
        }
    }
    float3 sky = gCubeMap.Sample(gsamLinearWrap, sampleDir).rgb;
    float3 light = normalize(float3(0.5, 1, 0.8));
    // sky = pow(sky, 2);
    // return i/64.0;
    return result.a>0?dot(light, normalS)*result.rgb:sky;
}
half3 FresnelSchlick(half VoH, half3 F0)
{
    half s = 1.0 - VoH;
    return F0 + (1.0 - F0) * Pow5(s);
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

float4 PS(VertexOut pin) : SV_Target
{
    float pz = gDepthMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);

	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;

	float4 SceneColor = float4(0,0,0,0);

    if (abs(posWS.x)>_MaxDis.x || posWS.y>_MaxDis.y || posWS.y<_MinDis.y || abs(posWS.z)>_MaxDis.z)
        return SceneColor;
    // return sqrt(sample3DColorLod(posWS, 3));

	float3 normalVS = gNormalMap.SampleLevel(gsamLinearWrap, pin.TexC, 0.0f).rgb;
    float3 normalWS = normalize(mul(normalVS, (float3x3)gInvView));
    float3 viewDirWS = normalize(posWS - gEyePosW);
    float3 reflectWS = reflect(viewDirWS, normalWS);

    float3 noise = float3(Random(posWS+0.5), Random(normalWS), Random(reflectWS));

    float3 o = RandomVector(noise);
    float3 u = normalize(cross(normalWS, o));
    float3 v = normalize(cross(normalWS, u));

    float k = 1.1 - o.x*1.05;
    k = k*abs(k);

    float3 sampleDir[6] = {
        normalize(normalWS*k+u), 
        normalize(normalWS*k-u), 
        normalize(normalWS*k+0.866*v+0.5*u), 
        normalize(normalWS*k-0.866*v-0.5*u), 
        normalize(normalWS*k+0.866*v-0.5*u), 
        normalize(normalWS*k-0.866*v+0.5*u) 
    };

    float3 indirectColor = 0;
    [unroll]
    for(int i=0; i<6; i++)
    {
        float fresnell = 1 - max(dot(normalWS, sampleDir[i]), 0);
        fresnell *= fresnell;
        fresnell = min(fresnell+0.04, 1);
        
        float3 current = posWS + normalWS*fresnell*fresnell;
        indirectColor += trace(current, sampleDir[i], normalWS, 0, 0.6);
    }
    indirectColor *= 0.31831;
    // return float4(indirectColor,1);

    float4 rm = gRMMap.SampleLevel(gsamPointClamp, pin.TexC, 0);
    float metallic = rm.r;
    float roughness = rm.a*rm.a;
    float3 base = gBaseColorMap.SampleLevel(gsamPointClamp, pin.TexC, 0).rgb;
    //return float4(rm.rh, 1);
    float fresnell = 1 - max(dot(normalWS, reflectWS), 0);
    fresnell *= fresnell;
    fresnell = saturate(fresnell+0.02+metallic);
    float3 current = posWS + normalWS*fresnell*fresnell*2;
    float3 reflectColor = trace(current, reflectWS, normalWS, roughness, 1);
    reflectColor = lerp(reflectColor, reflectColor*base, metallic);
    // return float4(pow(reflectColor,0.5),1);
    float3 H = normalWS;
    float NoV = max(dot(normalWS, -viewDirWS), 0);
    // float D = GTR2(NoH, roughness);
    float3 F0 = rm.rgb;
    float3 F = (FresnelSchlick(NoV, F0) - (roughness*0.7+0.3)*0.4);
    // half k = Kibl(roughness);
    // float G = GeometrySmith(NoV, NoL, k);
    half3 kd = 1-F;
    float3 diffuse = base * indirectColor;

    // float2 lut = SAMPLE_TEXTURE2D(_LutTex, sampler_LutTex, float2(NoV, roughness)).rg;
    float2 lut = F.xy;
    float3 specular = reflectColor;
    float3 res = lerp(diffuse, specular, saturate(F.r))*4;
    // diffuse*(1-F) + scene*(1-F) + F*specular;
	return float4(res, saturate(1-F.r));
}
