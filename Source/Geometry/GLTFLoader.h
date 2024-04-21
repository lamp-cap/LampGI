/*
* Copyright (c) 2019-2023, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
#pragma once

#include "../D3D/FrameResource.h"
#include "./Geometry/GeometryGenerator.h"
#include "../main/RenderItem.h"
#include "SceneObject.h"

namespace Scenes
{
    struct MeshPrimitive
    {
        int                           index = -1;
        int                           material = -1;
        bool                          opaque = true;
        bool                          doubleSided = false;
        uint32_t                      vertexByteOffset = 0;
        uint32_t                      indexByteOffset = 0;
        DirectX::BoundingBox                   boundingBox; // not instanced transformed
        std::vector<Vertex>           vertices;
        std::vector<uint32_t>         indices;
    };

    struct Mesh
    {
        int index = -1;
        std::string name = "";
        uint32_t numIndices = 0;
        uint32_t numVertices = 0;
        DirectX::BoundingBox boundingBox; // not instance transformed
        std::vector<MeshPrimitive> primitives;
    };

    struct MeshInstance
    {
        std::string name = "";
        int meshIndex = -1;
        DirectX::BoundingBox boundingBox; // instance transformed
        float transform[3][4] =
        {
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f
        };
    };

    struct Material
    {
        std::string name = "";
        GraphicsMaterial data;

        const GraphicsMaterial* GetGPUData() const { return &this->data; }
        static const uint32_t GetGPUDataSize() { return static_cast<uint32_t>(sizeof(GraphicsMaterial)); }
    };

    struct Light
    {
        std::string name = "";
        ELightType type;
        bool dirty = false;
        GraphicsLight data;

        const GraphicsLight* GetGPUData() const { return &this->data; }
        static const uint32_t GetGPUDataSize() { return static_cast<uint32_t>(sizeof(GraphicsLight)); }
    };
    
    struct Camera
    {
        std::string name = "";
        float yaw = 0.f;
        float pitch = 0.f;
        GraphicsCamera data;

        const GraphicsCamera* GetGPUData() const { return &this->data; }
        static const uint32_t GetGPUDataSize() { return static_cast<uint32_t>(sizeof(GraphicsCamera)); }
    };

    struct SceneNode
    {
        int  instance = -1;
        int  camera = -1;
        bool hasMatrix = false;
        DirectX::XMFLOAT3 translation = DirectX::XMFLOAT3(0.f, 0.f, 0.f);
        DirectX::XMFLOAT4 rotation = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
        DirectX::XMFLOAT3 scale = DirectX::XMFLOAT3(1.f, 1.f, 1.f);
        DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();

        std::vector<int> children;
    };

    struct Scene
    {
        std::string name = "";
        uint32_t activeCamera = 0;
        uint32_t numMeshPrimitives = 0;
        uint32_t numTriangles = 0;
        uint32_t hasDirectionalLight = 0;
        uint32_t numPointLights = 0;
        uint32_t numSpotLights = 0;
        uint32_t firstPointLight = 0;
        uint32_t firstSpotLight = 0;

        DirectX::BoundingBox boundingBox;

        std::vector<int> rootNodes;
        std::vector<SceneNode> nodes;
        std::vector<Camera> cameras;
        std::vector<Light> lights;
        std::vector<MeshInstance> instances;
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<NTexture> textures;

        Camera& GetActiveCamera() { return cameras[activeCamera]; }
        const Camera& GetActiveCamera() const { return cameras[activeCamera]; }
    };

    struct TextureDesc
    {
        UINT width = 0;
        UINT height = 0;
        UINT arraySize = 1;
        UINT mips = 1;
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    };

    HRESULT Initialize(const Config& config, Scene& scene);
    void Traverse(size_t nodeIndex, DirectX::XMMATRIX transform, Scene& scene);
    void UpdateCamera(Camera& camera);
    void Cleanup(Scene& scene);
    HRESULT CreateAndUploadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, 
        Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer, const NTexture& texture);
}
