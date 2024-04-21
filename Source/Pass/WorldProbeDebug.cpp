#include "WorldProbeDebug.h"

WorldProbeDebug::WorldProbeDebug(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : ScreenRenderPass(device, heaps, PSOs, L"WorldProbeDebug", 1)
{
    pso1 = name;
}

void WorldProbeDebug::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(L"WorldProbe"), GRstate, CSstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(L"ProbeWSReadBack"), GRstate, CDstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

    cmdList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(mHeaps->GetResource(L"ProbeWSReadBack"), 0),
        0, 0, 0, &CD3DX12_TEXTURE_COPY_LOCATION(mHeaps->GetResource(L"WorldProbe"), 0), nullptr);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(L"ProbeWSReadBack"),
        CDstate, CSstate));

    D3D12_RESOURCE_DESC desc = mHeaps->GetResource(L"ProbeWSReadBack")->GetDesc();
    UINT64 uploadBufferSize;
    md3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
    // Get the copy target location
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
    bufferFootprint.Footprint.Width = 1024;
    bufferFootprint.Footprint.Height = 1024;
    bufferFootprint.Footprint.Depth = 1;
    bufferFootprint.Footprint.RowPitch = static_cast<UINT>(4096);
    bufferFootprint.Footprint.Format = LDRFormat;

    const CD3DX12_TEXTURE_COPY_LOCATION copyDest(mHeaps->GetResource(probeWSDebug), bufferFootprint);
    const CD3DX12_TEXTURE_COPY_LOCATION copySrc(mHeaps->GetResource(L"ProbeWSReadBack"), 0);

    // Copy the texture
    cmdList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

    resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(L"WorldProbe"), CSstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(L"ProbeWSReadBack"), CSstate, GRstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());
}

void WorldProbeDebug::BuildDescriptors()
{
}

BOOL WorldProbeDebug::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    BuildResources();
    return true;
}

void WorldProbeDebug::BuildResources()
{
    mHeaps->CreateCommitResource2D(L"ProbeWSReadBack", 1024, 1024,
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr, 1);
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(1024*1024*4, D3D12_RESOURCE_FLAG_NONE);
    mHeaps->CreateCommitResource(probeWSDebug, desc, 
        nullptr, &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK), D3D12_RESOURCE_STATE_COPY_DEST);
}

void WorldProbeDebug::BuildRootSignatureAndPSO()
{
}