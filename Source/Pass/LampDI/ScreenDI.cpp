#include "ScreenDI.h"

ScreenDI::ScreenDI(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : LampDI(device, heaps, PSOs, L"LampSDI")
{
    pso1 = L"LampSDI";
    rootSig1 = L"LampSDI";
    BuildRootSignatureAndPSO();
}

void ScreenDI::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    Swap();
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mCurDI),
        GRstate, RTstate));

    float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearRenderTargetView(mHeaps->GetRtv(mCurDI), clearValue, 0, nullptr);

    // Specify the buffers we are going to render to.
    cmdList->OMSetRenderTargets(1, &mHeaps->GetRtv(mCurDI), false, nullptr);

    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

    auto passCB = currFrame->PassCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, mHeaps->SkySrv());
    cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->HistorySrv());
    cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->GetSrv(L"Normal"));
    cmdList->SetGraphicsRootDescriptorTable(4, mHeaps->GetSrv(L"Hiz"));
    cmdList->SetGraphicsRootDescriptorTable(5, mHeaps->GetSrv(mNormalDepth));

    DrawFullScreen(cmdList);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mCurDI),
        RTstate, GRstate));
}

void ScreenDI::BuildDescriptors()
{
}

BOOL ScreenDI::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    auto desc = mHeaps->GetResource(mCurDI)->GetDesc();
    RenderPass::OnResize(desc.Width, desc.Height);
    return true;
}

void ScreenDI::BuildResources()
{
}

void ScreenDI::BuildRootSignatureAndPSO()
{
    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Cube
    CD3DX12_DESCRIPTOR_RANGE srvTable1;
    srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // OffScreen
    CD3DX12_DESCRIPTOR_RANGE srvTable2;
    srvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // Normal
    CD3DX12_DESCRIPTOR_RANGE srvTable3;
    srvTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // Hiz
    CD3DX12_DESCRIPTOR_RANGE srvTable4;
    srvTable4.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // Normal Depth

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[6];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable0);
    slotRootParameter[2].InitAsDescriptorTable(1, &srvTable1);
    slotRootParameter[3].InitAsDescriptorTable(1, &srvTable2);
    slotRootParameter[4].InitAsDescriptorTable(1, &srvTable3);
    slotRootParameter[5].InitAsDescriptorTable(1, &srvTable4);

    auto staticSamplers = mPSOs->GetStaticSamplers1();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);

    std::vector<DXGI_FORMAT> rtvFormats = { HDRFormat };

    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "ScreenDIPS", rtvFormats);
}