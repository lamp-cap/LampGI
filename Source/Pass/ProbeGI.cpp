#include "ProbeGI.h"

ProbeGI::ProbeGI(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : ScreenRenderPass(device, heaps, PSOs, L"Probegi", 0)
{
    pso1 = name;
    rootSig1 = L"Probegi";
    BuildRootSignatureAndPSO();
}

void ProbeGI::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    auto desc = mHeaps->Temp2()->GetDesc();
    mViewport = { 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 1.0f };
    mScissorRect = { 0, 0, (int)desc.Width, (int)desc.Height };
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Temp2(),
        GRstate, RTstate));

    float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearRenderTargetView(mHeaps->Temp2Rtv(), clearValue, 0, nullptr);

    // Specify the buffers we are going to render to.
    cmdList->OMSetRenderTargets(1, &mHeaps->Temp2Rtv(), false, nullptr);

    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

    auto passCB = currFrame->PassCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, mHeaps->SkySrv());
    cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->GetSrv(L"BaseColor"));
    cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->GetSrv(L"VoxelizedColor"));
    cmdList->SetGraphicsRootDescriptorTable(4, mHeaps->GetSrv(L"ProbeSH0"));

    DrawFullScreen(cmdList);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Temp2(),
        RTstate, GRstate));
}

void ProbeGI::BuildDescriptors()
{
}

BOOL ProbeGI::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    if (RenderPass::OnResize(newWidth, newHeight))
    {
        BuildResources();
        BuildDescriptors();
        return true;
    }
    return false;
}

void ProbeGI::BuildResources()
{

}

void ProbeGI::BuildRootSignatureAndPSO()
{
    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE srvTable1;
    srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1);

    CD3DX12_DESCRIPTOR_RANGE srvTable2;
    srvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);

    CD3DX12_DESCRIPTOR_RANGE srvTable3;
    srvTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 6);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[5];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable0);
    slotRootParameter[2].InitAsDescriptorTable(1, &srvTable1);
    slotRootParameter[3].InitAsDescriptorTable(1, &srvTable2);
    slotRootParameter[4].InitAsDescriptorTable(1, &srvTable3);

    auto staticSamplers = mPSOs->GetStaticSamplers1();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);
    // PSO for drawing ProbeGI.
    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "ProbegiPS");

}