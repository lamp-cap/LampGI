#include "PSO.h"

LampPSO::LampPSO(Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice, bool msaa, UINT msaaQuality)
{
	md3dDevice = d3dDevice;
    mRootSig = std::make_unique<LampRootSignature>(d3dDevice);
    mShader = std::make_unique<LampShader>(d3dDevice);
    m4xMsaaState = msaa;
    m4xMsaaQuality = msaaQuality;
    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
	// BuildPSOs();
    // PSO for debug layer.
    std::vector<DXGI_FORMAT> rtvFormats = { mBackBufferFormat };
    BuildGraphicsPSO(L"debug", L"debug", "debugVS", "debugPS", false, rtvFormats);
}

void LampPSO::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gPSODesc = {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC cPSODesc = {};

    std::vector<DXGI_FORMAT> rtvFormats = { mBackBufferFormat };

    // PSO for opaque objects.
    CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    BuildGraphicsPSO(L"opaque", L"main", "standardVS", "opaquePS", rtvFormats, DepthStencilState);

    CD3DX12_RASTERIZER_DESC RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    // PSO for sky.
    // The camera is inside the sky sphere, so just turn off culling.
    RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    // Make sure the depth function is LESS_EQUAL and not just LESS.  
    // Otherwise, the normalized depth values at z = 1 (NDC) will the depth test if the depth buffer was cleared to 1.
    DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    BuildGraphicsPSO(L"sky", L"main", "skyVS", "skyPS", rtvFormats, DepthStencilState, RasterizerState);

    // PSO for shadow map pass.
    rtvFormats.clear(); 
    //RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    //RasterizerState.DepthBias = 100000;
    //RasterizerState.DepthBiasClamp = 0.0f;
    //RasterizerState.SlopeScaledDepthBias = 1.0f;
    //BuildGraphicsPSO(L"shadow_opaque", L"main", "shadowVS", "shadowOpaquePS", 
    //    rtvFormats, CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT), RasterizerState);

    // PSO for RSM pass.
    rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM ,DXGI_FORMAT_R8G8B8A8_UNORM }; 
    BuildGraphicsPSO(L"RSM", L"main", "RSMVS", "RSMPS", false, rtvFormats);

    // PSO for debug layer.
    rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
    BuildGraphicsPSO(L"debug", L"main", "debugVS", "debugPS", false, rtvFormats);

    // PSO for drawing normals.
    rtvFormats = { NormalMapFormat };
    BuildGraphicsPSO(L"drawNormals", L"main", "drawNormalsVS", "drawNormalsPS", rtvFormats);

    // PSO for voxelize.
    rtvFormats = { mBackBufferFormat };
    RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    RasterizerState.DepthClipEnable = false;
    RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
    DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    DepthStencilState.DepthEnable = false;
    DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    BuildGraphicsPSO(L"voxelize", L"voxelize", "VoxelizeVS", "VoxelizeGS", "VoxelizePS", 
        rtvFormats, DepthStencilState, RasterizerState);

    // PSO for drawing SSGI.
    BuildGraphicsPSO(L"ssgi", L"ssgi", "ssgiPS");

    // PSO for SSAO.
    rtvFormats = { AmbientMapFormat };
    BuildGraphicsPSO(L"ssao", L"ssao", "ssaoVS", "ssaoPS", false, rtvFormats);

    // PSO for SSAO blur.
    BuildGraphicsPSO(L"ssaoBlur", L"ssao", "ssaoBlurPS", rtvFormats);

    // PSO for sobel
    BuildComputePSO(L"sobel", L"sobel", "sobelCS");

    // PSO for mipmap
    BuildComputePSO(L"mipmap", L"mipmap", "mipmapCS");

    // PSO for mipmap
    BuildComputePSO(L"mipmap3D", L"mipmap", "mipmap3DCS");

    // PSO for hiz
    BuildComputePSO(L"hiz", L"mipmap", "hizCS");

    // PSO for compositing post process
    BuildGraphicsPSO(L"composite", L"sobel", "blitVS", "compositePS", false);

    // PSO for Cloud
    BuildGraphicsPSO(L"cloud", L"cloud", "cloudPS");

    // PSO for VXGI
    // BuildGraphicsPSO(L"vxgi", L"cloud", "vxgiPS");

    // PSO for compositing vxgi
    BuildGraphicsPSO(L"compositeVXGI", L"cloud", "compositeVXGIPS");

    // PSO for blit hiz.
    rtvFormats = { mBackBufferFormat, DXGI_FORMAT_R32_FLOAT };
    BuildGraphicsPSO(L"blitScreen", L"blit", "blitVS", "blitScreenPS", false, rtvFormats);

    BuildGraphicsPSO(L"blitColor", L"blit", "blitVS", "blitColorPS");

    // PSO for TAA.
    //BuildGraphicsPSO(L"taa", L"taa", "taaPS");

}

