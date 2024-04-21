#include "./Hiz.h"

HierachyZBuffer::HierachyZBuffer(ComPtr<ID3D12Device> device,
	std::shared_ptr<DescriptorHeap> heaps,
	std::shared_ptr<LampPSO> PSOs)
	: ScreenRenderPass(device, heaps, PSOs, L"hiz", 0)
{
	pso1 = name;
	rootSig1 = L"hiz";
	BuildRootSignatureAndPSO();
}

void HierachyZBuffer::GenerateHiz(ID3D12GraphicsCommandList* cmdList, UINT srcLevel)
{

	UINT width = (UINT)(mWidth / pow(2, srcLevel));
	UINT height = (UINT)(mHeight / pow(2, srcLevel));
	float data[] = { (float)srcLevel, 4, 1.0f / width, 1.0f / height };

	cmdList->SetComputeRoot32BitConstants(0, 4, data, 0);
	cmdList->SetComputeRootDescriptorTable(1, mHeaps->GetSrv(mHiz));
	cmdList->SetComputeRootDescriptorTable(2, mHeaps->GetGPUUav(mHiz + std::to_wstring(srcLevel)));

	UINT numGroupsX = (UINT)ceilf(width / 8.0f);
	UINT numGroupsY = (UINT)ceilf(height / 8.0f);

	cmdList->Dispatch(numGroupsX, numGroupsY, 1);
}


void HierachyZBuffer::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);
	cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(L"blit"));

	std::vector<D3D12_RESOURCE_BARRIER> resourceBarrier =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mHiz), GRstate, RTstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Offscreen(), GRstate, RTstate)
	};
	cmdList->ResourceBarrier(2, resourceBarrier.data());

	float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	cmdList->ClearRenderTargetView(mHeaps->OffscreenRtv(), clearValue, 0, nullptr);
	cmdList->ClearRenderTargetView(mHeaps->GetRtv(mHiz), clearValue, 0, nullptr);

	// Specify the buffers we are going to render to.
	cmdList->OMSetRenderTargets(2, &mHeaps->OffscreenRtv(), true, nullptr);

	cmdList->SetPipelineState(mPSOs->GetPSO(L"blit"));

	auto passCB = currFrame->PassCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(1, mHeaps->HistorySrv());
	cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->GetSrv(L"Depth"));

	DrawFullScreen(cmdList);

	resourceBarrier =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mHiz), RTstate, GRstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Offscreen(), RTstate, GRstate)
	};
	cmdList->ResourceBarrier(2, resourceBarrier.data());

	cmdList->SetComputeRootSignature(mPSOs->GetRootSignature(rootSig1));
	cmdList->SetPipelineState(mPSOs->GetPSO(pso1));
	int srcLevel = 0;
	GenerateHiz(cmdList, srcLevel);
	GenerateHiz(cmdList, 3);
}

void HierachyZBuffer::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 7;
	srvDesc.Format = DepthFormat;
	mHeaps->CreateSRV(mHiz, &srvDesc);
	for (int i = 0; i < 7; i++)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = i;
		uavDesc.Format = DepthFormat;
		mHeaps->CreateUAV(mHiz+std::to_wstring(i), mHeaps->GetResource(mHiz), &uavDesc);
	}
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	rtvDesc.Format = DepthFormat;
	mHeaps->CreateRTV(mHiz, &rtvDesc);
}

BOOL HierachyZBuffer::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
	if (RenderPass::OnResize(newWidth - newWidth%256, newHeight - newHeight%256))
	{
		BuildResources();
		BuildDescriptors();
		return true;
	}
	return false;
}

void HierachyZBuffer::BuildResources()
{
}

void HierachyZBuffer::BuildRootSignatureAndPSO()
{
	CD3DX12_DESCRIPTOR_RANGE srvTable0;
	srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE uavTable0;
	uavTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstants(4, 0);
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable0);
	slotRootParameter[2].InitAsDescriptorTable(1, &uavTable0);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 2> staticSamplers = { linearClamp, pointClamp };
	// A root signature is an array of root parameters.

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
		2, staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	mPSOs->CreateRootSignature(rootSigDesc, rootSig1);
	// PSO for Generate HierachyZBuffer.
	mPSOs->BuildComputePSO(name, rootSig1, "hizCS");
}
