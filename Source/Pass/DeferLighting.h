#pragma once

#include "ScreenRenderPass.h"

class DeferLighting : public ScreenRenderPass
{
public:
    DeferLighting(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    DeferLighting(const DeferLighting& rhs) = delete;
    DeferLighting& operator=(const DeferLighting& rhs) = delete;
    ~DeferLighting() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:

};
