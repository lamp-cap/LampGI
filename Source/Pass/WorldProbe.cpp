#include "WorldProbe.h"

WorldProbe::WorldProbe(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : ScreenRenderPass(device, heaps, PSOs, L"WorldProbe", 1)
{
    pso1 = name;
    rootSig1 = L"WorldProbe";
    pso2 = L"ProbeToSH";
    rootSig2 = L"ProbeToSH";
    BuildRootSignatureAndPSO();
}

void WorldProbe::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));
    // Change to RENDER_TARGET.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeWS),
        GRstate, RTstate));

    float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearRenderTargetView(mHeaps->GetRtv(probeWS), clearValue, 0, nullptr);

    // Specify the buffers we are going to render to.
    cmdList->OMSetRenderTargets(1, &mHeaps->GetRtv(probeWS), false, nullptr);

    auto passCB = currFrame->PassCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, mHeaps->SkySrv());
    cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->GetSrv(probeHis));
    cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->GetSrv(L"VoxelizedColor"));
    cmdList->SetGraphicsRootDescriptorTable(4, mHeaps->GetSrv(L"ProbeSH0"));

    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));
    // Draw fullscreen quad.
    DrawFullScreen(cmdList);

    std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeWS), RTstate, CSstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeHis), GRstate, CDstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

    cmdList->CopyResource(mHeaps->GetResource(probeHis), mHeaps->GetResource(probeWS));

    resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeWS), CSstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeHis), CDstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeSH0), GRstate, UAstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeSH1), GRstate, UAstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeSH2), GRstate, UAstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

    cmdList->SetComputeRootSignature(mPSOs->GetRootSignature(rootSig2));
    cmdList->SetPipelineState(mPSOs->GetPSO(pso2));

    cmdList->SetComputeRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(1, mHeaps->GetSrv(probeWS));
    cmdList->SetComputeRootDescriptorTable(2, mHeaps->GetGPUUav(probeSH0));

    cmdList->Dispatch(128, 128, 1);

    resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeSH0), UAstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeSH1), UAstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(probeSH2), UAstate, GRstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

}

void WorldProbe::BuildDescriptors()
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = HDRFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice = 0;
    uavDesc.Texture3D.WSize = 32;
    uavDesc.Texture3D.FirstWSlice = 0;
    mHeaps->CreateUAV(probeSH0, mHeaps->GetResource(probeSH0), &uavDesc);
    mHeaps->CreateUAV(probeSH1, mHeaps->GetResource(probeSH1), &uavDesc);
    mHeaps->CreateUAV(probeSH2, mHeaps->GetResource(probeSH2), &uavDesc);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Format = LDRFormat;
    mHeaps->CreateSRV(probeWS, &srvDesc);
    mHeaps->CreateSRV(probeHis, &srvDesc);

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MipLevels = 1;
    srvDesc.Format = HDRFormat;
    mHeaps->CreateSRV(probeSH0, &srvDesc);
    mHeaps->CreateSRV(probeSH1, &srvDesc);
    mHeaps->CreateSRV(probeSH2, &srvDesc);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    rtvDesc.Format = LDRFormat;
    mHeaps->CreateRTV(probeWS, &rtvDesc);
}

BOOL WorldProbe::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    RenderPass::OnResize(1024, 1024);
    BuildResources();
    BuildDescriptors();
    return true;
}

void WorldProbe::BuildResources()
{
    mHeaps->CreateCommitResource2D(probeWS, 1024, 1024,
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    mHeaps->CreateCommitResource2D(probeHis, 1024, 1024,
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    mHeaps->CreateCommitResource3D(probeSH0, 32, 16, 32,
        HDRFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr);
    mHeaps->CreateCommitResource3D(probeSH1, 32, 16, 32,
        HDRFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr);
    mHeaps->CreateCommitResource3D(probeSH2, 32, 16, 32,
        HDRFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr);
}

void WorldProbe::BuildRootSignatureAndPSO()
{
    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE srvTable1;
    srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    CD3DX12_DESCRIPTOR_RANGE srvTable2;
    srvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 2);
    CD3DX12_DESCRIPTOR_RANGE srvTable3;
    srvTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 5);

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
    // PSO for drawing WorldProbe.
    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "WorldProbePS");

    CD3DX12_DESCRIPTOR_RANGE srvTable4;
    srvTable4.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE uavTable1;
    uavTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter1[3];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter1[0].InitAsConstantBufferView(0);
    slotRootParameter1[1].InitAsDescriptorTable(1, &srvTable4);
    slotRootParameter1[2].InitAsDescriptorTable(1, &uavTable1);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc1(3, slotRootParameter1,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc1, rootSig2);
    // PSO for drawing WorldProbe.
    mPSOs->BuildComputePSO(pso2, rootSig2, "ProbeToSHCS");
}