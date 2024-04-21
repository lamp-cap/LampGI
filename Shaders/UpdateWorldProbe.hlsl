
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

Texture3D<float4> VoxelColor    : register(t0);
Texture3D<float4> VoxelNormal   : register(t1);
Texture3D<float4> VoxelORM      : register(t2);

Texture3D<float4> gProbeSH0     : register(t3);
Texture3D<float4> gProbeSH1     : register(t4);
Texture3D<float4> gProbeSH2     : register(t5);

RWTexture2D<float4> OutMip0     : register(u0);
RWTexture2D<float4> OutMip1     : register(u1);
RWTexture2D<float4> OutMip2     : register(u2);

SamplerState gsamLinearClamp    : register(s0);
SamplerState gsamPointClamp     : register(s1);


groupshared float gs_R[64];
groupshared float gs_G[64];
groupshared float gs_B[64];

void StoreColor( uint Index, float3 Color )
{
    gs_R[Index] = Color.r;
    gs_G[Index] = Color.g;
    gs_B[Index] = Color.b;
}
float3 LoadColor( uint Index )
{
    return float3( gs_R[Index], gs_G[Index], gs_B[Index]);
}
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
uint3 m2dTo3d(uint2 uv)
{
    uint x = uv.x>>3;
    uint y = uv.y>>3;
    uint u = x & 31;
    uint w = y & 31;
    uint v = (x >> 5)* 4 + (y >> 5);
    return uint3(u, v, w);
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
float3 trace(uint3 start, float3 sampleDir)
{
    float axis = max(abs(sampleDir.z), max(abs(sampleDir.x), abs(sampleDir.y))); 
    float3 offset = sampleDir / axis;

    // float k = Random(start);
    float3 current = start + offset;
    // current.y += 9;
    float4 result = 0;
    int i=0;
    [loop]
    for(i=0; i<48; i++)
    {
        current += offset;
        if (abs(current.x-128)>128 || abs(current.y-64)>64 || abs(current.z-128)>128) break;
        float4 temp = VoxelColor[uint3(current)];
        if(temp.a>0.5){
            float3 normal = VoxelNormal[uint3(current)].rgb*2-1;
            result = float4(temp.rgb + GetSH(uint3(current), normal), 1) * (dot(-sampleDir, normal) > 0.05 ? 1 : 0);
            break;
        }
    }
    float3 sky = float3(0.2, 0.3, 0.4);
    sky = 0;
    // return radiance;
    return lerp(sky, result.rgb, 1);
}

[numthreads( 8, 8, 1 )]
void CS( uint GroupIndex : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
    uint3 uvw = m2dTo3d(DTid.xy);
    float4 color = VoxelColor.SampleLevel(gsamLinearClamp, uvw * float3(0.03125, 0.0625, 0.03125), 3);


    float3 history = OutMip0[DTid.xy].rgb;
    if (dot(history, float3(0.2126729f, 0.7151522f, 0.0721750f)) < 0.018) history = 0;
    float3 res = 0;
    float alpha = color.a > 0 ? 1 : 0;

    if( color.a>0 ) 
    {
        float2 uv = float2((DTid.x & 7)/8.0, (DTid.y&7)/8.0);
        uv = uv*2-1;
        // uv *= 1.333333333f;
        // bool x = abs(uv.x)>1;
        // bool y = abs(uv.y)>1;
        // if(x&&!y) uv*=float2(0.75, -1);
        // if(y&&!x) uv*=float2(-1, 0.75);
        // if(x&&y) uv*=float2(-0.75, -0.75);
        float3 dir = uvToVector3(uv);
        float3 rand = RandomVector(uvw);
        res = trace(uvw*8, normalize(dir+rand*0.15));

        // res = lerp(history, res, 0.1);
    }
    OutMip0[DTid.xy] = float4(res, alpha);
    /*
    StoreColor(GroupIndex, res);

    GroupMemoryBarrierWithGroupSync();
    // 001001
    if (color.a>0 && (GroupIndex & 0x9) == 0)
    {
        float3 res2 = LoadColor(GroupIndex + 0x01);
        float3 res3 = LoadColor(GroupIndex + 0x08);
        float3 res4 = LoadColor(GroupIndex + 0x09);
        res = 0.25*(res + res2 + res3 + res4);

        OutMip1[DTid.xy / 2] = float4(res, 1);
        StoreColor(GroupIndex, res);
    }

    GroupMemoryBarrierWithGroupSync();
    // 011011
    if (color.a>0 && (GroupIndex & 0x1B) == 0)
    {
        float3 res2 = LoadColor(GroupIndex + 0x02);
        float3 res3 = LoadColor(GroupIndex + 0x10);
        float3 res4 = LoadColor(GroupIndex + 0x12);
        res = 0.25*(res + res2 + res3 + res4);

        OutMip2[DTid.xy / 4] = float4(res, 1);
    }
    */
}