#pragma once

#include "./LampDI.h"

class VoxelDI : public LampDI
{
public:
    VoxelDI(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    VoxelDI(const VoxelDI& rhs) = delete;
    VoxelDI& operator=(const VoxelDI& rhs) = delete;
    ~VoxelDI() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO()override;
    void BuildResources()override;
    void BuildDescriptors()override;

private:
};