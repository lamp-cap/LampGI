#include "./DescriptorHeap.h"


DescriptorHeap::DescriptorHeap(ComPtr <ID3D12Device> d3dDevice,
    ComPtr<ID3D12DescriptorHeap>& RtvHeap,
    ComPtr<ID3D12DescriptorHeap>& DsvHeap,
    UINT SrvDescriptorSize,
    UINT RtvDescriptorSize,
    UINT DsvDescriptorSize,
    UINT SwapChainCount)
{
    md3dDevice = d3dDevice;

    mCbvSrvUavDescriptorSize = SrvDescriptorSize;
    mRtvDescriptorSize = RtvDescriptorSize; 
    mDsvDescriptorSize = DsvDescriptorSize;
    NumSRVDescriptors = 0;
    NumRTVDescriptors = 0;
    mSwapChainCount = SwapChainCount;

    CreateRtvAndDsvDescriptorHeaps(RtvHeap, DsvHeap);
}

void DescriptorHeap::SwapTarget()
{
    Swap = !Swap;
}


void DescriptorHeap::UpdateSwapChain(UINT index)
{
    mCurrentBackBuffer = index;
}

void DescriptorHeap::OutputAllSRV()
{
    std::wstring msg;
    for (const auto& pair : SrvList) {
        msg += pair.first + L"\n";
    }
    OutputDebugString(msg.c_str());
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CurrentBackBuffer()const
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    return hDescriptor.Offset(mCurrentBackBuffer, mRtvDescriptorSize);
}

void DescriptorHeap::BuildSRV(std::shared_ptr<LampGeo> scene,
    ComPtr<ID3D12Resource> depthBuffer)
{
    mScene = scene;
    mDepthStencilBuffer = depthBuffer;
    BuildOffScreenTex();
    BuildDescriptorHeaps();
}

void DescriptorHeap::CreateRtvAndDsvDescriptorHeaps(
    ComPtr<ID3D12DescriptorHeap>& RtvHeap,
    ComPtr<ID3D12DescriptorHeap>& DsvHeap)
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = mSwapChainCount + 32;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(RtvHeap.GetAddressOf())));

    // Add +1 DSV for shadow map.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 2;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(DsvHeap.GetAddressOf())));

    mRtvHeap = RtvHeap;
    mDsvHeap = DsvHeap;
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    NumRTVDescriptors = mSwapChainCount;
    currentRTVHandle = hDescriptor.Offset(NumRTVDescriptors, mRtvDescriptorSize);
    currentDSVHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void DescriptorHeap::BuildDescriptorHeaps()
{
    // Create the SRV heap.
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvHeapDesc.NumDescriptors = 64;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
    mSrvDescriptorHeap->SetName(L"WithoutTexture");
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    // backBuffer
    NumSRVDescriptors = 0;
    currentCPUSRVHandle = hCpuDescriptor;
    currentGPUSRVHandle = hGpuDescriptor;

    std::vector<ComPtr<ID3D12Resource>> tex2DList =
    {
        mScene->TextureRes("bricksDiffuseMap"),
        mScene->TextureRes("bricksNormalMap"),
        mScene->TextureRes("tileDiffuseMap"),
        mScene->TextureRes("tileNormalMap"),
        mScene->TextureRes("defaultDiffuseMap"),
        mScene->TextureRes("defaultNormalMap"),
        mScene->TextureRes("TestMap0")
    };

    std::vector<std::wstring> nameList =
    {
        L"bricksDiffuseMap",
        L"bricksNormalMap",
        L"tileDiffuseMap",
        L"tileNormalMap",
        L"defaultDiffuseMap",
        L"defaultNormalMap",
        L"TestMap0"
    };

    auto skyCubeMap = mScene->TextureRes("skyCubeMap");

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.MipLevels = -1;

    for (UINT i = 0; i < (UINT)tex2DList.size(); ++i)
    {
        srvDesc.Format = tex2DList[i]->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
        CreateSRV(nameList[i], tex2DList[i].Get(), &srvDesc);
    }
    // Sky Cubemap
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    srvDesc.Format = skyCubeMap->GetDesc().Format;
    mNullSrv1 = currentGPUSRVHandle;
    CreateSRV(L"NullCube", &srvDesc);

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = -1;
    srvDesc.Format = LDRFormat;
    mNullSrv2 = currentGPUSRVHandle;
    CreateSRV(L"Null2D1", &srvDesc);
    mNullSrv3 = currentGPUSRVHandle;
    CreateSRV(L"Null2D2", &srvDesc);

    // Last RT
    CreateSRV(mHistory1, &srvDesc);
    CreateSRV(mHistory2, &srvDesc);
    CreateSRV(mOffscreen, &srvDesc);
    CreateSRV(mTemp1, &srvDesc);
    CreateSRV(mTemp2, &srvDesc);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    CreateRTV(mHistory1, &rtvDesc);
    CreateRTV(mHistory2, &rtvDesc);
    CreateRTV(mTemp1, &rtvDesc);
    CreateRTV(mTemp2, &rtvDesc);
    CreateRTV(mOffscreen, &rtvDesc);
    rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    CreateRTV(L"Hiz", &rtvDesc);

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    srvDesc.Format = skyCubeMap->GetDesc().Format;
    CreateSRV(mSky, skyCubeMap.Get(), &srvDesc);
}

