
#include "./RenderPass.h"
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

RenderPass::RenderPass(ComPtr<ID3D12Device> device, 
    std::shared_ptr<DescriptorHeap> heaps, 
    std::shared_ptr<LampPSO> PSOs,
    std::wstring name, UINT srvNum) 
    : SRVNum(srvNum), name(name)
{
    md3dDevice = device;
    mHeaps = heaps;
    mPSOs = PSOs;
}

UINT RenderPass::Width()const
{
    return mWidth;
}

UINT RenderPass::Height()const
{
    return mHeight;
}

BOOL RenderPass::OnResize(UINT newWidth, UINT newHeight, UINT newDepth)
{
	if((mWidth != newWidth) || (mHeight != newHeight) || (mDepth != newDepth))
	{
		mWidth = newWidth;
		mHeight = newHeight;
        mDepth = newDepth;

        mViewport = { 0.0f, 0.0f, (float)mWidth, (float)mHeight, 0.0f, 1.0f };
        mScissorRect = { 0, 0, (int)mWidth, (int)mHeight };
        return true;
	}
    return false;
}