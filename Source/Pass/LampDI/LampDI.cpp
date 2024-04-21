#include "LampDI.h"

LampDI::LampDI(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs,
    std::wstring name)
    : ScreenRenderPass(device, heaps, PSOs, name, 0)
{
}

void LampDI::Swap()
{
    std::wstring temp = mHisDI;
    mHisDI = mCurDI;
    mCurDI = temp;
}

void LampDI::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
}

void LampDI::BuildDescriptors()
{

}

BOOL LampDI::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    BuildResources();
    BuildDescriptors();

    auto desc = mHeaps->GetResource(mCurDI)->GetDesc();
    RenderPass::OnResize(desc.Width, desc.Height);

    return true;
}

void LampDI::BuildResources()
{
}

void LampDI::BuildRootSignatureAndPSO()
{
}