void DescriptorHeap::BuildOffScreenTex()
{
    CreateCommitResource2D(mOffscreen, 1280, 768, 
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    CreateCommitResource2D(L"Hiz", 1280, 768,
        DepthFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 7);

    UINT mWidth = 1440;
    UINT mHeight = 810;
    CreateCommitResource2D(mHistory1, mWidth, mHeight,
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    CreateCommitResource2D(mHistory2, mWidth, mHeight,
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    CreateCommitResource2D(mTemp1, mWidth/2, mHeight/2,
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    CreateCommitResource2D(mTemp2, mWidth, mHeight,
        LDRFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
}

void DescriptorHeap::CreateSRV(std::wstring name,
    std::wstring resource,
    D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
    auto cpuHandle = currentCPUSRVHandle;
    SrvList[name] = currentGPUSRVHandle;
    md3dDevice->CreateShaderResourceView(ResList[resource].Get(), desc, cpuHandle);
    currentCPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    currentGPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    ++NumSRVDescriptors;
    std::wstring msg = L"SRV: " + name + L"\n";
    OutputDebugString(msg.c_str());
}

void DescriptorHeap::CreateSRV(std::wstring name,
    D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
    auto cpuHandle = currentCPUSRVHandle;
    SrvList[name] = currentGPUSRVHandle;
    md3dDevice->CreateShaderResourceView(ResList[name].Get(), desc, cpuHandle);
    currentCPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    currentGPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    ++NumSRVDescriptors;
    std::wstring msg = L"SRV: " + name + L"\n";
    OutputDebugString(msg.c_str());
}

void DescriptorHeap::CreateSRV(std::wstring name,
    ID3D12Resource* resource,
    D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
    auto cpuHandle = currentCPUSRVHandle;
    SrvList[name] = currentGPUSRVHandle;
    md3dDevice->CreateShaderResourceView(resource, desc, cpuHandle);
    currentCPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    currentGPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    ++NumSRVDescriptors;
    std::wstring msg = L"SRV: " + name + L"\n";
    OutputDebugString(msg.c_str());
}

void DescriptorHeap::CreateRTV(std::wstring name,
    std::wstring resource,
    D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
    RtvList[name] = currentRTVHandle;
    md3dDevice->CreateRenderTargetView(ResList[resource].Get(), desc, currentRTVHandle);
    currentRTVHandle.Offset(NumRTVDescriptors, mRtvDescriptorSize);
    std::wstring msg = L"RTV: " + name + L"\n";
    OutputDebugString(msg.c_str());
}

void DescriptorHeap::CreateRTV(std::wstring name,
    D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
    RtvList[name] = currentRTVHandle;
    md3dDevice->CreateRenderTargetView(ResList[name].Get(), desc, currentRTVHandle);
    currentRTVHandle.Offset(1, mRtvDescriptorSize);
    std::wstring msg = L"RTV: " + name + L"\n";
    OutputDebugString(msg.c_str());
}

void DescriptorHeap::CreateDSV(std::wstring name,
    D3D12_DEPTH_STENCIL_VIEW_DESC* desc)
{
    DsvList[name] = currentDSVHandle.Offset(1, mDsvDescriptorSize);
    md3dDevice->CreateDepthStencilView(ResList[name].Get(), desc, DsvList.at(name));
}

void DescriptorHeap::CreateUAV(std::wstring name,
    D3D12_UNORDERED_ACCESS_VIEW_DESC* desc)
{
    CPUUavList[name] = currentCPUSRVHandle;
    GPUUavList[name] = currentGPUSRVHandle;
    md3dDevice->CreateUnorderedAccessView(ResList[name].Get(), nullptr, desc, CPUUavList[name]);
    currentCPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    currentGPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    ++NumSRVDescriptors;
    std::wstring msg = L"UAV: " + name + L"\n";
    OutputDebugString(msg.c_str());
}

void DescriptorHeap::CreateUAV(std::wstring name, ID3D12Resource* resource,
    D3D12_UNORDERED_ACCESS_VIEW_DESC* desc)
{
    CPUUavList[name] = currentCPUSRVHandle;
    GPUUavList[name] = currentGPUSRVHandle;
    md3dDevice->CreateUnorderedAccessView(resource, nullptr, desc, CPUUavList[name]);
    currentCPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    currentGPUSRVHandle.Offset(1, mCbvSrvUavDescriptorSize);
    ++NumSRVDescriptors;
    std::wstring msg = L"UAV: " + name + L"\n";
    OutputDebugString(msg.c_str());
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::DepthStencilView()const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void DescriptorHeap::CreateCommitResource2D(
    std::wstring name,
    UINT width, UINT height, 
    DXGI_FORMAT format,
    D3D12_RESOURCE_FLAGS flags, 
    UINT mipLevels,
    std::array<float, 4> clearColor)
{
    CD3DX12_CLEAR_VALUE optClear(format, clearColor.data());
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = mipLevels;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Format = format;
    texDesc.Flags = flags;

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&ResList[name])));
}

void DescriptorHeap::CreateCommitResource2D(std::wstring name,
    UINT width, UINT height, DXGI_FORMAT format,
    D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE* optClear,
    UINT mipLevels)
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = mipLevels;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Format = format;
    texDesc.Flags = flags;

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        optClear,
        IID_PPV_ARGS(&ResList[name])));
}

