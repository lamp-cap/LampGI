#ifndef GBUFFER_H
#define GBUFFER_H

#pragma once

#include "SceneRenderPass.h"

class GBuffer final : public SceneRenderPass
{
public:
    GBuffer(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs,
        std::shared_ptr<LampGeo> Scene);
    GBuffer(const GBuffer& rhs) = delete;
    GBuffer& operator=(const GBuffer& rhs) = delete;
    ~GBuffer() = default;

    static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static const DXGI_FORMAT HizMapFormat = DXGI_FORMAT_R32_FLOAT;
    static const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;

    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:
    void BuildRootSignatureAndPSO()override;
    void BuildResources()override;
    void BuildDescriptors()override;

private:

    const std::wstring mBaseColor = L"BaseColor";
    const std::wstring mNormal = L"Normal";
    const std::wstring mMetalicRoughness = L"MetalicRoughness";
};

#endif // GBuffer_H