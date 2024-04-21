#include "App.h"
#include "Pass/GBuffer.h"
#include "Pass/DeferLighting.h"
#include "Pass/Voxelize.h"
#include "Pass/VXGI.h"
#include "Pass/TAA.h"
#include "Pass/Mipmap3D.h"
#include "Pass/WorldProbe.h"
#include "Pass/WorldProbeDebug.h"
#include "Pass/ProbeGI.h"
#include "Pass/SSGI.h"
//#include "./Pass/Sobel.h"
//#include "./Pass/Mipmap.h"
#include "Pass/Hiz.h"
#include "Pass/Blit.h"
#include "Pass/LampDI/ProbeND.h"
#include "Pass/LampDI/ScreenDI.h"
#include "Pass/LampDI/VoxelDI.h"
#include "Pass/LampDI/ProbeDI.h"
#include "Pass/LampDI/ScreenProbeSH.h"
#include "Pass/LampDI/Reflection.h"
#include  "Pass//LampDI/CompositionDI.h"

LampApp::LampApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
    // Estimate the scene bounding sphere manually since we know how the scene was constructed.
    // The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
    // the world space origin.  In general, you need to loop over every world space vertex
    // position and compute the bounding sphere.
    mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
    mLastMousePos.x = 0;
    mLastMousePos.y = 0;
    mLightPosW = XMFLOAT3(0.0f, 0.0f, 0.0f);
    mRotatedLightDirections[0] = XMFLOAT3(0.0f, 0.0f, 0.0f);
    mRotatedLightDirections[1] = XMFLOAT3(0.0f, 0.0f, 0.0f);
    mRotatedLightDirections[2] = XMFLOAT3(0.0f, 0.0f, 0.0f);
    isStart = true;
}

LampApp::~LampApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
    mFrameResources.clear();
}

bool LampApp::Initialize()
{
    if (!D3DApp::InitMainWindow())
        return false;

    if (!D3DApp::InitDirect3D())
        return false;

    // Do the initial resize code.
    OnResize();

    InitialGraphics();

    return true;
}

void LampApp::InitialGraphics() 
{
    mCamera.SetPosition(0.0f, 2.0f, -10.0f);
    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mPSO = std::make_shared<LampPSO>(md3dDevice, m4xMsaaState, m4xMsaaQuality);
    mScene = std::make_shared<LampGeo>(md3dDevice);

    mPasses.push_back(std::make_unique<Shadow>(md3dDevice, mHeaps, mPSO, mScene));
    mPasses.push_back(std::make_unique<GBuffer>(md3dDevice, mHeaps, mPSO, mScene));
    mPasses.push_back(std::make_unique<DeferLighting>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<Voxel>(md3dDevice, mHeaps, mPSO, mScene));
    mPasses.push_back(std::make_unique<TAA>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<Mipmap3D>(md3dDevice, mHeaps, mPSO, L"VoxelizedColor"));
    mPasses.push_back(std::make_unique<HierachyZBuffer>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<Blit>(md3dDevice, mHeaps, mPSO));

    mPasses.push_back(std::make_unique<WorldProbe>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<ProbeND>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<ScreenDI>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<VoxelDI>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<ProbeDI>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<ScreenProbeSH>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<LampReflection>(md3dDevice, mHeaps, mPSO));
    mPasses.push_back(std::make_unique<CompositionDI>(md3dDevice, mHeaps, mPSO));


    mScene->LoadScene(mCommandList.Get());
    mHeaps->BuildSRV(mScene, mDepthStencilBuffer);
    mPasses[0]->OnResize(2048, 2048); // Shadow
    mPasses[1]->OnResize(mClientWidth, mClientHeight); // GBuffer
    mPasses[2]->OnResize(mClientWidth, mClientHeight); // DeferLighting
    mPasses[3]->OnResize(256, 256, 256); // Voxelize
    mPasses[4]->OnResize(mClientWidth, mClientHeight); // TAA
    mPasses[5]->OnResize(256, 128, 256); // Mipmap3D
    mPasses[6]->OnResize(mClientWidth, mClientHeight); // Hiz

    mPasses[8]->OnResize(1, 1); // World Probe
    mPasses[9]->OnResize(1, 1); // Probe Normal Depth
    mPasses[10]->OnResize(1, 1); // SSDI
    mPasses[11]->OnResize(1, 1); // VXDI
    mPasses[12]->OnResize(1, 1); // ProbeDI
    mPasses[13]->OnResize(1, 1); // ProbeSH
    mPasses[14]->OnResize(mClientWidth, mClientHeight); // Reflection
    mPasses[15]->OnResize(mClientWidth, mClientHeight); // Composite
    
    BuildFrameResources();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();
}

