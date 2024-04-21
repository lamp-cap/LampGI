#pragma once

#include "RenderPass.h"

class ScreenRenderPass : public RenderPass
{
public:
    ScreenRenderPass(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs,
        std::wstring name,
        UINT numSRV);
    ScreenRenderPass(const ScreenRenderPass& rhs) = delete;
    ScreenRenderPass& operator=(const ScreenRenderPass& rhs) = delete;
    ~ScreenRenderPass() = default;

protected:
    void DrawFullScreen(ID3D12GraphicsCommandList* cmdList);
};
