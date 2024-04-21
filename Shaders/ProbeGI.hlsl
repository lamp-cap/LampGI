#include "fullScreenVS.hlsli"
 
// Nonnumeric values cannot be added to a cbuffer.
TextureCube gCubeMap    : register(t0);
Texture2D gBaseColorMap : register(t1);
Texture2D gRMMap        : register(t2);
Texture2D gNormalMap    : register(t3);
Texture2D gDepthMap     : register(t4);

Texture3D gVoxelColor	: register(t5);

Texture3D gProbeSH0     : register(t6);
Texture3D gProbeSH1     : register(t7);
Texture3D gProbeSH2     : register(t8);

static const float3 _MaxDis = float3(12.5, 12.3046875, 12.5);
static const float3 _MinDis = float3(-12.5, -0.1953124, -12.5);

float3 positionToUVW(float3 position) {
    float3 uvw = position - _MinDis;
    uvw += 0.00001;
    uvw.x /= max(_MaxDis.x - _MinDis.x, 0.01);
    uvw.y /= max(_MaxDis.y - _MinDis.y, 0.01);
    uvw.z /= max(_MaxDis.z - _MinDis.z, 0.01);
    return uvw*float3(32, 16, 32);
}
float2 UVWToUV(uint3 uvw) {
    return float2(((uvw.y>>2)*32 + uvw.x)/128.0, ((uvw.y&3)*32 + uvw.z)/128.0);
}
float2 VectorToUV( float3 N )
{
    N.xy /= dot( 1, abs(N) );
    if( N.z <= 0 )
    {
        N.xy = ( 1 - abs(N.yx) ) * ( N.xy >= 0 ? float2(1,1) : float2(-1,-1) );
    }
    return (N.xy*0.5+0.5) * 0.75 + 0.125;
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
bool isLegal(float3 uvw, float3 uvwf, float3 normal)
{
    bool a = gVoxelColor.SampleLevel(gsamLinearClamp, uvw*float3(0.03125, 0.0625, 0.03125), 3).a>0;
    bool b = dot(uvw+0.5 - uvwf, normal)>-0.8;
    return a&&b;
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

    float3 normalVS = gNormalMap.SampleLevel(gsamLinearWrap, pin.TexC, 0.0f).rgb;
    float3 normalWS = normalize(mul(normalVS, (float3x3)gInvView));
    float3 uvw = positionToUVW(posWS);
    // return float4(GetSH(uvw, normalWS)*4, 1);
    uint ub = floor(uvw.x);
    uint ut = ub + 1;
    uint vb = floor(uvw.y);
    uint vt = vb + 1;
    uint wb = floor(uvw.z);
    uint wt = wb + 1;
    uint3 uvw0 = uint3(ub, vb, wb);
    uint3 uvw1 = uint3(ut, vb, wb);
    uint3 uvw2 = uint3(ub, vt, wb);
    uint3 uvw3 = uint3(ut, vt, wb);
    uint3 uvw4 = uint3(ub, vb, wt);
    uint3 uvw5 = uint3(ut, vb, wt);
    uint3 uvw6 = uint3(ub, vt, wt);
    uint3 uvw7 = uint3(ut, vt, wt);

    float3 w = uvw - uvw0;
    float x = 1-w.x;
    float y = 1-w.y;
    float z = 1-w.z;
    float weight = 0;

    float3 sh0 = GetSH(uvw0, normalWS)*x*y*z;
    if(isLegal(uvw0, uvw, normalWS)) weight += x*y*z;
    else sh0 = 0;

    float3 sh1 = GetSH(uvw1, normalWS)*(1-x)*y*z;
    if(isLegal(uvw1, uvw, normalWS)) weight += (1-x)*y*z;
    else sh1 = 0;

    float3 sh2 = GetSH(uvw2, normalWS)*x*(1-y)*z;
    if(isLegal(uvw2, uvw, normalWS)) weight += x*(1-y)*z;
    else sh2 = 0;

    float3 sh3 = GetSH(uvw3, normalWS)*(1-x)*(1-y)*z;
    if(isLegal(uvw3, uvw, normalWS)) weight += (1-x)*(1-y)*z;
    else sh3 = 0;

    float3 sh4 = GetSH(uvw4, normalWS)*x*y*(1-z);
    if(isLegal(uvw4, uvw, normalWS)) weight += x*y*(1-z);
    else sh4 = 0;

    float3 sh5 = GetSH(uvw5, normalWS)*(1-x)*y*(1-z);
    if(isLegal(uvw5, uvw, normalWS)) weight += (1-x)*y*(1-z);
    else sh5 = 0;

    float3 sh6 = GetSH(uvw6, normalWS)*x*(1-y)*(1-z);
    if(isLegal(uvw6, uvw, normalWS)) weight += x*(1-y)*(1-z);
    else sh6 = 0;

    float3 sh7 = GetSH(uvw7, normalWS)*(1-x)*(1-y)*(1-z);
    if(isLegal(uvw7, uvw, normalWS)) weight += (1-x)*(1-y)*(1-z);
    else sh7 = 0;
    
    // return weight>0?1:0;
    weight = max(weight, 0.01);
    float3 res = sh0 + sh1 + sh2 + sh3 + sh4 + sh5 + sh6 + sh7;
    // return float4(res, 1);
    res /= weight;
    // return float4(res, 1);


    return float4(res, 1);
}
