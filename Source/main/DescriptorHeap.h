#pragma once

#include "./main/geometry.h"

using Microsoft::WRL::ComPtr;

class DescriptorHeap
{
public:
    DescriptorHeap(ComPtr<ID3D12Device> d3dDevice,
                  ComPtr<ID3D12DescriptorHeap>& mRtvHeap,
                  ComPtr<ID3D12DescriptorHeap>& mDsvHeap,
                  UINT SrvDescriptorSize,
                  UINT RtvDescriptorSize,
                  UINT DsvDescriptorSize,
                  UINT SwapChainCount);
    DescriptorHeap(const DescriptorHeap& rhs) = delete;
    DescriptorHeap& operator=(const DescriptorHeap& rhs) = delete;
    ~DescriptorHeap() = default;

    void SwapTarget();
    void UpdateSwapChain(UINT index);

    void OutputAllSRV();

    void BuildSRV(std::shared_ptr<LampGeo> scene,
        ComPtr<ID3D12Resource> depthBuffer);

    void CreateSRV(std::wstring name,  std::wstring resource,
        D3D12_SHADER_RESOURCE_VIEW_DESC* desc);
    void CreateSRV(std::wstring name, D3D12_SHADER_RESOURCE_VIEW_DESC* desc);
    void CreateSRV(std::wstring name, ID3D12Resource* resource,
        D3D12_SHADER_RESOURCE_VIEW_DESC* desc);

    void CreateRTV(std::wstring name, std::wstring resource, D3D12_RENDER_TARGET_VIEW_DESC* desc);
    void CreateRTV(std::wstring name, D3D12_RENDER_TARGET_VIEW_DESC* desc);
    void CreateDSV(std::wstring name, D3D12_DEPTH_STENCIL_VIEW_DESC* desc);
    void CreateUAV(std::wstring name, D3D12_UNORDERED_ACCESS_VIEW_DESC* desc);
    void CreateUAV(std::wstring name, ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC* desc);
    
    void CreateCommitResource2D(std::wstring name,
        UINT width, UINT height, DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags, UINT mipLevels = 1,
        std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 0.0f});

    void CreateCommitResource2D(std::wstring name,
        UINT width, UINT height, DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE* optClear,
        UINT mipLevels = 1);

    void CreateCommitResource3D(std::wstring name,
        UINT width, UINT height, UINT depth, DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE* optClear,
        UINT mipLevels = 1);

    void CreateCommitResource(std::wstring name,
        D3D12_RESOURCE_DESC resDesc, D3D12_CLEAR_VALUE* optClear, 
        D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_RESOURCE_STATES state);

    ID3D12Resource* GetResource(std::wstring name)const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv(std::wstring name)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUUav(std::wstring name)const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUUav(std::wstring name)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(std::wstring name)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(std::wstring name)const;
    ID3D12DescriptorHeap* CustomSRVHeap()const;

    CD3DX12_CPU_DESCRIPTOR_HANDLE CurrentBackBuffer()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE SkySrv()const;
    ID3D12Resource* Offscreen()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE OffscreenSrv()const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE OffscreenRtv()const;
    ID3D12Resource* Current()const;
    ID3D12Resource* History()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE HistorySrv()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE CurrentSrv()const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE CurrentRtv()const;
    ID3D12Resource* Temp1()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE Temp1Srv()const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Temp1Rtv()const;
    ID3D12Resource* Temp2()const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;
    ID3D12Resource* depthStencilBuffer()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE Temp2Srv()const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Temp2Rtv()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE NullSrv()const;

    static constexpr DXGI_FORMAT ScreenFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT LDRFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	static constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_R32_FLOAT;

private:
    ComPtr<ID3D12Device> md3dDevice;
    UINT NumTexSRVDescriptors;
    UINT NumSRVDescriptors;
    UINT NumRTVDescriptors;
    UINT mSwapChainCount;
    UINT mCurrentBackBuffer;

    UINT mCbvSrvUavDescriptorSize;
    UINT mRtvDescriptorSize;
    UINT mDsvDescriptorSize;

    std::shared_ptr<LampGeo> mScene;

    BOOL Swap;

    std::unordered_map<std::wstring, CD3DX12_GPU_DESCRIPTOR_HANDLE> SrvList;
    std::unordered_map<std::wstring, CD3DX12_CPU_DESCRIPTOR_HANDLE> CPUUavList;
    std::unordered_map<std::wstring, CD3DX12_GPU_DESCRIPTOR_HANDLE> GPUUavList;
    std::unordered_map<std::wstring, CD3DX12_CPU_DESCRIPTOR_HANDLE> DsvList;
    std::unordered_map<std::wstring, CD3DX12_CPU_DESCRIPTOR_HANDLE> RtvList;
    std::unordered_map<std::wstring, ComPtr<ID3D12Resource> > ResList;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUSRVHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE currentGPUSRVHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTVHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE currentDSVHandle;

    static constexpr LPCWSTR mOffscreen = L"Offscreen";
    static constexpr LPCWSTR mHistory1 = L"History1";
    static constexpr LPCWSTR mHistory2 = L"History2";
    static constexpr LPCWSTR mTemp1 = L"Temp1";
    static constexpr LPCWSTR mTemp2 = L"Temp2";
    static constexpr LPCWSTR mSky = L"Sky";

    ComPtr<ID3D12Resource> mDepthStencilBuffer = nullptr;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv1;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv2;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv3;

    void BuildDescriptorHeaps();
    void BuildOffScreenTex();
    void CreateRtvAndDsvDescriptorHeaps(
        ComPtr<ID3D12DescriptorHeap>& mRtvHeap,
        ComPtr<ID3D12DescriptorHeap>& mDsvHeap);
};