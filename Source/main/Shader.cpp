#include "Shader.h"

LampShader::LampShader(Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice)
{
	md3dDevice = d3dDevice;
    LoadShaders();
}

LPVOID LampShader::GetShader(std::string name)
{
	return mShaders[name]->GetBufferPointer();
}

SIZE_T LampShader::GetSize(std::string name)
{
	return mShaders[name]->GetBufferSize();
}

void LampShader::LoadShaders()
{
    OutputDebugString(L"Begin Loading Shaders\n");
    LoadBlobs();
    OutputDebugString(L"Finish Loading Shaders\n");
}

void LampShader::LoadBlobs()
{
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Default_VS.cso", mShaders["standardVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Default_PS.cso", mShaders["opaquePS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Blit_PS.cso", mShaders["blitPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\fullScreen_VS.cso", mShaders["fullScreenVS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Shadows_VS.cso", mShaders["shadowVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Shadows_PS.cso", mShaders["shadowOpaquePS"].GetAddressOf()));
    // mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\RSM_VS.cso", mShaders["RSMVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\RSM_PS.cso", mShaders["RSMPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ShadowDebug_VS.cso", mShaders["debugVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ShadowDebug_PS.cso", mShaders["debugPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\DrawNormals_VS.cso", mShaders["drawNormalsVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\DrawNormals_PS.cso", mShaders["drawNormalsPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\GBuffer_VS.cso", mShaders["drawGBufferVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\GBuffer_PS.cso", mShaders["drawGBufferPS"].GetAddressOf()));

    // mShaders["drawDeferLightingVS"] = d3dUtil::CompileShader(L"Shaders\\DeferLighting.hlsl", nullptr, "VS", "vs_5_1");
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\DeferLighting_PS.cso", mShaders["DeferLightingPS"].GetAddressOf()));

    // mShaders["SSGILightingVS"] = d3dUtil::CompileShader(L"Shaders\\SSGI.hlsl", nullptr, "VS", "vs_5_1");
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\SSGI_PS.cso", mShaders["ssgiPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Voxelize_VS.cso", mShaders["VoxelizeVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Voxelize_GS.cso", mShaders["VoxelizeGS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Voxelize_PS.cso", mShaders["VoxelizePS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Ssao_VS.cso", mShaders["ssaoVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Ssao_PS.cso", mShaders["ssaoPS"].GetAddressOf()));

    // mShaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\SsaoBlur_PS.cso", mShaders["ssaoBlurPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Sky_VS.cso", mShaders["skyVS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Sky_PS.cso", mShaders["skyPS"].GetAddressOf()));

    // mShaders["compositeVS"] = d3dUtil::CompileShader(L"Shaders\\Composite.hlsl", nullptr, "VS", "vs_5_0");
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Composite_PS.cso", mShaders["compositePS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Sobel_CS.cso", mShaders["sobelCS"].GetAddressOf()));

    // mShaders["cloudVS"] = d3dUtil::CompileShader(L"Shaders\\Cloud.hlsl", nullptr, "VS", "vs_5_0");
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Cloud_PS.cso", mShaders["cloudPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\VXGI_PS.cso", mShaders["vxgiPS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\CompositeVXGI_PS.cso", mShaders["compositeVXGIPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\GenerateMip_CS.cso", mShaders["mipmapCS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\GenerateMip3D_CS.cso", mShaders["mipmap3DCS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\GenerateHiz_CS.cso", mShaders["hizCS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\TAA_PS.cso", mShaders["taaPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\WorldProbe_CS.cso", mShaders["WorldProbeCS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\UpdateProbe_PS.cso", mShaders["WorldProbePS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ProbeToSH_CS.cso", mShaders["ProbeToSHCS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ProbeGI_PS.cso", mShaders["ProbegiPS"].GetAddressOf()));

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ScreenProbeND_PS.cso", mShaders["ProbeNDPS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ScreenDI_PS.cso", mShaders["ScreenDIPS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\VoxelDI_PS.cso", mShaders["VoxelDIPS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ProbeDI_PS.cso", mShaders["ProbeDIPS"].GetAddressOf())); 
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\ScreenProbeToSH_CS.cso", mShaders["ScreenProbeCS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\Reflection_PS.cso", mShaders["ReflectionPS"].GetAddressOf()));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\cso\\CompositeDI_PS.cso", mShaders["CompositeDIPS"].GetAddressOf()));
}

