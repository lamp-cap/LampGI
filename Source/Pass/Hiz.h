#pragma once

#include "ScreenRenderPass.h"

class HierachyZBuffer : public ScreenRenderPass
{
public:
	HierachyZBuffer(ComPtr<ID3D12Device> device,
		std::shared_ptr<DescriptorHeap> heaps,
		std::shared_ptr<LampPSO> PSOs);
	HierachyZBuffer(const HierachyZBuffer& rhs) = delete;
	HierachyZBuffer& operator=(const HierachyZBuffer& rhs) = delete;
	~HierachyZBuffer() = default;

	virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
	BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

	void BuildRootSignatureAndPSO();
	void BuildResources()override;
	void BuildDescriptors()override;
	void GenerateHiz(ID3D12GraphicsCommandList* cmdList, UINT srcLevel);

private:

	const std::wstring mHiz = L"Hiz";
};

 

 