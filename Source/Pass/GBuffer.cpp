#include "GBuffer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

GBuffer::GBuffer(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs,
    std::shared_ptr<LampGeo> Scene)
    : SceneRenderPass(device, heaps, PSOs, Scene, L"GBuffer", 5)
{
    pso1 = name;
    rootSig1 = L"GBuffer";
    BuildRootSignatureAndPSO();
}

void GBuffer::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);

    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));

    // Bind all the materials used in this scene.  For structured buffers, 
    // we can bypass the heap and set as a root descriptor.
    auto matBuffer = currFrame->MaterialBuffer->Resource();
    cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

    // Bind all the textures used in this scene.  
    cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->CustomSRVHeap()->GetGPUDescriptorHandleForHeapStart());

    // Change to RENDER_TARGET.
    std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mBaseColor), GRstate, RTstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mNormal), GRstate, RTstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mMetalicRoughness), GRstate, RTstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

    // Clear the screen normal map and depth buffer.
    float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearRenderTargetView(mHeaps->GetRtv(mBaseColor), clearValue, 0, nullptr);
    cmdList->ClearRenderTargetView(mHeaps->GetRtv(mNormal), clearValue, 0, nullptr);
    cmdList->ClearRenderTargetView(mHeaps->GetRtv(mMetalicRoughness), clearValue, 0, nullptr);
    cmdList->ClearDepthStencilView(mHeaps->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    cmdList->OMSetRenderTargets(3, &mHeaps->GetRtv(mBaseColor), true, &mHeaps->DepthStencilView());

    // Bind the constant buffer for this pass.
    auto passCB = currFrame->PassCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

    DrawRenderItems(cmdList, RenderLayer::Opaque, currFrame);
    DrawRenderItems(cmdList, RenderLayer::Wall, currFrame);

    resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mBaseColor), RTstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mNormal), RTstate, GRstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mMetalicRoughness), RTstate, GRstate)
    };
    cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());
}

void GBuffer::BuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Format = LDRFormat;
    mHeaps->CreateSRV(mBaseColor, &srvDesc);
    mHeaps->CreateSRV(mMetalicRoughness, &srvDesc);
    srvDesc.Format = HDRFormat;
    mHeaps->CreateSRV(mNormal, &srvDesc);

    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    mHeaps->CreateSRV(L"Depth", mHeaps->depthStencilBuffer(), &srvDesc);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    rtvDesc.Format = LDRFormat;
    mHeaps->CreateRTV(mBaseColor, &rtvDesc);
    mHeaps->CreateRTV(mMetalicRoughness, &rtvDesc);
    rtvDesc.Format = HDRFormat;
    mHeaps->CreateRTV(mNormal, &rtvDesc);
}

BOOL GBuffer::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    if (RenderPass::OnResize(newWidth, newHeight))
    {
        BuildResources();
        BuildDescriptors();
        return true;
    }
    return false;
}

void GBuffer::BuildResources()
{
    mHeaps->CreateCommitResource2D(mBaseColor, mWidth, mHeight, 
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    mHeaps->CreateCommitResource2D(mNormal, mWidth, mHeight, 
        HDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    mHeaps->CreateCommitResource2D(mMetalicRoughness, mWidth, mHeight, 
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
}

void GBuffer::BuildRootSignatureAndPSO()
{
    CD3DX12_DESCRIPTOR_RANGE texTable0;
    texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsShaderResourceView(0, 1);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);

    auto staticSamplers = mPSOs->GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    mPSOs->CreateRootSignature(rootSigDesc, rootSig1);

    std::vector<DXGI_FORMAT> rtvFormats = { LDRFormat, LDRFormat, HDRFormat };
    mPSOs->BuildGraphicsPSO(pso1, rootSig1, "drawGBufferVS", "drawGBufferPS", rtvFormats);
}