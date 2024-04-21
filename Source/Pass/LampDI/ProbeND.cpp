#include "ProbeND.h"

ProbeND::ProbeND(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : LampDI(device, heaps, PSOs, L"ProbeND")
{
    pso1 = L"LampNDDI";
    rootSig1 = L"LampNDDI";
    BuildRootSignatureAndPSO();
}

void ProbeND::Swap()
{
    std::wstring temp = mHisDI;
    mHisDI = mCurDI;
    mCurDI = temp;
}

void ProbeND::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    Swap();
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mNormalDepth),
        GRstate, RTstate));

    float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearRenderTargetView(mHeaps->GetRtv(mNormalDepth), clearValue, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &mHeaps->GetRtv(mNormalDepth), false, nullptr);

    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

    auto passCB = currFrame->PassCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, mHeaps->GetSrv(L"Normal"));

    DrawFullScreen(cmdList);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mNormalDepth),
        RTstate, GRstate));
}

void ProbeND::BuildDescriptors()
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = HDRFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    mHeaps->CreateUAV(ScreenProbeSH0, &uavDesc);
    mHeaps->CreateUAV(ScreenProbeSH1, &uavDesc);
    mHeaps->CreateUAV(ScreenProbeSH2, &uavDesc);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Format = HDRFormat;
    mHeaps->CreateSRV(mCurDI, &srvDesc);
    mHeaps->CreateSRV(mHisDI, &srvDesc);
    mHeaps->CreateSRV(mTempDI, &srvDesc);
    mHeaps->CreateSRV(mNormalDepth, &srvDesc);
    mHeaps->CreateSRV(ScreenProbeSH0, &srvDesc);
    mHeaps->CreateSRV(ScreenProbeSH1, &srvDesc);
    mHeaps->CreateSRV(ScreenProbeSH2, &srvDesc);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    rtvDesc.Format = HDRFormat;
    mHeaps->CreateRTV(mCurDI, &rtvDesc);
    mHeaps->CreateRTV(mHisDI, &rtvDesc);
    mHeaps->CreateRTV(mTempDI, &rtvDesc);
    mHeaps->CreateRTV(mNormalDepth, &rtvDesc);

}

BOOL ProbeND::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    BuildResources();
    BuildDescriptors();

    auto desc = mHeaps->GetResource(mCurDI)->GetDesc();
    RenderPass::OnResize(desc.Width, desc.Height);

    return true;
}

void ProbeND::BuildResources()
{
    mHeaps->CreateCommitResource2D(mCurDI, 1280, 720, HDRFormat,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mHeaps->CreateCommitResource2D(mHisDI, 1280, 720, HDRFormat,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    mHeaps->CreateCommitResource2D(mTempDI, 1280, 720, HDRFormat,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    mHeaps->CreateCommitResource2D(mNormalDepth, 1280, 720, HDRFormat,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    mHeaps->CreateCommitResource2D(ScreenProbeSH0, 160, 90, HDRFormat,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mHeaps->CreateCommitResource2D(ScreenProbeSH1, 160, 90, HDRFormat,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mHeaps->CreateCommitResource2D(ScreenProbeSH2, 160, 90, HDRFormat,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void ProbeND::BuildRootSignatureAndPSO()
{
    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); // Normal Depth

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable0);

    auto staticSamplers = mPSOs->GetStaticSamplers1();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);
    std::vector<DXGI_FORMAT> rtvFormats = { HDRFormat };
    // PSO for drawing SSGI.
    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "ProbeNDPS", rtvFormats);
}