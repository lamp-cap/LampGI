#pragma once

#include "../main/DescriptorHeap.h"
#include "../main/PSO.h"
#include <DirectXMath.h>
 
class RenderPass
{
public:
	RenderPass(ComPtr<ID3D12Device> device, 
		std::shared_ptr<DescriptorHeap> heaps, 
		std::shared_ptr<LampPSO> PSOs,
		std::wstring name, UINT srvNum);
    RenderPass(const RenderPass& rhs) = delete;
    RenderPass& operator=(const RenderPass& rhs) = delete;
    ~RenderPass() = default; 
	
	const UINT SRVNum;
	UINT Width()const;
    UINT Height()const;

	virtual BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1);
	virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame) = 0;

protected:
	const std::wstring name;
	ComPtr<ID3D12Device> md3dDevice;
	std::shared_ptr<DescriptorHeap> mHeaps;
	std::shared_ptr<LampPSO> mPSOs;
	std::wstring pso1;
	std::wstring pso2;
	std::wstring pso3;
	std::wstring rootSig1;
	std::wstring rootSig2;
	std::wstring rootSig3;

	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mDepth = 0;
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
	// std::unordered_map<std::string, DXGI_FORMAT> Formats;

	static constexpr D3D12_RESOURCE_STATES DWstate = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	static constexpr D3D12_RESOURCE_STATES DRstate = D3D12_RESOURCE_STATE_DEPTH_READ;
	static constexpr D3D12_RESOURCE_STATES GRstate = D3D12_RESOURCE_STATE_GENERIC_READ;
	static constexpr D3D12_RESOURCE_STATES RTstate = D3D12_RESOURCE_STATE_RENDER_TARGET;
	static constexpr D3D12_RESOURCE_STATES UAstate = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	static constexpr D3D12_RESOURCE_STATES CSstate = D3D12_RESOURCE_STATE_COPY_SOURCE;
	static constexpr D3D12_RESOURCE_STATES CDstate = D3D12_RESOURCE_STATE_COPY_DEST;

	// static const DXGI_FORMAT ScreenFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT LDRFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	static constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_R32_FLOAT;

	virtual void BuildResources() = 0;
	virtual void BuildDescriptors() = 0;
	virtual void BuildRootSignatureAndPSO() = 0;
};