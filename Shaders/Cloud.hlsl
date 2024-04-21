#include "./fullScreenVS.hlsli"

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gOffScreenMap : register(t0);
TextureCube gCubeMap    : register(t1);
Texture2D gNormalMap    : register(t2);
Texture2D gDepthMap     : register(t3);
Texture2D gRandomVecMap : register(t4);
Texture3D gVoxelMap		: register(t5);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

half2 RayBoxDst(half3 boundsMin, half3 boundsMax, half3 rayOrigin, half3 rayDir)
{
    // about ray box algorithm 
    // https://jcgt.org/published/0007/03/04/ 

    half3 t0 = (boundsMin - rayOrigin) / rayDir;
    half3 t1 = (boundsMax - rayOrigin) / rayDir;
    half3 tmin = min(t0, t1);
    half3 tmax = max(t0, t1);

    half dstA = max(max(tmin.x, tmin.y), tmin.z);
    half dstB = min(tmax.x, min(tmax.y, tmax.z));

    half dstToBox = max(0, dstA);
    half dstInsideBox = max(0, dstB - dstToBox);
    return half2(dstToBox, dstInsideBox);
}

float4 sampleDensity(half3 position, half3 MinDis, half3 MaxDis) {
	half3 uvw = (position - MinDis);
	uvw.x *= rcp(MaxDis.x - MinDis.x);
	uvw.y *= rcp(MaxDis.y - MinDis.y);
	uvw.z *= rcp(MaxDis.z - MinDis.z);
	// uvw.y = 1-uvw.y;
    float level = floor(abs(gTotalTime%14-7));
	return gVoxelMap.SampleLevel(gsamPointWrap, uvw, 0);
}

float4 SampleTexture3D(float3 worldPosition)
{
    half3 rayPos = gEyePosW;
    half3 ViewDirWS = worldPosition - rayPos;
    half3 rayDir = normalize(ViewDirWS);

    // 射线检测包围盒算法，rayBoxDst返回的是一个half2(dstToBox, dstInsideBox)
    // dstToBox表示从摄像机出发，到包围盒的距离; destInsideBox表示包围盒中从这头到那头的距离
    half3 MinDis = half3(-12.5, -3.125, -12.5);
    half3 MaxDis = half3(12.5, 9.375, 12.5);
    half2 rayBoxInfo = RayBoxDst(MinDis, MaxDis, rayPos, rayDir);
    half dstToBox = rayBoxInfo.x;
    half dstInsideBox = rayBoxInfo.y;
	
    half dstToOpaque = length(ViewDirWS)*2;
    half dstLimit = dstInsideBox;

    int stepCount = 64;
	
    half stepSize = dstInsideBox / stepCount;
    half3 stepVec = rayDir * stepSize;
    
    half3 currentPoint = rayPos + dstToBox * rayDir;
	half3 startPoint = currentPoint;
	currentPoint += stepVec * frac(gTotalTime * dot(worldPosition, half3(rayDir.x, gTotalTime, rayDir.z*gTotalTime)));
    half dstTravelled = 0;

    half lightIntensity = 0;
	half4 color = 0;
	bool isHit = false;
	[unroll]
    for(int i = 0; i < stepCount; i ++){
        if(dstTravelled < dstLimit)
        {
            float4 sampleColor = sampleDensity(currentPoint, MinDis, MaxDis);
            color.rgb += sampleColor.rgb * sampleColor.a;
            color.a += sampleColor.a;
            if(color.a > 0.99)
            {
				isHit = true;
                break;
            }
			else 
			{
                currentPoint += stepVec;
                dstTravelled += stepSize;
            }
        }
        else {
            break;
        }
    }
	return color.a>0 ? color / color.a : 0;
}

float4 PS(VertexOut pin) : SV_Target
{
    float pz = gDepthMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);
	float4 posVS = float4((pz/pin.PosV.z)*pin.PosV, 1);
    float3 posWS = mul(posVS, gInvView).xyz;
	
	float4 Cloudcolor = SampleTexture3D(posWS);
	float4 SceneColor = gOffScreenMap.Sample(gsamLinearClamp, pin.TexC);

	return Cloudcolor;
}
