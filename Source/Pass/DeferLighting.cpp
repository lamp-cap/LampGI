#include "DeferLighting.h"

DeferLighting::DeferLighting(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : ScreenRenderPass(device, heaps, PSOs, L"deferLighting", 0)
{
    pso1 = name;
    rootSig1 = L"deferLighting";
    BuildRootSignatureAndPSO();
}

void DeferLighting::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));
    // Change to RENDER_TARGET.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Temp2(),
        GRstate, RTstate));

    float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearRenderTargetView(mHeaps->Temp2Rtv(), clearValue, 0, nullptr);

    // Specify the buffers we are going to render to.
    cmdList->OMSetRenderTargets(1, &mHeaps->Temp2Rtv(), false, nullptr);

    auto passCB = currFrame->PassCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, mHeaps->GetSrv(L"ProbeNormalDepth"));
    cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->SkySrv());
    cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->GetSrv(L"BaseColor"));
    cmdList->SetGraphicsRootDescriptorTable(4, mHeaps->GetSrv(L"ScreenProbeSH0"));

    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));
    // Draw fullscreen quad.
    DrawFullScreen(cmdList);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Temp2(),
        RTstate, GRstate));
}

void DeferLighting::BuildDescriptors()
{
}

BOOL DeferLighting::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    if (RenderPass::OnResize(newWidth, newHeight))
    {
        BuildResources();
        BuildDescriptors();
        return true;
    }
    return false;
}

void DeferLighting::BuildResources()
{

}

void DeferLighting::BuildRootSignatureAndPSO()
{

    CD3DX12_DESCRIPTOR_RANGE texTable0;
    texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE texTable1;
    texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE texTable2;
    texTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 3, 0);

    CD3DX12_DESCRIPTOR_RANGE texTable3;
    texTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 7, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[5];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[2].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable2, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[4].InitAsDescriptorTable(1, &texTable3, D3D12_SHADER_VISIBILITY_PIXEL);

    auto staticSamplers = mPSOs->GetStaticSamplers1();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);
    // PSO for drawing DeferLighting.
    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "DeferLightingPS");

}