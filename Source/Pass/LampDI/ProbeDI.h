#pragma once

#include "./LampDI.h"

class ProbeDI : public LampDI
{
public:
    ProbeDI(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    ProbeDI(const ProbeDI& rhs) = delete;
    ProbeDI& operator=(const ProbeDI& rhs) = delete;
    ~ProbeDI() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO();
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};
