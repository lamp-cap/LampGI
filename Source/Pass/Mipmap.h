#pragma once

#include "../main/PSO.h"

class Mipmap
{
public:
	Mipmap(ID3D12Device* device);
		
	Mipmap(const Mipmap& rhs)=delete;
	Mipmap& operator=(const Mipmap& rhs)=delete;
	~Mipmap() = default;

	const UINT SRVNum = 8;

	void SetPSO(ID3D12PipelineState* PSO, ID3D12RootSignature* rootSig, UINT srvDescriptorSize);

	void GenerateMipmap(
		ID3D12GraphicsCommandList* mCommandList,
		ID3D12Resource* src,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv1,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);

private:

	void GenerateMipmap(
		ID3D12GraphicsCommandList* mCommandList,
		CD3DX12_GPU_DESCRIPTOR_HANDLE shGpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		UINT srcLevel);

private:
	Microsoft::WRL::ComPtr <ID3D12Device> md3dDevice;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
	Microsoft::WRL::ComPtr<ID3DBlob> mCSShader;
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSig;

	UINT64 mWidth;
	UINT64 mHeight;
	DXGI_FORMAT mFormat;
	UINT mSrvDescriptorSize;

	Microsoft::WRL::ComPtr <ID3D12Resource> mTemp = nullptr;
};

 