void LampPSO::BuildPSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc, std::wstring name)
{
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs[name])));
    mPSOs[name]->SetName(name.c_str());
}

void LampPSO::BuildPSO(D3D12_COMPUTE_PIPELINE_STATE_DESC desc, std::wstring name)
{
    ThrowIfFailed(md3dDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mPSOs[name])));
    mPSOs[name]->SetName(name.c_str());
}

// postProcess
void LampPSO::BuildGraphicsPSO(
    std::wstring name,
    std::wstring rootSignature,
    std::string ShaderPS,
    std::vector<DXGI_FORMAT> RTVFormats,
    BOOL DepthWrite,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType,
    CD3DX12_RASTERIZER_DESC RasterizerDesc,
    CD3DX12_BLEND_DESC BlendState,
    CD3DX12_DEPTH_STENCIL_DESC DepthStencilState,
    D3D12_PIPELINE_STATE_FLAGS Flags)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC mPsoDesc;
    ZeroMemory(&mPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    mPsoDesc.InputLayout = { nullptr, 0 };
    mPsoDesc.pRootSignature = mRootSig->GetRootSignature(rootSignature);
    mPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(FullScreenVSName)),
        mShader->GetSize(FullScreenVSName)
    };
    mPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderPS)),
        mShader->GetSize(ShaderPS)
    };
    mPsoDesc.RasterizerState = RasterizerDesc;
    mPsoDesc.BlendState = BlendState;
    mPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    mPsoDesc.DepthStencilState.DepthEnable = false;
    mPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    mPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    mPsoDesc.SampleMask = UINT_MAX;
    mPsoDesc.PrimitiveTopologyType = PrimitiveTopologyType;
    mPsoDesc.NumRenderTargets = (UINT)RTVFormats.size();
    for (size_t i = 0; i < RTVFormats.size(); ++i)
    {
        mPsoDesc.RTVFormats[i] = RTVFormats[i];
    }
    mPsoDesc.SampleDesc.Count = 1;
    mPsoDesc.SampleDesc.Quality = 0;

    BuildPSO(mPsoDesc, name);
}

void LampPSO::BuildGraphicsPSO(
    std::wstring name,
    std::wstring rootSignature,
    std::string ShaderVS,
    std::string ShaderPS,
    std::vector<DXGI_FORMAT> RTVFormats,
    CD3DX12_DEPTH_STENCIL_DESC DepthStencilState,
    CD3DX12_RASTERIZER_DESC RasterizerDesc,
    CD3DX12_BLEND_DESC BlendState,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType,
    D3D12_PIPELINE_STATE_FLAGS Flags)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC mPsoDesc;
    ZeroMemory(&mPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    mPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    mPsoDesc.pRootSignature = mRootSig->GetRootSignature(rootSignature);
    mPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderVS)),
        mShader->GetSize(ShaderVS)
    };
    mPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderPS)),
        mShader->GetSize(ShaderPS)
    };
    mPsoDesc.RasterizerState = RasterizerDesc;
    mPsoDesc.BlendState = BlendState;
    mPsoDesc.DepthStencilState = DepthStencilState;
    mPsoDesc.DSVFormat = mDepthStencilFormat;
    mPsoDesc.SampleMask = UINT_MAX;
    mPsoDesc.PrimitiveTopologyType = PrimitiveTopologyType;
    mPsoDesc.NumRenderTargets = (UINT)RTVFormats.size();
    if (RTVFormats.size() > 0)
    {
        for (size_t i = 0; i < RTVFormats.size(); ++i)
        {
            mPsoDesc.RTVFormats[i] = RTVFormats[i];
        }
    }
    else
    {
        mPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    }
    mPsoDesc.SampleDesc.Count = 1;
    mPsoDesc.SampleDesc.Quality = 0;

    BuildPSO(mPsoDesc, name);
}

