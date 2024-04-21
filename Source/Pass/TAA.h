#pragma once

#include "ScreenRenderPass.h"

class TAA : public ScreenRenderPass
{
public:
    TAA(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    TAA(const TAA& rhs) = delete;
    TAA& operator=(const TAA& rhs) = delete;
    ~TAA() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:

};
