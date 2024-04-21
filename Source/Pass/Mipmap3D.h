#pragma once

#include "ScreenRenderPass.h"

class Mipmap3D : public ScreenRenderPass
{
public:
	Mipmap3D(ComPtr<ID3D12Device> device,
		std::shared_ptr<DescriptorHeap> heaps,
		std::shared_ptr<LampPSO> PSOs,
		std::wstring target);
	Mipmap3D(const Mipmap3D& rhs) = delete;
	Mipmap3D& operator=(const Mipmap3D& rhs) = delete;
	~Mipmap3D() = default;

	virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
	BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

	void BuildRootSignatureAndPSO();
	void BuildResources()override;
	void BuildDescriptors()override;
	void GenerateMipmap3D(ID3D12GraphicsCommandList* cmdList, UINT srcLevel);

private:

	std::wstring mTarget;
	BOOL mInitialize;
	BOOL hasTemp;
	std::wstring mTemp;
	DXGI_FORMAT mFormat;
};

 