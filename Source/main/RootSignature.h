#pragma once

#include "../D3D/d3dUtil.h"

class LampRootSignature
{
public:
    LampRootSignature(Microsoft::WRL::ComPtr <ID3D12Device> md3dDevice);
    LampRootSignature(const LampRootSignature& rhs) = delete;
    LampRootSignature& operator=(const LampRootSignature& rhs) = delete;
    ~LampRootSignature() = default;

    ID3D12RootSignature* GetRootSignature(std::wstring name);
    void CreateRootSignature(D3D12_ROOT_SIGNATURE_DESC rootSigDesc, std::wstring name);

private:
    Microsoft::WRL::ComPtr <ID3D12Device> md3dDevice;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D12RootSignature>> mRootSignatures;

    void BuildSsaoRootSignature();
    void BuildDebugRootSignature();
    void BuildSSGIRootSignature();
    void BuildVoxelizeRootSignature();
    void BuildCloudRootSignature();
    void BuildPostProcessRootSignature();
    void BuildMipmapRootSignature();
    void BuildBlitRootSignature();
    void BuildTaaRootSignature();
    void BuildComposeVXGIRootSignature();

    void BuildRootSignatures();


    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 5> GetStaticSamplers1();
};