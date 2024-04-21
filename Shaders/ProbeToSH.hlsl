
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

Texture2D<float4> _ProbeTex    : register(t0);

RWTexture3D<float4> _SH0     : register(u0);
RWTexture3D<float4> _SH1     : register(u1);
RWTexture3D<float4> _SH2     : register(u2);

SamplerState gsamLinearClamp    : register(s0);
SamplerState gsamPointClamp     : register(s1);

groupshared float3 shcGroup[8 * 8 * 4];

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
float4 GetY01(float3 p){
    return float4(0.28209479, 0.488602512 * p.y, 0.488602512 * p.z, 0.488602512 * p.x);
}

[numthreads( 8, 8, 1 )]
void CS( uint GroupIndex : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
    float4 color = _ProbeTex[DTid.xy];
    uint3 uvw = m2dTo3d(DTid.xy);
    float k = 3.14159265 * 0.0625;
    float4 shc01 = 0;

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
        shc01 = GetY01(dir);
    }
    else
    {
        color = 0;
    }
    uint groupOffset = GroupIndex * 4;
    shcGroup[groupOffset + 0] = color.rgb * k * shc01.x;
    shcGroup[groupOffset + 1] = color.rgb * k * shc01.y;
    shcGroup[groupOffset + 2] = color.rgb * k * shc01.z;
    shcGroup[groupOffset + 3] = color.rgb * k * shc01.w;

    GroupMemoryBarrierWithGroupSync();
    for(uint i = 32; i > 0; i >>= 1){
        if(GroupIndex < i)
        {
            for(uint offset = 0; offset < 4; offset ++){
                shcGroup[GroupIndex * 4 + offset] += shcGroup[(GroupIndex + i) * 4 + offset];
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (GroupIndex == 0)
    {
        _SH0[uvw] = float4(shcGroup[0].x, shcGroup[1].x,shcGroup[2].x,shcGroup[3].x);
        _SH1[uvw] = float4(shcGroup[0].y, shcGroup[1].y,shcGroup[2].y,shcGroup[3].y);
        _SH2[uvw] = float4(shcGroup[0].z, shcGroup[1].z,shcGroup[2].z,shcGroup[3].z);
    }
}