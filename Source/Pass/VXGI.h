#pragma once

#include "ScreenRenderPass.h"

class VXGI : public ScreenRenderPass
{
public:
    VXGI(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    VXGI(const VXGI& rhs) = delete;
    VXGI& operator=(const VXGI& rhs) = delete;
    ~VXGI() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};