void DescriptorHeap::CreateCommitResource(std::wstring name,
    D3D12_RESOURCE_DESC resDesc, D3D12_CLEAR_VALUE* optClear, 
    D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_RESOURCE_STATES state)
{
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        pHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        state,
        optClear,
        IID_PPV_ARGS(&ResList[name])));
}

void DescriptorHeap::CreateCommitResource3D(std::wstring name,
    UINT width, UINT height, UINT depth, DXGI_FORMAT format,
    D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE* optClear,
    UINT mipLevels)
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    texDesc.Alignment = 0;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = depth;
    texDesc.MipLevels = mipLevels;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = flags;

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        optClear,
        IID_PPV_ARGS(&ResList[name])));
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::NullSrv()const
{
    return mNullSrv1;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetSrv(std::wstring name)const
{
    return SrvList.at(name);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUUav(std::wstring name)const
{
    return CPUUavList.at(name);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUUav(std::wstring name)const
{
    return GPUUavList.at(name);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetRtv(std::wstring name)const
{
    return RtvList.at(name);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetDsv(std::wstring name)const
{
    return DsvList.at(name);
}

ID3D12Resource* DescriptorHeap::GetResource(std::wstring name)const
{
    return ResList.at(name).Get();
}

ID3D12DescriptorHeap* DescriptorHeap::CustomSRVHeap()const
{
    return mSrvDescriptorHeap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::SkySrv()const
{
    return SrvList.at(mSky);
}
CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::OffscreenSrv()const
{
    return SrvList.at(mOffscreen);
}
CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::OffscreenRtv()const
{
    return RtvList.at(mOffscreen);
}
ID3D12Resource* DescriptorHeap::Offscreen()const
{
    return ResList.at(mOffscreen).Get();
}
ID3D12Resource* DescriptorHeap::Current()const
{
    return Swap ? ResList.at(mHistory1).Get() : ResList.at(mHistory2).Get();
}
ID3D12Resource* DescriptorHeap::History()const
{
    return Swap ? ResList.at(mHistory2).Get() : ResList.at(mHistory1).Get();
}
CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CurrentRtv()const
{
    return Swap ? RtvList.at(mHistory1) : RtvList.at(mHistory2);
}
CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::CurrentSrv()const
{
    return Swap ? SrvList.at(mHistory1) : SrvList.at(mHistory2);
}
CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::HistorySrv()const
{
    return Swap ? SrvList.at(mHistory2) : SrvList.at(mHistory1);
}
ID3D12Resource* DescriptorHeap::Temp1()const
{
    return ResList.at(mTemp1).Get();
}
CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::Temp1Srv()const
{
    return SrvList.at(mTemp1);
}
CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::Temp1Rtv()const
{
    return RtvList.at(mTemp1);
}
ID3D12Resource* DescriptorHeap::Temp2()const
{
    return ResList.at(mTemp2).Get();
}
ID3D12Resource* DescriptorHeap::depthStencilBuffer()const
{
    return mDepthStencilBuffer.Get();
}
CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::Temp2Srv()const
{
    return SrvList.at(mTemp2);
}
CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::Temp2Rtv()const
{
    return RtvList.at(mTemp2);
}