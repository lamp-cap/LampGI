#pragma once

#include "./LampDI.h"

class ScreenProbeSH : public LampDI
{
public:
    ScreenProbeSH(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    ScreenProbeSH(const ScreenProbeSH& rhs) = delete;
    ScreenProbeSH& operator=(const ScreenProbeSH& rhs) = delete;
    ~ScreenProbeSH() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};