void LampPSO::BuildGraphicsPSO(
    std::wstring name,
    std::wstring rootSignature,
    std::string ShaderVS,
    std::string ShaderPS,
    BOOL DepthWrite,
    std::vector<DXGI_FORMAT> RTVFormats,
    CD3DX12_RASTERIZER_DESC RasterizerDesc,
    CD3DX12_BLEND_DESC BlendState,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType,
    D3D12_PIPELINE_STATE_FLAGS Flags)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC mPsoDesc;
    ZeroMemory(&mPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    mPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    mPsoDesc.pRootSignature = mRootSig->GetRootSignature(rootSignature);
    mPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderVS)),
        mShader->GetSize(ShaderVS)
    };
    mPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderPS)),
        mShader->GetSize(ShaderPS)
    };
    mPsoDesc.RasterizerState = RasterizerDesc;
    mPsoDesc.BlendState = BlendState;
    mPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    mPsoDesc.DSVFormat = mDepthStencilFormat;
    if (!DepthWrite)
    {
        mPsoDesc.DepthStencilState.DepthEnable = false;
        mPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        mPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    }
    mPsoDesc.SampleMask = UINT_MAX;
    mPsoDesc.PrimitiveTopologyType = PrimitiveTopologyType;
    mPsoDesc.NumRenderTargets = (UINT)RTVFormats.size();
    if (RTVFormats.size() > 0)
    {
        for (size_t i = 0; i < RTVFormats.size(); ++i)
        {
            mPsoDesc.RTVFormats[i] = RTVFormats[i];
        }
    }
    else
    {
        mPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    }
    mPsoDesc.SampleDesc.Count = 1;
    mPsoDesc.SampleDesc.Quality = 0;

    BuildPSO(mPsoDesc, name);
}

void LampPSO::BuildGraphicsPSO(
    std::wstring name,
    std::wstring rootSignature,
    std::string ShaderVS,
    std::string ShaderGS,
    std::string ShaderPS,
    std::vector<DXGI_FORMAT> RTVFormats,
    CD3DX12_DEPTH_STENCIL_DESC DepthStencilState,
    CD3DX12_RASTERIZER_DESC RasterizerDesc,
    CD3DX12_BLEND_DESC BlendState,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType,
    D3D12_PIPELINE_STATE_FLAGS Flags)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC mPsoDesc;
    ZeroMemory(&mPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    mPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    mPsoDesc.pRootSignature = mRootSig->GetRootSignature(rootSignature);
    mPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderVS)),
        mShader->GetSize(ShaderVS)
    };
    mPsoDesc.GS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderGS)),
        mShader->GetSize(ShaderGS)
    };
    mPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderPS)),
        mShader->GetSize(ShaderPS)
    };
    mPsoDesc.RasterizerState = RasterizerDesc;
    mPsoDesc.BlendState = BlendState;
    mPsoDesc.DepthStencilState = DepthStencilState;
    mPsoDesc.DSVFormat = DepthStencilState.DepthEnable ? mDepthStencilFormat : DXGI_FORMAT_UNKNOWN;
    mPsoDesc.SampleMask = UINT_MAX;
    mPsoDesc.PrimitiveTopologyType = PrimitiveTopologyType;
    mPsoDesc.NumRenderTargets = (UINT)RTVFormats.size();
    for (size_t i = 0; i < RTVFormats.size(); ++i)
    {
        mPsoDesc.RTVFormats[i] = RTVFormats[i];
    }
    mPsoDesc.SampleDesc.Count = 1;
    mPsoDesc.SampleDesc.Quality = 0;

    BuildPSO(mPsoDesc, name);
}

void LampPSO::BuildComputePSO(
    std::wstring name,
    std::wstring rootSignature,
    std::string ShaderCS,
    D3D12_PIPELINE_STATE_FLAGS Flags)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC mPSODesc = {};
    mPSODesc.pRootSignature = mRootSig->GetRootSignature(rootSignature);
    mPSODesc.CS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderCS)),
        mShader->GetSize(ShaderCS)
    };
    mPSODesc.Flags = Flags;

    BuildPSO(mPSODesc, name);
}

ID3D12PipelineState* LampPSO::GetPSO(std::wstring name)
{
    return mPSOs[name].Get();
}
ID3D12RootSignature* LampPSO::GetRootSignature(std::wstring name)
{
    return mRootSig->GetRootSignature(name);
}

void LampPSO::CreateRootSignature(D3D12_ROOT_SIGNATURE_DESC rootSigDesc, std::wstring name)
{
    mRootSig->CreateRootSignature(rootSigDesc, name);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> LampPSO::GetStaticSamplers()
{
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.  

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
        0.0f,                             // mipLODBias
        8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        8);                                // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC shadow(
        6, // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
        0.0f,                               // mipLODBias
        16,                                 // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp,
        shadow
    };
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 5> LampPSO::GetStaticSamplers1()
{
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.  

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC shadow(
        4, // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
        0.0f,                               // mipLODBias
        16,                                 // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        shadow
    };
}
