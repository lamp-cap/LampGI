#pragma once


class LampPSODescCreator
{
public:
    LampPSODescCreator(ComPtr<ID3D12Device> d3dDevice, std::shared_ptr<LampRootSignature> rootsig);
    LampPSODescCreator(const LampPSODescCreator& rhs) = delete;
    LampPSODescCreator& operator=(const LampPSODescCreator& rhs) = delete;
    ~LampPSODescCreator() = default;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildGraphicsPSODesc(
        std::wstring rootSignature,
        std::string ShaderPS,
        std::vector<DXGI_FORMAT> RTVFormats = { mBackBufferFormat },
        BOOL DepthWrite = false,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildGraphicsPSODesc(
        std::wstring rootSignature,
        std::string ShaderVS,
        std::string ShaderPS,
        std::vector<DXGI_FORMAT> RTVFormats = { mBackBufferFormat },
        CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        CD3DX12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildGraphicsPSODesc(
        std::wstring rootSignature,
        std::string ShaderVS,
        std::string ShaderPS,
        BOOL DepthWrite,
        std::vector<DXGI_FORMAT> RTVFormats = { mBackBufferFormat },
        CD3DX12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildGraphicsPSODesc(
        std::wstring rootSignature,
        std::string ShaderVS,
        std::string ShaderGS,
        std::string ShaderPS,
        std::vector<DXGI_FORMAT> RTVFormats = { mBackBufferFormat },
        CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        CD3DX12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);
    D3D12_COMPUTE_PIPELINE_STATE_DESC BuildComputePSODesc(
        std::wstring rootSignature,
        std::string ShaderCS,
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE
    );

private:
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    std::shared_ptr<LampRootSignature> mRS;
    std::unique_ptr<LampShader> mShader;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    static const DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    static const DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    const std::string FullScreenVSName = "fullScreenVS";
};
