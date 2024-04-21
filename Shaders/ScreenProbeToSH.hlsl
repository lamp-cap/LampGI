Texture2D<float4> _ProbeTex     : register(t0);
Texture2D<float4> _ProbeND      : register(t1);

RWTexture2D<float4> _SH0        : register(u0);
RWTexture2D<float4> _SH1        : register(u1);
RWTexture2D<float4> _SH2        : register(u2);

SamplerState gsamLinearClamp    : register(s0);
SamplerState gsamPointClamp     : register(s1);

groupshared float3 shcGroup[8 * 8 * 4];

float4 GetY01(float3 p){
    return float4(0.28209479, 0.488602512 * p.y, 0.488602512 * p.z, 0.488602512 * p.x);
}

[numthreads( 8, 8, 1 )]
void CS( uint GroupIndex : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
    float4 color = _ProbeTex[DTid.xy];
    uint2 uv = DTid.xy >> 3;
    float k = 3.14159265 * 0.0625;
    float4 shc01 = 0;

    if( color.a>0 ) 
    {
        float3 dir = _ProbeND[DTid.xy];
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
        float4 sh0 = _SH0[uv];
        float4 sh1 = _SH1[uv];
        float4 sh2 = _SH2[uv];

        _SH0[uv] = lerp(sh0, float4(shcGroup[0].x, shcGroup[1].x,shcGroup[2].x,shcGroup[3].x), 0.1f);
        _SH1[uv] = lerp(sh1, float4(shcGroup[0].y, shcGroup[1].y,shcGroup[2].y,shcGroup[3].y), 0.1f);
        _SH2[uv] = lerp(sh2, float4(shcGroup[0].z, shcGroup[1].z,shcGroup[2].z,shcGroup[3].z), 0.1f);
    }
}