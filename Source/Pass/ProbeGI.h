#pragma once

#include "ScreenRenderPass.h"

class ProbeGI : public ScreenRenderPass
{
public:
    ProbeGI(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    ProbeGI(const ProbeGI& rhs) = delete;
    ProbeGI& operator=(const ProbeGI& rhs) = delete;
    ~ProbeGI() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};
