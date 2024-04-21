#pragma once

#include "../ScreenRenderPass.h"

class CompositionDI : public ScreenRenderPass
{
public:
    CompositionDI(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    CompositionDI(const CompositionDI& rhs) = delete;
    CompositionDI& operator=(const CompositionDI& rhs) = delete;
    ~CompositionDI() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};
