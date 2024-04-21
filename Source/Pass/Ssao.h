#ifndef SSAO_H
#define SSAO_H

#pragma once

#include "ScreenRenderPass.h"
 
class Ssao : public ScreenRenderPass
{
public:
	Ssao(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    Ssao(const Ssao& rhs) = delete;
    Ssao& operator=(const Ssao& rhs) = delete;
    ~Ssao() = default; 

    static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
    static const int MaxBlurRadius = 5;


    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
	BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO()override;
    void BuildResources()override;
    void BuildDescriptors()override;

private:

    std::vector<float> CalcGaussWeights(float sigma);
 
    void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame, int blurCount);
	void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur);

    void BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList);
	void BuildOffsetVectors();
    void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);

private:
    static const BOOL UpdatePerFrame = false;
    const UINT blurCount = 2;

    DirectX::XMFLOAT4 mOffsets[14];

    const std::wstring mAmbient0 = L"ambient0";
    const std::wstring mAmbient1 = L"ambient1";
    const std::wstring mRandomVector = L"randomVector";
};

#endif // SSAO_H