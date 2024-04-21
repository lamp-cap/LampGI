#include "./Ssao.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

Ssao::Ssao(ComPtr<ID3D12Device> device,
    std::shared_ptr<DescriptorHeap> heaps,
    std::shared_ptr<LampPSO> PSOs)
   : ScreenRenderPass(device, heaps, PSOs, L"ssao", 4)
{
    rootSig1 = L"ssao";
	BuildOffsetVectors();
}

void Ssao::GetOffsetVectors(DirectX::XMFLOAT4 offsets[14])
{
    std::copy(&mOffsets[0], &mOffsets[14], &offsets[0]);
}

void Ssao::Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)
{
    if (UpdatePerFrame)
    {
        BuildRandomVectorTexture(cmdList);
    }

    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
    cmdList->SetGraphicsRootSignature(mPSOs->GetRootSignature(rootSig1));

    // Change to RENDER_TARGET.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mAmbient0),
        GRstate, RTstate));

    float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    cmdList->ClearRenderTargetView(mHeaps->GetRtv(mAmbient0), clearValue, 0, nullptr);

    // Specify the buffers we are going to render to.
    cmdList->OMSetRenderTargets(1, &mHeaps->GetRtv(mAmbient0), true, nullptr);

    // Bind the constant buffer for this pass.
    auto ssaoCBAddress = currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);
    cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);

    // Bind the normal and depth maps.
    cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->GetSrv(L"Normal"));

    // Bind the random vector map.
    cmdList->SetGraphicsRootDescriptorTable(3, mHeaps->GetSrv(mRandomVector));

    cmdList->SetPipelineState(mPSOs->GetPSO(name));

    // Draw fullscreen quad.
    DrawFullScreen(cmdList);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->GetResource(mAmbient0),
        RTstate, GRstate));

    BlurAmbientMap(cmdList, currFrame, blurCount);
}

std::vector<float> Ssao::CalcGaussWeights(float sigma)
{
    float twoSigma2 = 2.0f*sigma*sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // For example, for sigma = 3, the width of the bell curve is 
    int blurRadius = (int)ceil(2.0f * sigma);

    assert(blurRadius <= MaxBlurRadius);

    std::vector<float> weights;
    weights.resize(2 * blurRadius + 1);

    float weightSum = 0.0f;

    for(int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = (float)i;

        weights[i + blurRadius] = expf(-x*x / twoSigma2);

        weightSum += weights[i + blurRadius];
    }

    // Divide by the sum so all the weights add up to 1.0.
    for(size_t i = 0; i < weights.size(); ++i)
    {
        weights[i] /= weightSum;
    }

    return weights;
}

void Ssao::BuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    srvDesc.Format = LDRFormat;
    mHeaps->CreateSRV(mRandomVector, &srvDesc);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = LDRFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;
    mHeaps->CreateUAV(mRandomVector, &uavDesc);

    srvDesc.Format = AmbientMapFormat;
    mHeaps->CreateSRV(mAmbient0, &srvDesc);
    mHeaps->CreateSRV(mAmbient1, &srvDesc);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    rtvDesc.Format = AmbientMapFormat;
    mHeaps->CreateRTV(mAmbient0, &rtvDesc);
    mHeaps->CreateRTV(mAmbient1, &rtvDesc);
}

BOOL Ssao::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
    if(RenderPass::OnResize(newWidth, newHeight))
    {
        BuildResources();
        BuildDescriptors();
        return true;
    }
    return false;
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame, int blurCount)
{
    cmdList->SetPipelineState(mPSOs->GetPSO(name));

    auto ssaoCBAddress = currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);
 
    for(int i = 0; i < blurCount; ++i)
    {
        BlurAmbientMap(cmdList, true);
        BlurAmbientMap(cmdList, false);
    }
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur)
{
	ID3D12Resource* output = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;
	
	// Ping-pong the two ambient map textures as we apply
	// horizontal and vertical blur passes.
	if(horzBlur == true)
	{
		output = mHeaps->GetResource(mAmbient1);
		inputSrv = mHeaps->GetSrv(mAmbient0);
		outputRtv = mHeaps->GetRtv(mAmbient1);
        cmdList->SetGraphicsRoot32BitConstant(1, 1, 0);
	}
	else
	{
        output = mHeaps->GetResource(mAmbient0);
        inputSrv = mHeaps->GetSrv(mAmbient1);
        outputRtv = mHeaps->GetRtv(mAmbient0);
        cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
	}
 
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output, GRstate, RTstate));

	float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    cmdList->ClearRenderTargetView(outputRtv, clearValue, 0, nullptr);
 
    cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);
	
    // Bind the normal and depth maps.
    cmdList->SetGraphicsRootDescriptorTable(2, mHeaps->GetSrv(L"Normal"));

    // Bind the input ambient map to second texture table.
    cmdList->SetGraphicsRootDescriptorTable(3, inputSrv);
	
	// Draw fullscreen quad.
    DrawFullScreen(cmdList);
   
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output, RTstate, GRstate));
}

void Ssao::BuildResources()
{
    std::array<float, 4> clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    mHeaps->CreateCommitResource2D(mAmbient0, mWidth, mHeight, 
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 1, clearColor);

    mHeaps->CreateCommitResource2D(mAmbient1, mWidth, mHeight, 
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 1, clearColor);
}

void Ssao::BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList)
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = 256;
    texDesc.Height = 256;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    mHeaps->CreateCommitResource2D(
        mRandomVector, 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    //TODO Generate RandomVector Texture with ComputeShader
}
 
void Ssao::BuildOffsetVectors()
{
    // Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
	// and the 6 center points along each cube face.  We always alternate the points on 
	// opposites sides of the cubes.  This way we still get the vectors spread out even
	// if we choose to use less than 14 samples.
	
	// 8 cube corners
	mOffsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	mOffsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	mOffsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	mOffsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	mOffsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	mOffsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	mOffsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	mOffsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	mOffsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	mOffsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

    for(int i = 0; i < 14; ++i)
	{
		// Create random lengths in [0.25, 1.0].
		float s = MathHelper::RandF(0.25f, 1.0f);
		
		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&mOffsets[i]));
		
		XMStoreFloat4(&mOffsets[i], v);
	}
}

void Ssao::BuildRootSignatureAndPSO()
{

}

