#pragma once

#include "../ScreenRenderPass.h"

class LampReflection : public ScreenRenderPass
{
public:
    LampReflection(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    LampReflection(const LampReflection& rhs) = delete;
    LampReflection& operator=(const LampReflection& rhs) = delete;
    ~LampReflection() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO()override;
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};