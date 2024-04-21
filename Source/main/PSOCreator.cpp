#include "PSOCreator.h"

LampPSODescCreator::LampPSODescCreator(ComPtr<ID3D12Device> d3dDevice, std::shared_ptr<LampRootSignature> rootsig)
{
    md3dDevice = d3dDevice;
    mRS = rootsig;
    mShader = std::make_unique<LampShader>(d3dDevice);
    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

// postProcess
D3D12_GRAPHICS_PIPELINE_STATE_DESC
LampPSODescCreator::BuildGraphicsPSODesc(
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
    mPsoDesc.pRootSignature = mRS->GetRootSignature(rootSignature);
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
    return mPsoDesc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC
LampPSODescCreator::BuildGraphicsPSODesc(
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
    mPsoDesc.pRootSignature = mRS->GetRootSignature(rootSignature);
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
    return mPsoDesc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC
LampPSODescCreator::BuildGraphicsPSODesc(
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
    mPsoDesc.pRootSignature = mRS->GetRootSignature(rootSignature);
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
    return mPsoDesc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC
LampPSODescCreator::BuildGraphicsPSODesc(
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
    mPsoDesc.pRootSignature = mRS->GetRootSignature(rootSignature);
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
    return mPsoDesc;
}

D3D12_COMPUTE_PIPELINE_STATE_DESC
LampPSODescCreator::BuildComputePSODesc(
    std::wstring rootSignature,
    std::string ShaderCS,
    D3D12_PIPELINE_STATE_FLAGS Flags)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC mPSODesc = {};
    mPSODesc.pRootSignature = mRS->GetRootSignature(rootSignature);
    mPSODesc.CS =
    {
        reinterpret_cast<BYTE*>(mShader->GetShader(ShaderCS)),
        mShader->GetSize(ShaderCS)
    };
    mPSODesc.Flags = Flags;
    return mPSODesc;
}
