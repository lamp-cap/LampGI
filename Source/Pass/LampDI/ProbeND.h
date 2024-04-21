#pragma once

#include "./LampDI.h"

class ProbeND : public LampDI
{
public:
    ProbeND(ComPtr<ID3D12Device> device,
        std::shared_ptr<DescriptorHeap> heaps,
        std::shared_ptr<LampPSO> PSOs);
    ProbeND(const ProbeND& rhs) = delete;
    ProbeND& operator=(const ProbeND& rhs) = delete;
    ~ProbeND() = default;

    virtual void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;
    BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:

    void BuildRootSignatureAndPSO()override;
    void BuildResources()override;
    void BuildDescriptors()override;
    void Swap();

    std::wstring mCurDI = L"CurDI";
    std::wstring mHisDI = L"HisDI";
    std::wstring mTempDI = L"TempDI";
    std::wstring mNormalDepth = L"ProbeNormalDepth";
    std::wstring ScreenProbeSH0 = L"ScreenProbeSH0";
    std::wstring ScreenProbeSH1 = L"ScreenProbeSH1";
    std::wstring ScreenProbeSH2 = L"ScreenProbeSH2";
private:
};