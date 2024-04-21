#pragma once

#include "../D3D/d3dUtil.h"
#include "SceneRenderPass.h"

class Shadow final : public SceneRenderPass
{
public:

	Shadow(ComPtr<ID3D12Device> device,
		std::shared_ptr<DescriptorHeap> heaps,
		std::shared_ptr<LampPSO> PSOs,
		std::shared_ptr<LampGeo> Scene);
	Shadow(const Shadow& rhs) = delete;
	Shadow& operator=(const Shadow& rhs) = delete;
	~Shadow() = default;

	BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

	void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;

protected:

	void BuildResources()override;
	void BuildDescriptors()override;
	void BuildRootSignatureAndPSO()override;

private:
	const std::wstring mColorLS = L"ColorLS";
	const std::wstring mNormalLS = L"NormalLS";
	const std::wstring mShadow = L"MainLightShadow";

	const UINT mRSMWidth = 512;
	const UINT mRSMHeight = 256;

	const DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;
};

 