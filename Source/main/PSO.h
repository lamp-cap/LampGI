#pragma once

#include "RootSignature.h"
#include "Shader.h"
#include "DescriptorHeap.h"

class LampPSO
{
public:
    LampPSO(Microsoft::WRL::ComPtr <ID3D12Device> d3dDevice, bool msaa, UINT msaaQuality);
    LampPSO(const LampPSO& rhs) = delete;
    LampPSO& operator=(const LampPSO& rhs) = delete;
    ~LampPSO() = default;

    void BuildGraphicsPSO(
            std::wstring name,
            std::wstring rootSignature,
            std::string ShaderPS,
            std::vector<DXGI_FORMAT> RTVFormats = { mBackBufferFormat },
            BOOL DepthWrite = false,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            CD3DX12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);
    void BuildGraphicsPSO(
        std::wstring name,
        std::wstring rootSignature,
        std::string ShaderVS,
        std::string ShaderPS,
        std::vector<DXGI_FORMAT> RTVFormats = { mBackBufferFormat },
        CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        CD3DX12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);
    void BuildGraphicsPSO(
        std::wstring name,
        std::wstring rootSignature,
        std::string ShaderVS,
        std::string ShaderPS,
        BOOL DepthWrite,
        std::vector<DXGI_FORMAT> RTVFormats = { mBackBufferFormat },
        CD3DX12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);
    void BuildGraphicsPSO(
        std::wstring name,
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
    void BuildComputePSO(
        std::wstring name,
        std::wstring rootSignature,
        std::string ShaderCS,
        D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE);

    ID3D12PipelineState* GetPSO(std::wstring name);
    ID3D12RootSignature* GetRootSignature(std::wstring name);

    void CreateRootSignature(D3D12_ROOT_SIGNATURE_DESC rootSigDesc, std::wstring name);
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 5> GetStaticSamplers1();

private:
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

    std::unique_ptr<LampRootSignature> mRootSig;
    std::unique_ptr<LampShader> mShader;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    static const DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    static const DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
    static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    const std::string FullScreenVSName = "fullScreenVS";
    BOOL m4xMsaaState;
    UINT m4xMsaaQuality;

    void BuildPSOs();
    void BuildPSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc, std::wstring name);
    void BuildPSO(D3D12_COMPUTE_PIPELINE_STATE_DESC desc, std::wstring name);
};