void LampApp::OnResize()
{
    D3DApp::OnResize();

    mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void LampApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
    mHeaps->SwapTarget();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        if (eventHandle != NULL)
        {
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }

    //
    // Animate the lights (and hence shadows).
    //

    mLightRotationAngle += 0.1f * gt.DeltaTime();

    XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
    for (int i = 0; i < 3; ++i)
    {
        XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
        lightDir = XMVector3TransformNormal(lightDir, R);
        XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
    }

    AnimateMaterials(gt);
    UpdateObjectCBs(gt);
    UpdateMaterialBuffer(gt);
    UpdateShadowTransform(gt);
    UpdateMainPassCB(gt);
    UpdateShadowPassCB(gt);
    UpdateSsaoCB(gt);
    UpdateTaaCB(gt);
}

void LampApp::CreateRtvAndDsvDescriptorHeaps()
{
    mHeaps = std::make_shared<DescriptorHeap>(
        md3dDevice,
        mRtvHeap,
        mDsvHeap,
        mCbvSrvUavDescriptorSize,
        mRtvDescriptorSize,
        mDsvDescriptorSize,
        SwapChainBufferCount);
}

void LampApp::BuildFrameResources()
{
    for (int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            2, (UINT)mScene->mAllRitems.size(), (UINT)mScene->mMaterials.size()));
    }
}

void LampApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    ThrowIfFailed(cmdListAlloc->Reset());
    DrawPrePass(cmdListAlloc);

    Voxelize(cmdListAlloc);
    /*
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), nullptr));

    ID3D12DescriptorHeap* descriptorHeaps[] = { mHeaps->CustomSRVHeap() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    
    // mPasses[8]->Draw(mCommandList.Get(), mCurrFrameResource);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    */
    /*
    if (isStart)
    {
        FlushCommandQueue();

        // Map the data so we can read it on CPU.
        void* mappedData = nullptr;
        ThrowIfFailed(mHeaps->GetResource(L"WorldProbeDebug")->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

        // std::ofstream fout("results.txt");
        std::vector<UINT> array1;
        UINT num = 0;
        for (int i = 0; i < 128; ++i)
        {
            for (int j = 0; j < 128; ++j)
            {
                uint8_t* pPixelData = reinterpret_cast<uint8_t*>(mappedData) + i * 128 * 8 + j * 8;
                if (pPixelData[3] & 255)
                {
                    fout << "( " << (int)pPixelData[0]
                        << ", " << (int)pPixelData[1]
                        << ", " << (int)pPixelData[2]
                        << ", " << (int)pPixelData[3]
                        << " )" << std::endl;
                    array1.push_back(i * 128 + j);
                    num++;
                }
            }
        }
        // fout << "#: " << num << " in " << 128*128 << " k= " << std::endl;

        mHeaps->GetResource(L"WorldProbeDebug")->Unmap(0, nullptr);
    }*/

    DrawScreenPass(cmdListAlloc);

    BlitToScreenPass(cmdListAlloc);

    isStart = false;
    mLastView = mCamera.GetView();
    mLastProj = mCamera.GetProj();

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    // ThrowIfFailed(mSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void LampApp::DrawPrePass(ComPtr<ID3D12CommandAllocator> cmdAlloc)
{
    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSO->GetPSO(L"opaque")));

    ID3D12DescriptorHeap* descriptorHeaps[] = { mHeaps->CustomSRVHeap() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // ForwardRenderPass();
    // DrawSceneToShadowMap();
    mPasses[0]->Draw(mCommandList.Get(), mCurrFrameResource); // MainLightShadow
    mPasses[1]->Draw(mCommandList.Get(), mCurrFrameResource); // GBuffer
    mPasses[6]->Draw(mCommandList.Get(), mCurrFrameResource); // Hiz

    mPasses[9]->Draw(mCommandList.Get(), mCurrFrameResource); // Probe Normal Depth
    // DrawSceneToRSM();
    // Normal/depth pass.
    // DrawNormalsAndDepth();
    // DrawGBuffer();
    // Add the command list to the queue for execution.
    // 
    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void LampApp::Voxelize(ComPtr<ID3D12CommandAllocator> cmdAlloc)
{
    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), nullptr));

    ID3D12DescriptorHeap* descriptorHeaps[] = { mHeaps->CustomSRVHeap() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mPasses[3]->Draw(mCommandList.Get(), mCurrFrameResource); // Voxelize
    mPasses[5]->Draw(mCommandList.Get(), mCurrFrameResource); // Mipmap3D
    mPasses[8]->Draw(mCommandList.Get(), mCurrFrameResource); // WorldProbe

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void LampApp::DrawScreenPass(ComPtr<ID3D12CommandAllocator> cmdAlloc)
{
    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), nullptr));

    ID3D12DescriptorHeap* descriptorHeaps[] = { mHeaps->CustomSRVHeap() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Compute SSAO.
    // SetRootSignature(L"ssao");
    // mSsao->ComputeSsao(mCommandList.Get(), mCurrFrameResource, 2);

    // Main rendering pass.
    mPasses[10]->Draw(mCommandList.Get(), mCurrFrameResource); // ssdi
    mPasses[11]->Draw(mCommandList.Get(), mCurrFrameResource); // vxdi
    mPasses[12]->Draw(mCommandList.Get(), mCurrFrameResource); // probedi
    mPasses[13]->Draw(mCommandList.Get(), mCurrFrameResource); // probe 2 SH
    // mPasses[9]->Draw(mCommandList.Get(), mCurrFrameResource); // Probe gi
    // mPasses[4]->Draw(mCommandList.Get(), mCurrFrameResource); // vxgi
    mPasses[2]->Draw(mCommandList.Get(), mCurrFrameResource); // DeferLighting

    mPasses[14]->Draw(mCommandList.Get(), mCurrFrameResource); // reflection
    mPasses[15]->Draw(mCommandList.Get(), mCurrFrameResource); // composite
    //DrawSky();

    // Indicate a state transition on the resource usage.
    //if (false)
    {
        // copy screen to Offscreen
        // BlitScreenColor(true);

        //
        // PostProcessing
        // mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);
        // DrawSobel();
        // DrawCloud();
        //DrawVXGI();

        // DrawSSR();
        //if (!isStart) DrawTAA();
        // copy screen to history
        //Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        //Transition(mHistory.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);

        //mCommandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(mHistory.Get(), 0), 0, 0, 0,
        //    &CD3DX12_TEXTURE_COPY_LOCATION(CurrentBackBuffer(), 0), nullptr);
        // Change offscreen texture to be used as an input.
        //Transition(mHistory.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
        //Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void LampApp::ForwardRenderPass()
{
    // Shadow map pass.
    SetRootSignature(L"main");

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Clear the back buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    ClearDepthStencil(DepthStencilView());
    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    // Bind all the textures used in this scene.
    mCommandList->SetGraphicsRootDescriptorTable(4, mHeaps->CustomSRVHeap()->GetGPUDescriptorHandleForHeapStart());

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
    mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

    mCommandList->SetGraphicsRootDescriptorTable(3, mHeaps->SkySrv());

    mCommandList->SetPipelineState(mPSO->GetPSO(L"opaque"));
    DrawRenderItems(mCommandList.Get(), mScene->RenderItems(RenderLayer::Opaque));
    DrawRenderItems(mCommandList.Get(), mScene->RenderItems(RenderLayer::Debug));
    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void LampApp::BlitToScreenPass(ComPtr<ID3D12CommandAllocator> cmdAlloc)
{
    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), nullptr));

    ID3D12DescriptorHeap* descriptorHeaps[] = { mHeaps->CustomSRVHeap() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    D3D12_RESOURCE_STATES GRstate = D3D12_RESOURCE_STATE_GENERIC_READ;
    D3D12_RESOURCE_STATES PRstate = D3D12_RESOURCE_STATE_PRESENT;
    D3D12_RESOURCE_STATES RTstate = D3D12_RESOURCE_STATE_RENDER_TARGET;
    D3D12_RESOURCE_STATES CDstate = D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_RESOURCE_STATES CSstate = D3D12_RESOURCE_STATE_COPY_SOURCE;

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), PRstate, RTstate));
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), false, nullptr);

    mPasses[4]->Draw(mCommandList.Get(), mCurrFrameResource); // TAA

    std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), RTstate, CSstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Current(), GRstate, CDstate)
    };

    mCommandList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());
    mCommandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(mHeaps->Current(), 0),
        0, 0, 0, &CD3DX12_TEXTURE_COPY_LOCATION(CurrentBackBuffer(), 0), nullptr);

    resourceBarriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), CSstate, RTstate),
        CD3DX12_RESOURCE_BARRIER::Transition(mHeaps->Current(), CDstate, GRstate)
    };
    mCommandList->ResourceBarrier(resourceBarriers.size(), resourceBarriers.data());

    // Debug Layer
    if (showDebugView)
    {
        SetRootSignature(L"debug");
        CD3DX12_GPU_DESCRIPTOR_HANDLE DebugDescriptor(mHeaps->CustomSRVHeap()->GetGPUDescriptorHandleForHeapStart());
        DebugDescriptor.Offset(2, mCbvSrvUavDescriptorSize);
        auto passCB = mCurrFrameResource->PassCB->Resource();
        mCommandList->SetGraphicsRootDescriptorTable(2, mHeaps->CustomSRVHeap()->GetGPUDescriptorHandleForHeapStart());
        mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

        SetPSO(L"debug");
        DrawRenderItems(mCommandList.Get(), mScene->RenderItems(RenderLayer::Debug));
    }
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), RTstate, PRstate));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void LampApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    auto objectCB = mCurrFrameResource->ObjectCB->Resource();

    // For each render item...
    for (size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (UINT64)ri->ObjCBIndex * objCBByteSize;

        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

// RSM
/* 
void LampApp::DrawSceneToRSM()
{
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 256.0f, 144.0f, 0.0f, 1.0f };
    RSSetViewportsAndScissorRect(&viewport, &mShadowMap->ScissorRect());

    Transition(mShadowMap->ColorResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    Transition(mShadowMap->NormalResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    // Clear the back buffer and depth buffer.
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    //float clearColor1[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    ClearRenderTarget(mShadowMap->ColorRtv(), clearColor);
    ClearRenderTarget(mShadowMap->NormalRtv(), clearColor);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(2, &mShadowMap->ColorRtv(), true, nullptr);

    // Bind the pass constant buffer for the shadow map pass.
    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
    auto passCB = mCurrFrameResource->PassCB->Resource();
    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
    mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

    SetPSO(L"RSM");

    DrawRenderItems(mCommandList.Get(), mScene->RenderItems(RenderLayer::Opaque));

    Transition(mShadowMap->ColorResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
    Transition(mShadowMap->NormalResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
}
*/
// normal and Depth
/*
void LampApp::DrawNormalsAndDepth()
{
    RSSetViewportsAndScissorRect(&mScreenViewport, &mScissorRect);

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    Transition(mSsao->NormalMap(), state, true);


    float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    ClearRenderTarget(mSsao->NormalMapRtv(), clearValue);
    ClearDepthStencil(DepthStencilView());

    mCommandList->OMSetRenderTargets(1, &mSsao->NormalMapRtv(), true, &DepthStencilView());

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    SetPSO(L"drawNormals");

    DrawRenderItems(mCommandList.Get(), mScene->RenderItems(RenderLayer::Opaque));

    // Change back to GENERIC_READ so we can read the texture in a shader.
    Transition(mSsao->NormalMap(), state, false);
}

*/
// sky
/*
void LampApp::DrawSky()
{
    SetRootSignature(L"main");

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    // Bind the sky cube map. 
    mCommandList->SetGraphicsRootDescriptorTable(3, mHeaps->SkySrv());

    SetPSO(L"sky");
    DrawRenderItems(mCommandList.Get(), mScene->RenderItems(RenderLayer::Sky));
}

*/
// sobel
/*
void LampApp::DrawSobel()
{
    mSobel->DrawSobel(mCommandList.Get(), mPSO->GetRootSignature(L"sobel"),
        mPSO->GetPSO(L"sobel"), mHeaps->OffscreenSrv());

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);
    SetRootSignature(L"sobel");

    SetPSO(L"composite");

    mCommandList->SetGraphicsRootDescriptorTable(0, mHeaps->OffscreenSrv());
    mCommandList->SetGraphicsRootDescriptorTable(1, mSobel->Srv());
    DrawFullscreenQuad(mCommandList.Get());
}

*/
// voxel
/*
void LampApp::Voxelize(ComPtr<ID3D12CommandAllocator> cmdAlloc)
{
    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSO->GetPSO(L"opaque")));

    ID3D12DescriptorHeap* descriptorHeaps[] = { mHeaps->TextureSRVHeap() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    SetRootSignature(L"voxelize");
    RSSetViewportsAndScissorRect(&mVoxel->Viewport(), &mVoxel->ScissorRect());

    Transition(mVoxel->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Transition(mVoxel->tempResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    // Clear the back buffer and depth buffer.
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ClearRenderTarget(mVoxel->TempRtv(), clearColor);
    mCommandList->ClearUnorderedAccessViewFloat(mVoxel->VoxelGpuUav(), mVoxel->VoxelCpuUav(), mVoxel->Resource(), clearColor, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &mVoxel->TempRtv(), true, nullptr);
    // textures
    mCommandList->SetGraphicsRootDescriptorTable(5, mHeaps->TextureSRVHeap()->GetGPUDescriptorHandleForHeapStart());

    auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
    mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    mCommandList->SetGraphicsRootDescriptorTable(4, mHeaps->SkySrv());
    mCommandList->SetGraphicsRootDescriptorTable(3, mVoxel->VoxelGpuUav());

    SetPSO(L"voxelize");

    DrawRenderItems(mCommandList.Get(), mScene->RenderItems(RenderLayer::Opaque));

    Transition(mVoxel->Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
    Transition(mVoxel->tempResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}
*/
// cloud
/*
void LampApp::DrawCloud()
{
    SetRootSignature(L"cloud");
    // mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);

    SetPSO(L"cloud");

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootDescriptorTable(1, mHeaps->OffscreenSrv());
    mCommandList->SetGraphicsRootDescriptorTable(2, mSsao->NormalMapSrv());
    mCommandList->SetGraphicsRootDescriptorTable(3, mVoxel->VoxelSrv());

    DrawFullscreenQuad(mCommandList.Get());
}
*/
// vxgi
/*
void LampApp::DrawVXGI()
{
    SetRootSignature(L"cloud");
    Transition(mHeaps->Temp1(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->OMSetRenderTargets(1, &mHeaps->Temp1Rtv(), true, nullptr);
    SetPSO(L"vxgi");

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootDescriptorTable(1, mHeaps->SkySrv());
    mCommandList->SetGraphicsRootDescriptorTable(2, mSsao->NormalMapSrv());
    mCommandList->SetGraphicsRootDescriptorTable(3, mVoxel->VoxelSrv());

    DrawFullscreenQuad(mCommandList.Get());
    Transition(mHeaps->Temp1(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    
    SetRootSignature(L"composeVXGI");
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);
    SetPSO(L"compositeVXGI");
    mCommandList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootDescriptorTable(1, mHeaps->Temp1Srv());
    mCommandList->SetGraphicsRootDescriptorTable(2, mHeaps->OffscreenSrv());
    mCommandList->SetGraphicsRootDescriptorTable(3, mSsao->NormalMapSrv());

    DrawFullscreenQuad(mCommandList.Get());
}

*/
// ssr
/*
void LampApp::DrawSSR()
{
    // copy screen to Offscreen
    Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(mHeaps->Offscreen(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);

    mCommandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(mHeaps->Offscreen(), 0), 0, 0, 0,
        &CD3DX12_TEXTURE_COPY_LOCATION(CurrentBackBuffer(), 0), nullptr);
    // Change offscreen texture to be used as an input.
    Transition(mHeaps->Offscreen(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    SetRootSignature(L"ssgi");
    // mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);

    SetPSO(L"ssgi");

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootDescriptorTable(1, mHeaps->OffscreenSrv());
    mCommandList->SetGraphicsRootDescriptorTable(2, mSsao->AmbientMapSrv());
    mCommandList->SetGraphicsRootDescriptorTable(3, mSsao->NormalMapSrv());

    DrawFullscreenQuad(mCommandList.Get());
}

*/
// taa
/*
void LampApp::DrawTAA()
{
    // copy screen to Offscreen
    Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(mHeaps->Offscreen(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);

    mCommandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(mHeaps->Offscreen(), 0), 0, 0, 0,
        &CD3DX12_TEXTURE_COPY_LOCATION(CurrentBackBuffer(), 0), nullptr);
    // Change offscreen texture to be used as an input.
    Transition(mHeaps->Offscreen(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    SetRootSignature(L"taa");
    // mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);

    SetPSO(L"taa");

    auto taaCB = mCurrFrameResource->TaaCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(0, taaCB->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootDescriptorTable(1, mHeaps->HistorySrv());
    mCommandList->SetGraphicsRootDescriptorTable(2, mSsao->DepthSrv());

    DrawFullscreenQuad(mCommandList.Get());
}

*/
// blit
/*
void LampApp::BlitScreenColor(bool generateMipmaps)
{
    // copy screen to Offscreen
    Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    Transition(mHeaps->Offscreen(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // mCommandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(mOffscreen.Get(), 0), 0, 0, 0,
    //    &CD3DX12_TEXTURE_COPY_LOCATION(CurrentBackBuffer(), 0), nullptr);
    SetRootSignature(L"blit");
    mCommandList->OMSetRenderTargets(1, &mHeaps->OffscreenRtv(), true, nullptr);
    //mCommandList->SetGraphicsRootDescriptorTable(0, CurrentBackBufferView());
    SetPSO(L"blit");

    DrawFullscreenQuad(mCommandList.Get());
    // Change offscreen texture to be used as an input.
    Transition(mHeaps->Offscreen(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // copy depth to hiz
    Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
    Transition(mSsao->HizMap(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    
    SetRootSignature(L"blit");
    mCommandList->OMSetRenderTargets(1, &mSsao->HizRtv(), true, nullptr);
    mCommandList->SetGraphicsRootDescriptorTable(0, mSsao->DepthSrv());
    SetPSO(L"blit");
    
    DrawFullscreenQuad(mCommandList.Get());
    
    Transition(mSsao->HizMap(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    if (generateMipmaps)
    {
        mMipmap->SetPSO(mPSO->GetPSO(L"mipmap"), mPSO->GetRootSignature(L"mipmap"), mCbvSrvUavDescriptorSize);
        mMipmap->GenerateMipmap(mCommandList.Get(),
            mHeaps->Offscreen(),
            mHeaps->GetGpuSrv(mOffScreenRTIndex),
            mHeaps->GetCpuSrv(mMipmapHeapIndex),
            mHeaps->GetGpuSrv(mMipmapHeapIndex));

        mHiz->SetPSO(mPSO->GetPSO(L"hiz"), mPSO->GetRootSignature(L"mipmap"), mCbvSrvUavDescriptorSize);
        mHiz->GenerateHiZ(mCommandList.Get(),
            mSsao->HizMap(),
            mSsao->HizSrv(),
            mHeaps->GetCpuSrv(mMipmapHeapIndex + 8),
            mHeaps->GetGpuSrv(mMipmapHeapIndex + 8));
    }
    //if(isStart)
    {
        mMipmap3D->SetPSO(mPSO->GetPSO(L"mipmap3D"), mPSO->GetRootSignature(L"mipmap"), mCbvSrvUavDescriptorSize);
        mMipmap3D->GenerateMipmap3D(mCommandList.Get(),
            mVoxel->Resource(),
            mVoxel->VoxelSrv(),
            mHeaps->GetCpuSrv(mMipmapHeapIndex + 16),
            mHeaps->GetGpuSrv(mMipmapHeapIndex + 16));
    }
}
*/
void LampApp::DrawFullscreenQuad(ID3D12GraphicsCommandList* cmdList)
{
    // Null-out IA stage since we build the vertex off the SV_VertexID in the shader.
    cmdList->IASetVertexBuffers(0, 1, nullptr);
    cmdList->IASetIndexBuffer(nullptr);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawInstanced(6, 1, 0, 0);
}

void LampApp::SetPSO(std::wstring name)
{
    mCommandList->SetPipelineState(mPSO->GetPSO(name));
}

void LampApp::SetRootSignature(std::wstring name)
{
    mCommandList->SetGraphicsRootSignature(mPSO->GetRootSignature(name));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE LampApp::GetRtv(int index)const
{
    auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtv.Offset(index, mRtvDescriptorSize);
    return rtv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE LampApp::GetDsv(int index)const
{
    auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
    dsv.Offset(index, mDsvDescriptorSize);
    return dsv;
}
