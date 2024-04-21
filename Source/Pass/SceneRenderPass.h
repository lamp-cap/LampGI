#pragma once

#include "RenderPass.h"

class SceneRenderPass : public RenderPass
{
public:
    SceneRenderPass(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs,
        std::shared_ptr<LampGeo> Scene,
        std::wstring name,
        UINT numSRV);
    SceneRenderPass(const SceneRenderPass& rhs) = delete;
    SceneRenderPass& operator=(const SceneRenderPass& rhs) = delete;
    ~SceneRenderPass() = default;

protected:
    std::shared_ptr<LampGeo> mScene;

    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const RenderLayer layer, FrameResource* currFrame);

};
