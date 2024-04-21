#pragma once

#include "ScreenRenderPass.h"

class WorldProbeDebug : public ScreenRenderPass
{
public:
    WorldProbeDebug(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    WorldProbeDebug(const WorldProbeDebug& rhs) = delete;
    WorldProbeDebug& operator=(const WorldProbeDebug& rhs) = delete;
    ~WorldProbeDebug() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
    const std::wstring probeWSDebug = L"WorldProbeDebug";
};