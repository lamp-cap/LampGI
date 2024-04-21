#pragma once

namespace Scenes
{
    enum class ELightType
    {
        DIRECTIONAL,
        SPOT,
        POINT,
        COUNT
    };
    enum class EHeapType
    {
        DEFAULT = 0,
        UPLOAD = 1, 
        READBACK = 2
    };
    struct BufferDesc
    {
        UINT64 size = 0;
        UINT64 alignment = 0;
        EHeapType heap = EHeapType::UPLOAD;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    };

    struct GraphicsMaterial
    {
        DirectX::XMFLOAT3       albedo;                  // RGB [0-1]
        float                   opacity;                 // [0-1]
        DirectX::XMFLOAT3       emissiveColor;           // RGB [0-1]
        float                   roughness;               // [0-1]
        float                   metallic;                // [0-1]
        int                     alphaMode;               // 0: Opaque, 1: Blend, 2: Masked
        float                   alphaCutoff;             // [0-1]
        int                     doubleSided;             // 0: false, 1: true
        int                     albedoTexIdx;            // RGBA [0-1]
        int                     roughnessMetallicTexIdx; // R: Occlusion, G: Roughness, B: Metallic
        int                     normalTexIdx;            // Tangent space XYZ
        int                     emissiveTexIdx;          // RGB [0-1]
    };

    struct GraphicsLight
    {
        UINT                    type;                // 0: directional, 1: spot, 2: point (don't really need type on gpu with implicit placement)
        DirectX::XMFLOAT3       direction;           // Directional / Spot
        float                   power;
        DirectX::XMFLOAT3       position;            // Spot / Point
        float                   radius;              // Spot / Point
        DirectX::XMFLOAT3       color;
        float                   umbraAngle;          // Spot
        float                   penumbraAngle;       // Spot
        DirectX::XMFLOAT2       pad0;
    };

    struct GraphicsCamera
    {
        DirectX::XMFLOAT3       position;
        float                   aspect;
        DirectX::XMFLOAT3       up;
        float                   fov;
        DirectX::XMFLOAT3       right;
        float                   tanHalfFovY;
        DirectX::XMFLOAT3       forward;
        float                   pad0;
        DirectX::XMFLOAT2       resolution;
        float                   pad1;
    };
    struct ConfigLight
    {
        std::string name = "";
        ELightType type = ELightType::DIRECTIONAL;

        DirectX::XMFLOAT3 position = { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3 direction = { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3 color = { 1.f, 1.f, 1.f };

        float power = 1.f;
        float radius = 0.f;
        float umbraAngle = 0.f;
        float penumbraAngle = 0.f;
    };
    struct ConfigCamera
    {
        std::string name = "";
        DirectX::XMFLOAT3 position = { 0.f, 0.f, 0.f };
        float fov = 45.f;
        float yaw = 0.f;
        float pitch = 0.f;
        float aspect = 0.f;
    };
    struct ConfigScene
    {
        std::string name = "";
        std::string path = "";
        std::string file = "";
        DirectX::XMFLOAT3 skyColor = { 0.f, 0.f, 0.f };
        float skyIntensity = 1.0f;

        std::vector<ConfigCamera> cameras;
        std::vector<ConfigLight> lights;
    };
    enum class ETextureType
    {
        ENGINE = 0,
        SCENE,
    };

    enum class ETextureFormat
    {
        UNCOMPRESSED = 0,
        BC7,
    };
    struct NTexture
    {
        std::string name = "";
        std::string filepath = "";

        ETextureType type = ETextureType::SCENE;
        ETextureFormat format = ETextureFormat::UNCOMPRESSED;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t stride = 0;
        uint32_t mips = 0;

        uint64_t texelBytes = 0;    // the number of bytes (aligned, all mips)
        uint8_t* texels = nullptr;

        bool cached = false;

        void SetName(std::string n)
        {
            if (strcmp(name.c_str(), "") == 0) { name = n; }
        }
    };

    struct Config
    {
        ConfigScene         scene;
    };
}