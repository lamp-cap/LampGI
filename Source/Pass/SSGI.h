#pragma once

#include "ScreenRenderPass.h"

class SSGI : public ScreenRenderPass
{
public:
    SSGI(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    SSGI(const SSGI& rhs) = delete;
    SSGI& operator=(const SSGI& rhs) = delete;
    ~SSGI() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};
