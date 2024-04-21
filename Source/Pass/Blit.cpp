#include "Blit.h"

Blit::Blit(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
    : ScreenRenderPass(device, heaps, PSOs, L"blit", 0)
{
    pso1 = name;
    rootSig1 = L"blit";
    BuildRootSignatureAndPSO();
}

void Blit::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
}

void Blit::BuildDescriptors()
{
}

BOOL Blit::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    return true;
}

void Blit::BuildResources()
{

}

void Blit::BuildRootSignatureAndPSO()
{
    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE srvTable1;
    srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable0);
    slotRootParameter[2].InitAsDescriptorTable(1, &srvTable1);

    auto staticSamplers = mPSOs->GetStaticSamplers1();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);
    // PSO for drawing ProbeGI.
    std::vector<DXGI_FORMAT> RTVFormats = { LDRFormat, DepthFormat };
    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "blitPS", RTVFormats, false);
}