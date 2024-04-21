#include "./Mipmap3D.h"

Mipmap3D::Mipmap3D(ComPtr<ID3D12Device> device,
	std::shared_ptr<DescriptorHeap> heaps,
	std::shared_ptr<LampPSO> PSOs,
	std::wstring target)
	: ScreenRenderPass(device, heaps, PSOs, L"mipmap3D", 0)
{
	mTarget = target;
	pso1 = name;
	rootSig1 = L"mipmap3D";
	mInitialize = false;
	BuildRootSignatureAndPSO();
}

void Mipmap3D::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
	if (!mInitialize && hasTemp)
	{
		std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers = {
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mTemp), GRstate, CSstate),
		CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mTarget), GRstate, CDstate)
		};
		cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

		cmdList->CopyResource(mHeaps->GetResource(mTarget), mHeaps->GetResource(mTemp));
		
		resourceBarriers = {
			CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mTemp), CSstate, GRstate),
			CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mTarget), CDstate, GRstate)
		};
		cmdList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());
	}
	cmdList->SetComputeRootSignature(mPSOs->GetRootSignature(rootSig1));
	cmdList->SetPipelineState(mPSOs->GetPSO(pso1));
	int srcLevel = 0;
	GenerateMipmap3D(cmdList, srcLevel);
	GenerateMipmap3D(cmdList, 3);
}

void Mipmap3D::BuildDescriptors()
{
	for (int i = 0; i < 7; i++)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.MipSlice = i;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = -1;
		uavDesc.Format = mFormat;
		mHeaps->CreateUAV(L"MIP3DUAV"+std::to_wstring(i), mHeaps->GetResource(mTarget), &uavDesc);
	}
}

BOOL Mipmap3D::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
	if (RenderPass::OnResize(newWidth, newHeight, newDepth))
	{
		// BuildResources();
		BuildDescriptors();
		return true;
	}
	return false;
}

void Mipmap3D::BuildResources()
{
	auto texDesc = mHeaps->GetResource(mTarget)->GetDesc();
	mFormat = texDesc.Format;
	
	if (texDesc.Flags != D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		mHeaps->CreateCommitResource3D(mTemp,
			texDesc.Width, texDesc.Height, texDesc.DepthOrArraySize, texDesc.Format,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr, texDesc.MipLevels);
		hasTemp = true;
	}
}

void Mipmap3D::BuildRootSignatureAndPSO()
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
	// PSO for Generate Mipmap3D.
	mPSOs->BuildComputePSO(name, rootSig1, "mipmap3DCS");
}

void Mipmap3D::GenerateMipmap3D(ID3D12GraphicsCommandList* cmdList, UINT srcLevel)
{
	UINT width = (UINT)(mWidth / pow(2, srcLevel));
	UINT height = (UINT)(mHeight / pow(2, srcLevel));
	UINT depth = (UINT)(mDepth / pow(2, srcLevel));
	float data[] = { (float)srcLevel, 1.0f / width, 1.0f / height, 1.0f / depth };

	cmdList->SetComputeRoot32BitConstants(0, 4, data, 0);
	cmdList->SetComputeRootDescriptorTable(1, mHeaps->GetSrv(mTarget));
	cmdList->SetComputeRootDescriptorTable(2, mHeaps->GetGPUUav(L"MIP3DUAV" + std::to_wstring(srcLevel)));

	UINT numGroupsX = (UINT)ceilf(width / 8.0f);
	UINT numGroupsY = (UINT)ceilf(height / 8.0f);
	UINT numGroupsZ = (UINT)ceilf(depth / 8.0f);

	cmdList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}