void LampShader::CompileShaders()
{
    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
        "ALPHA_TEST", "1",
        NULL, NULL
    };

    mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["fullScreenVS"] = d3dUtil::CompileShader(L"Shaders\\fullScreenVS.hlsl", nullptr, "VS", "vs_5_1");

    mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mShaders["RSMVS"] = d3dUtil::CompileShader(L"Shaders\\RSM.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["RSMPS"] = d3dUtil::CompileShader(L"Shaders\\RSM.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["drawNormalsVS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["drawNormalsPS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["drawGBufferVS"] = d3dUtil::CompileShader(L"Shaders\\GBuffer.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["drawGBufferPS"] = d3dUtil::CompileShader(L"Shaders\\GBuffer.hlsl", nullptr, "PS", "ps_5_1");

    // mShaders["drawDeferLightingVS"] = d3dUtil::CompileShader(L"Shaders\\DeferLighting.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["DeferLightingPS"] = d3dUtil::CompileShader(L"Shaders\\DeferLighting.hlsl", nullptr, "PS", "ps_5_1");

    // mShaders["SSGILightingVS"] = d3dUtil::CompileShader(L"Shaders\\SSGI.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["ssgiPS"] = d3dUtil::CompileShader(L"Shaders\\SSGI.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["VoxelizeVS"] = d3dUtil::CompileShader(L"Shaders\\Voxelize.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["VoxelizePS"] = d3dUtil::CompileShader(L"Shaders\\Voxelize.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["VoxelizeGS"] = d3dUtil::CompileShader(L"Shaders\\Voxelize.hlsl", nullptr, "GS", "gs_5_1");

    mShaders["ssaoVS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["ssaoPS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

    // mShaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["ssaoBlurPS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

    // mShaders["compositeVS"] = d3dUtil::CompileShader(L"Shaders\\Composite.hlsl", nullptr, "VS", "vs_5_0");
    mShaders["compositePS"] = d3dUtil::CompileShader(L"Shaders\\Composite.hlsl", nullptr, "PS", "ps_5_0");
    mShaders["sobelCS"] = d3dUtil::CompileShader(L"Shaders\\Sobel.hlsl", nullptr, "SobelCS", "cs_5_0");

    // mShaders["cloudVS"] = d3dUtil::CompileShader(L"Shaders\\Cloud.hlsl", nullptr, "VS", "vs_5_0");
    mShaders["cloudPS"] = d3dUtil::CompileShader(L"Shaders\\Cloud.hlsl", nullptr, "PS", "ps_5_0");

    mShaders["vxgiPS"] = d3dUtil::CompileShader(L"Shaders\\VXGI.hlsl", nullptr, "PS", "ps_5_0");

    mShaders["mipmapCS"] = d3dUtil::CompileShader(L"Shaders\\GenerateMip.hlsl", nullptr, "CS", "cs_5_0");
    mShaders["mipmap3DCS"] = d3dUtil::CompileShader(L"Shaders\\GenerateMip3D.hlsl", nullptr, "CS", "cs_5_0");
    mShaders["hizCS"] = d3dUtil::CompileShader(L"Shaders\\GenerateHiz.hlsl", nullptr, "CS", "cs_5_0");

    mShaders["blitVS"] = d3dUtil::CompileShader(L"Shaders\\Blit.hlsl", nullptr, "VS", "vs_5_0");
    mShaders["blitPS"] = d3dUtil::CompileShader(L"Shaders\\Blit.hlsl", nullptr, "PS", "ps_5_0");

}