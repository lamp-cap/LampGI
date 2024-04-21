#pragma once

#include "./D3D/d3dApp.h"
#include "./envir/Camera.h"
#include "./main/geometry.h"
#include "Pass/ShadowMap.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

class LampApp : public D3DApp
{
public:
    LampApp(HINSTANCE hInstance);
    LampApp(const LampApp& rhs) = delete;
    LampApp& operator=(const LampApp& rhs) = delete;
    ~LampApp();

    virtual bool Initialize()override;

private:
    virtual void CreateRtvAndDsvDescriptorHeaps()override;
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void AnimateMaterials(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateMaterialBuffer(const GameTimer& gt);
    void UpdateShadowTransform(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateShadowPassCB(const GameTimer& gt);
    void UpdateSsaoCB(const GameTimer& gt);
    void UpdateTaaCB(const GameTimer& gt);

    void InitialGraphics();
    void BuildFrameResources();

    void DrawPrePass( Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAlloc);
    void Voxelize(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAlloc);
    void DrawScreenPass( Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAlloc);
    void BlitToScreenPass(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAlloc);
    void ForwardRenderPass();

    void DrawFullscreenQuad(ID3D12GraphicsCommandList* cmdList);
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

    void SetPSO(std::wstring name);
    void SetRootSignature(std::wstring name);

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    BOOL showDebugView = false;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::vector<std::unique_ptr<RenderPass>> mPasses;

    UINT mOffScreenRTIndex = 0;
    UINT mSkyTexHeapIndex = 0;
    UINT mShadowMapHeapIndex = 0;
    UINT mSsaoHeapIndexStart = 0;
    UINT mSobelHeapIndex = 0;
    UINT mVoxelHeapIndex = 0;
    UINT mMipmapHeapIndex = 0;

    UINT mNullCubeSrvIndex = 0;
    UINT mNullTexSrvIndex1 = 0;
    UINT mNullTexSrvIndex2 = 0;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

    PassConstants mMainPassCB;  // index 0 of pass cbuffer.
    PassConstants mShadowPassCB;// index 1 of pass cbuffer.

    Camera mCamera;

    std::shared_ptr<DescriptorHeap> mHeaps;

    std::shared_ptr<LampPSO> mPSO;
    std::shared_ptr<LampGeo> mScene;

    DirectX::BoundingSphere mSceneBounds;

    float mLightNearZ = 0.0f;
    float mLightFarZ = 0.0f;
    XMFLOAT3 mLightPosW;
    XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
    XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
    XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

    XMMATRIX mLastView = { 1,0,0,0,
                           0,1,0,0, 
                           0,0,1,0, 
                           0,0,0,1 };
    XMMATRIX mLastProj = { 1,0,0,0,
                           0,1,0,0,
                           0,0,1,0,
                           0,0,0,1 };

    float mLightRotationAngle = 0.0f;
    XMFLOAT3 mBaseLightDirections[3] = {
        XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(0.0f, -0.707f, -0.707f)
    };
    XMFLOAT3 mRotatedLightDirections[3];

    POINT mLastMousePos;

    BOOL isStart;
};
