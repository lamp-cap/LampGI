#include "ShadowMap.h"

Shadow::Shadow(ComPtr<ID3D12Device> device,
	std::shared_ptr<DescriptorHeap> heaps,
	std::shared_ptr<LampPSO> PSOs,
	std::shared_ptr<LampGeo> Scene)
	: SceneRenderPass(device, heaps, PSOs, Scene, L"shadow_opaque", 3)
{
	pso1 = name;
	rootSig1 = L"shadow";
	BuildRootSignatureAndPSO();
}

void Shadow::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
	cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	auto passCB = currFrame->PassCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	auto matBuffer = currFrame->MaterialBuffer->Resource();
	cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
	// Bind null SRV for shadow map pass.
	cmdList->SetGraphicsRootDescriptorTable(3,  mHeaps->CustomSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	// Change to DEPTH_WRITE.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mShadow), GRstate, DWstate));

	// Clear the depth buffer.
	cmdList->ClearDepthStencilView(mHeaps->GetDsv(mShadow),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	cmdList->OMSetRenderTargets(0, nullptr, false, &mHeaps->GetDsv(mShadow));

	// Bind the pass constant buffer for the shadow map pass.
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	cmdList->SetPipelineState(mPSOs->GetPSO(pso1));

	DrawRenderItems(cmdList, RenderLayer::Opaque, currFrame);
	DrawRenderItems(cmdList, RenderLayer::Wall, currFrame);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mShadow), DWstate, GRstate));
}

BOOL Shadow::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
	if (RenderPass::OnResize(newWidth, newHeight))
	{
		BuildResources();
		BuildDescriptors();
		return true;
	}
	return false;
}

void Shadow::BuildDescriptors()
{
    // Create SRV to resource so we can sample the shadow map in a shader program.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; 
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;
    mHeaps->CreateSRV(mShadow, &srvDesc);
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	mHeaps->CreateSRV(mColorLS, &srvDesc);
	mHeaps->CreateSRV(mNormalLS, &srvDesc);

	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Texture2D.MipSlice = 0;
	mHeaps->CreateDSV(mShadow, &dsvDesc);


	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	mHeaps->CreateRTV(mColorLS, &rtvDesc);
	mHeaps->CreateRTV(mNormalLS, &rtvDesc);
}

void Shadow::BuildResources()
{
	// printf("***** Shadow BuildResource ****");
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

	mHeaps->CreateCommitResource2D(mShadow, mWidth, mHeight, mFormat, 
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &optClear);

	mHeaps->CreateCommitResource2D(mNormalLS, mRSMWidth, mRSMHeight, 
		DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	mHeaps->CreateCommitResource2D(mColorLS, mRSMWidth, mRSMHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
}

void Shadow::BuildRootSignatureAndPSO()
{

	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0, 0);


	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = mPSOs->GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	mPSOs->CreateRootSignature(rootSigDesc, rootSig1);

	std::vector<DXGI_FORMAT> rtvFormats = { LDRFormat, LDRFormat };
	CD3DX12_RASTERIZER_DESC RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.DepthBias = 100000;
	RasterizerState.DepthBiasClamp = 0.0f;
	RasterizerState.SlopeScaledDepthBias = 1.0f;
	mPSOs->BuildGraphicsPSO(pso1, rootSig1, "shadowVS", "shadowOpaquePS",
		rtvFormats, CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT), RasterizerState);
}