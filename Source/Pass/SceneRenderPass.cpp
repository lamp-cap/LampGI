#include "SceneRenderPass.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

SceneRenderPass::SceneRenderPass(
    ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs,
    std::shared_ptr<LampGeo> Scene,
    std::wstring name,
    UINT numSRV)
    : RenderPass(device, heaps, PSOs, name, numSRV)
{
    mScene = Scene;
}

void SceneRenderPass::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const RenderLayer layer, FrameResource* currFrame)
{
    const std::vector<RenderItem*>& ritems = mScene->RenderItems(layer);
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    auto objectCB = currFrame->ObjectCB->Resource();

    // For each render item...
    for (size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (UINT64)ri->ObjCBIndex * objCBByteSize;

        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}
