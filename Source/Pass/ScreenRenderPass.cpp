#include "ScreenRenderPass.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

ScreenRenderPass::ScreenRenderPass(
    ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs,
    std::wstring name,
    UINT numSRV)
    : RenderPass(device, heaps, PSOs, name, numSRV)
{
}

void ScreenRenderPass::DrawFullScreen(ID3D12GraphicsCommandList* cmdList)
{
    // Null-out IA stage since we build the vertex off the SV_VertexID in the shader.
    cmdList->IASetVertexBuffers(0, 1, nullptr);
    cmdList->IASetIndexBuffer(nullptr);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawInstanced(6, 1, 0, 0);
}
