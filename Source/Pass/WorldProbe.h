#pragma once

#include "ScreenRenderPass.h"

class WorldProbe : public ScreenRenderPass
{
public:
    WorldProbe(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    WorldProbe(const WorldProbe& rhs) = delete;
    WorldProbe& operator=(const WorldProbe& rhs) = delete;
    ~WorldProbe() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
    const std::wstring probeWS = L"WorldProbe";
    const std::wstring probeHis = L"WorldProbeHis";
    const std::wstring probeSH0 = L"ProbeSH0";
    const std::wstring probeSH1 = L"ProbeSH1";
    const std::wstring probeSH2 = L"ProbeSH2";
};