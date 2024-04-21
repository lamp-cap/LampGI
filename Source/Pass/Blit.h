#pragma once

#include "ScreenRenderPass.h"

class Blit : public ScreenRenderPass
{
public:
    Blit(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    Blit(const Blit& rhs) = delete;
    Blit& operator=(const Blit& rhs) = delete;
    ~Blit() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:

};
