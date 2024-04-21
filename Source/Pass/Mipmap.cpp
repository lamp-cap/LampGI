#include "Mipmap.h"

using Microsoft::WRL::ComPtr;

Mipmap::Mipmap(ID3D12Device* device)
{
	md3dDevice = device;
}

void Mipmap::GenerateMipmap(
	ID3D12GraphicsCommandList* mCommandList,
	ID3D12Resource* src,
	CD3DX12_GPU_DESCRIPTOR_HANDLE shGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv)
{
	auto texDesc = src->GetDesc();

	if (texDesc.MipLevels < 2 || texDesc.DepthOrArraySize > 1) return;

	if (mTemp == nullptr)
	{
		mFormat = texDesc.Format;
		mWidth = texDesc.Width;
		mHeight = texDesc.Height;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&mTemp)));

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src,
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

		mCommandList->CopyResource(mTemp.Get(), src);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src,
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTemp.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		for (int i = 0; i < 7; i++)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = i;
			uavDesc.Texture2D.PlaneSlice = 0;
			uavDesc.Format = mFormat;
			md3dDevice->CreateUnorderedAccessView(mTemp.Get(), nullptr, &uavDesc, hCpuSrv);
			hCpuSrv.Offset(mSrvDescriptorSize);
		}
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = mFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 3;
		md3dDevice->CreateShaderResourceView(mTemp.Get(), &srvDesc, hCpuSrv);
	}
	else
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src,
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	mCommandList->SetComputeRootSignature(rootSig.Get());
	mCommandList->SetPipelineState(mPSO.Get());

	GenerateMipmap(mCommandList, shGpuSrv, hGpuSrv, 0);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTemp.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(src, mTemp.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTemp.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	GenerateMipmap(mCommandList, shGpuSrv, hGpuSrv, 3);
	
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTemp.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->CopyResource(src, mTemp.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTemp.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Mipmap::GenerateMipmap(ID3D12GraphicsCommandList* mCommandList,
							 CD3DX12_GPU_DESCRIPTOR_HANDLE shGpuSrv,
							 CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
							 UINT srcLevel)
{
	UINT width = (UINT)(mWidth / pow(2, srcLevel));
	UINT height = (UINT)(mHeight / pow(2, srcLevel));
	float data[] = { (float)srcLevel, 4, 1.0f / width, 1.0f / height };
	mCommandList->SetComputeRoot32BitConstants(0, 4, data, 0);
	mCommandList->SetComputeRootDescriptorTable(1, shGpuSrv);
	mCommandList->SetComputeRootDescriptorTable(2, hGpuSrv.Offset(srcLevel * mSrvDescriptorSize));

	UINT numGroupsX = (UINT)ceilf(width / 8.0f);
	UINT numGroupsY = (UINT)ceilf(height / 8.0f);

	mCommandList->Dispatch(numGroupsX, numGroupsY, 1);

}

void Mipmap::SetPSO(ID3D12PipelineState* PSO, ID3D12RootSignature* rootSignature, UINT srvDescriptorSize)
{
	mPSO = PSO;
	rootSig = rootSignature;
	mSrvDescriptorSize = srvDescriptorSize;
}

