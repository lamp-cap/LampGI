#include "ScreenProbeSH.h"

ScreenProbeSH::ScreenProbeSH(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : LampDI(device, heaps, PSOs, L"LampSH")
{
    pso1 = L"LampSH";
    rootSig1 = L"LampSH";
    BuildRootSignatureAndPSO();
}

void ScreenProbeSH::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    Swap();
    std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(ScreenProbeSH0), GRstate, UAstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(ScreenProbeSH1), GRstate, UAstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(ScreenProbeSH2), GRstate, UAstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

    cmdList->SetComputeRootSignature(mPSOs->GetRootSignature(rootSig1));
    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

    cmdList->SetComputeRootDescriptorTable(0, mHeaps->GetSrv(mCurDI));
    cmdList->SetComputeRootDescriptorTable(1, mHeaps->GetSrv(mNormalDepth));
    cmdList->SetComputeRootDescriptorTable(2, mHeaps->GetGPUUav(ScreenProbeSH0));

    cmdList->Dispatch(160, 90, 1);

    resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(ScreenProbeSH0), UAstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(ScreenProbeSH1), UAstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(ScreenProbeSH2), UAstate, GRstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());
}

BOOL ScreenProbeSH::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    auto desc = mHeaps->GetResource(mCurDI)->GetDesc();
    RenderPass::OnResize(desc.Width, desc.Height);
    return true;
}

void ScreenProbeSH::BuildResources()
{

}

void ScreenProbeSH::BuildDescriptors()
{
}

void ScreenProbeSH::BuildRootSignatureAndPSO()
{
    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE srvTable1;
    srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

    CD3DX12_DESCRIPTOR_RANGE uavTable0;
    uavTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    slotRootParameter[0].InitAsDescriptorTable(1, &srvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable1);
    slotRootParameter[2].InitAsDescriptorTable(1, &uavTable0);

    auto staticSamplers = mPSOs->GetStaticSamplers1();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);
    // PSO for drawing WorldProbe.
    mPSOs->BuildComputePSO(pso1, rootSig1, "ScreenProbeCS");

}