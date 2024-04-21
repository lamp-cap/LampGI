#include "Voxelize.h"

Voxel::Voxel(ComPtr<ID3D12Device> device,
	std::shared_ptr<DescriptorHeap> heaps,
	std::shared_ptr<LampPSO> PSOs,
	std::shared_ptr<LampGeo> Scene)
	: SceneRenderPass(device, heaps, PSOs, Scene, L"Voxelize", 6)
{
	pso1 = name;
	rootSig1 = L"Voxelize";
	BuildRootSignatureAndPSO();
}

void Voxel::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));

	// Change to RENDER_TARGET.
	std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers = {
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mTempTarget), GRstate, RTstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mVoxelColor), GRstate, UAstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mVoxelNormal), GRstate, UAstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mVoxelMat), GRstate, UAstate)
	};
	cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

	float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	cmdList->ClearRenderTargetView(mHeaps->GetRtv(mTempTarget), clearValue, 0, nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = mHeaps->CustomSRVHeap()->GetDesc();

	D3D12_CPU_DESCRIPTOR_HANDLE tempCPUHandle = mHeaps->CustomSRVHeap()->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE uavCPUHandle = mHeaps->GetCPUUav(mVoxelColor);

	D3D12_GPU_DESCRIPTOR_HANDLE tempGPUHandle = mHeaps->CustomSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE uavGPUHandle = mHeaps->GetGPUUav(mVoxelColor);
	/*
	if (uavCPUHandle.ptr >= tempCPUHandle.ptr && uavCPUHandle.ptr < tempCPUHandle.ptr + heapDesc.NumDescriptors * 144)
		OutputDebugString(L"CPUuavHandle is on my heap.\n");
	if (uavGPUHandle.ptr >= tempGPUHandle.ptr && uavGPUHandle.ptr < tempGPUHandle.ptr + heapDesc.NumDescriptors * 144)
		OutputDebugString(L"GPUUavHandle is on my heap.\n");
	if(heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		OutputDebugString(L"Heap flags is D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE.\n");

	D3D12_RESOURCE_DESC resourceDesc = mHeaps->GetResource(mVoxelColor)->GetDesc();
	if(resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		OutputDebugString(L"resource State is D3D12_RESOURCE_STATE_UNORDERED_ACCESS.\n");
	*/
	// cmdList->ClearUnorderedAccessViewFloat(mHeaps->GetGPUUav(mVoxelColor), mHeaps->GetCPUUav(mVoxelColor),
	//	mHeaps->GetResource(mVoxelColor), clearValue, 0, nullptr);

	// Specify the buffers we are going to render to.
	cmdList->OMSetRenderTargets(1, &mHeaps->GetRtv(mTempTarget), false, nullptr);
	// textures
	cmdList->SetGraphicsRootDescriptorTable(5, mHeaps->CustomSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	auto matBuffer = currFrame->MaterialBuffer->Resource();
	cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	auto passCB = currFrame->PassCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootDescriptorTable(4, mHeaps->GetSrv(L"MainLightShadow"));
	cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->GetGPUUav(mVoxelColor));

	cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

	DrawRenderItems(cmdList, RenderLayer::Opaque, currFrame);

	resourceBarriers = {
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mTempTarget), RTstate, GRstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mVoxelColor), UAstate, GRstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mVoxelNormal), UAstate, GRstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mVoxelMat), UAstate, GRstate)
	};
	cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());
}

void Voxel::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	// srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture3D.MostDetailedMip = 0;
	srvDesc.Texture3D.MipLevels = 7;
	srvDesc.Format = LDRFormat;
	mHeaps->CreateSRV(mVoxelColor, &srvDesc);
	mHeaps->CreateSRV(mVoxelMat, &srvDesc);
	mHeaps->CreateSRV(mVoxelNormal, &srvDesc);
	
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = LDRFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.WSize = 256;
	uavDesc.Texture3D.FirstWSlice = 0;
	mHeaps->CreateUAV(mVoxelColor, &uavDesc);
	mHeaps->CreateUAV(mVoxelMat, &uavDesc);
	mHeaps->CreateUAV(mVoxelNormal, &uavDesc);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	rtvDesc.Format = LDRFormat;
	mHeaps->CreateRTV(mTempTarget, &rtvDesc);
}

BOOL Voxel::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
	if (RenderPass::OnResize(newWidth, newHeight, newDepth))
	{
		BuildResources();
		BuildDescriptors();
		return true;
	}
	return false;
}

void Voxel::BuildResources()
{
	mHeaps->CreateCommitResource3D(mVoxelColor,
		mWidth, mHeight/2, mDepth, LDRFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr, 7);
	mHeaps->CreateCommitResource3D(mVoxelMat,
		mWidth, mHeight/2, mDepth, LDRFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr, 7);
	mHeaps->CreateCommitResource3D(mVoxelNormal,
		mWidth, mHeight/2, mDepth, LDRFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr, 7);

	mHeaps->CreateCommitResource2D(mTempTarget, mWidth, mHeight,
		LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
}

void Voxel::BuildRootSignatureAndPSO()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE uavTable0;
	uavTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 0);
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &uavTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = mPSOs->GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	mPSOs->CreateRootSignature(rootSigDesc, rootSig1);

	std::vector<DXGI_FORMAT> rtvFormats = { LDRFormat };
	CD3DX12_RASTERIZER_DESC RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	RasterizerState.DepthClipEnable = false;
	RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
	CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DepthStencilState.DepthEnable = false;
	DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mPSOs->BuildGraphicsPSO(pso1, rootSig1, "VoxelizeVS", "VoxelizeGS", "VoxelizePS",
		rtvFormats, DepthStencilState, RasterizerState);
}