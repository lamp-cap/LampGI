#include "TAA.h"

TAA::TAA(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : ScreenRenderPass(device, heaps, PSOs, L"taa", 0)
{
    pso1 = name;
    rootSig1 = L"taa";
    BuildRootSignatureAndPSO();
}

void TAA::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));

    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

    auto taaCB = currFrame->TaaCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(0, taaCB->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, mHeaps->HistorySrv());
    cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->CurrentSrv());
    cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->GetSrv(L"Depth"));

    // Draw fullscreen quad.
    DrawFullScreen(cmdList);
}

void TAA::BuildDescriptors()
{
}

BOOL TAA::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    if (RenderPass::OnResize(newWidth, newHeight))
    {
        BuildResources();
        BuildDescriptors();
        return true;
    }
    return false;
}

void TAA::BuildResources()
{

}

void TAA::BuildRootSignatureAndPSO()
{

    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE srvTable1;
    srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    CD3DX12_DESCRIPTOR_RANGE srvTable2;
    srvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable0);
    slotRootParameter[2].InitAsDescriptorTable(1, &srvTable1);
    slotRootParameter[3].InitAsDescriptorTable(1, &srvTable2);

    auto staticSamplers = mPSOs->GetStaticSamplers();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);
    // PSO for drawing TAA.
    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "taaPS");

}