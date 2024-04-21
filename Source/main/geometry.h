#pragma once

#include "RenderItem.h"
#include "./Geometry/GeometryGenerator.h"
#include "../D3D/FrameResource.h"

class LampGeo
{
public:
    LampGeo(Microsoft::WRL::ComPtr <ID3D12Device> d3dDevice);
    LampGeo(const LampGeo& rhs) = delete;
    LampGeo& operator=(const LampGeo& rhs) = delete;
    ~LampGeo();

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

    std::vector<RenderItem*>& RenderItems(RenderLayer layer);
    Microsoft::WRL::ComPtr<ID3D12Resource> TextureRes(std::string name);
    void LoadScene(ID3D12GraphicsCommandList* mCommandList);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    std::vector<UINT> m_IndexOffsets;
    std::vector<UINT> m_VertexOffsets;
    std::vector<UINT> m_MeshToSceneMapping;
    std::vector<Submesh> submeshes;
    // Render items divided by PSO.
    std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

    std::unordered_map<std::string, std::unique_ptr<Mesh>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

    void BuildShapeGeometry(ID3D12GraphicsCommandList* mCommandList);
    void BuildSkullGeometry(ID3D12GraphicsCommandList* mCommandList);
    void BuildMaterials();
    void BuildRenderItems();
    void LoadTextures(ID3D12GraphicsCommandList* mCommandList);
    HRESULT LoadOBJ(ID3D12GraphicsCommandList* mCommandList, const char* fileName);
    HRESULT mLoadOBJ(ID3D12GraphicsCommandList* mCommandList);
    HRESULT LoadGLTF(ID3D12GraphicsCommandList* mCommandList);
};