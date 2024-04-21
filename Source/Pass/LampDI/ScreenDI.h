#pragma once

#include "./LampDI.h"

class ScreenDI : public LampDI
{
public:
    ScreenDI(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    ScreenDI(const ScreenDI& rhs) = delete;
    ScreenDI& operator=(const ScreenDI& rhs) = delete;
    ~ScreenDI() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO()override;
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};