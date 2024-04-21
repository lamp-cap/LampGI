/*
* Copyright (c) 2019-2023, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "GLTFLoader.h"

#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

#include <tinygltf/tiny_gltf.h>
#include <tinygltf/stb_image.h>

#include <regex>
#include <math.h>
#include <directxtex/DirectXTex.h>

using namespace DirectX;

namespace Scenes
{
    /**
     * Compute the aligned memory required for the texture.
     * Add texels to the texture if either dimension is not a factor of 4 (required for BC7 compressed formats).
     */
    bool FormatTexture(NTexture& texture)
    {
        // BC7 compressed textures require 4x4 texel blocks
        // Add texels to the texture if its original dimensions aren't factors of 4
        if (texture.width % 4 != 0 || texture.height % 4 != 0)
        {
            // Get original row stride
            uint32_t rowSize = (texture.width * texture.stride);
            uint32_t numRows = texture.height;

            // Align the new texture to 4x4
            texture.width = ALIGN(4, texture.width);
            texture.height = ALIGN(4, texture.height);

            uint32_t alignedRowSize = (texture.width * texture.stride);
            uint32_t size = alignedRowSize * texture.height;

            // Copy the original texture into the new one
            size_t offset = 0;
            size_t alignedOffset = 0;
            uint8_t* texels = new uint8_t[size];
            memset(texels, 0, size);
            for (uint32_t row = 0; row < numRows; row++)
            {
                memcpy(&texels[alignedOffset], &texture.texels[offset], rowSize);
                alignedOffset += alignedRowSize;
                offset += rowSize;
            }

            // Release the memory of the original texture
            delete[] texture.texels;
            texture.texels = texels;
        }

        // Compute the texture's aligned memory size
        uint32_t rowSize = (texture.width * texture.stride);
        uint32_t rowPitch = ALIGN(256, rowSize);          // 256 == D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
        texture.texelBytes = (rowPitch * texture.height);

        return (texture.texelBytes > 0);
    }

    /**
     * Release texture memory (CPU).
     */
    void Unload(NTexture& texture)
    {
        delete[] texture.texels;
        texture = {};
    }

    bool Load(NTexture& texture)
    {
        if (texture.format == ETextureFormat::UNCOMPRESSED)
        {
            // Load the uncompressed texture with stb_image (require 4 component RGBA)
            texture.texels = stbi_load(texture.filepath.c_str(), (int*)&(texture.width), (int*)&texture.height, (int*)&texture.stride, STBI_rgb_alpha);
            if (!texture.texels)
            {
                std::string msg = "Error: failed to load texture: \'" + texture.name + "\' \'" + texture.filepath + "\'";
                MessageBoxA(0, msg.c_str(), 0, 0);
                return false;
            }

            texture.stride = 4;
            texture.mips = 1;

            // Prep the texture for compression and use on the GPU
            return FormatTexture(texture);
        }
        return false;
    }



    void SetTranslation(const tinygltf::Node& gltfNode, XMFLOAT3& translation)
    {
        translation = XMFLOAT3((float)gltfNode.translation[0], (float)gltfNode.translation[1], (float)gltfNode.translation[2]);
    }

    void SetRotation(const tinygltf::Node& gltfNode, XMFLOAT4& rotation)
    {
        rotation = XMFLOAT4((float)gltfNode.rotation[0], (float)gltfNode.rotation[1], (float)gltfNode.rotation[2], (float)gltfNode.rotation[3]);
    }

    void SetScale(const tinygltf::Node& gltfNode, XMFLOAT3& scale)
    {
        scale = XMFLOAT3((float)gltfNode.scale[0], (float)gltfNode.scale[1], (float)gltfNode.scale[2]);
    }
    /**
     * Get the size (in bytes) of an aligned BC7 compressed texture.
     * This matches the size returned by D3D12Device->GetCopyableFootprints(...).
     * https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-getcopyablefootprints
     */
    uint32_t GetBC7TextureSizeInBytes(uint32_t width, uint32_t height)
    {
        uint32_t numRows = height / 4;
        uint32_t rowPitch = ALIGN(16, width * 4);
        return ALIGN(512, numRows * ALIGN(256, rowPitch));
    }
    /**
     * Copy a compressed BC7 texture into our format, aligned for GPU use.
     */
     /*
    bool FormatCompressedTexture(ScratchImage& src, NTexture& dst)
    {
        bool result = false;

        // Get the texture's metadata
        const TexMetadata metadata = src.GetMetadata();

        // Check if the texture's format is supported
        if (metadata.format != DXGI_FORMAT_BC7_UNORM && metadata.format != DXGI_FORMAT_BC7_UNORM_SRGB && metadata.format != DXGI_FORMAT_BC7_TYPELESS)
        {
            std::string msg = "Error: unsupported compressed texture format for: \'" + dst.name + "\' \'" + dst.filepath + "\'\n. Compressed textures must be in BC7 format";
            MessageBoxA(0, msg.c_str(), 0, 0);
            return false;
        }

        // Set texture data
        dst.width = static_cast<int>(metadata.width);
        dst.height = static_cast<int>(metadata.height);
        dst.stride = 1;
        dst.mips = static_cast<int>(metadata.mipLevels);
        dst.texelBytes = 0;

        // Compute the total size of the texture in bytes (including alignment).
        // Note: BC7 uses fixed block sizes of 4x4 texels with 16 bytes per block, 1 byte per texel.
        for (uint32_t mipIndex = 0; mipIndex < dst.mips; mipIndex++)
        {
            // Compute the size of the mip level and add it to the total aligned memory size
            const Image* image = src.GetImage(mipIndex, 0, 0);

            uint32_t alignedWidth = ALIGN(4, static_cast<uint32_t>(image->width));
            uint32_t alignedHeight = ALIGN(4, static_cast<uint32_t>(image->height));

            // Add the size of the last mip (one texel)
            if (dst.mips > 1 && (mipIndex + 1) == dst.mips)
            {
                dst.texelBytes += 16; // BC7 blocks are 16 bytes
                break;
            }

            // Get the aligned memory size in bytes of the mip level and add it to the texture memory total
            dst.texelBytes += GetBC7TextureSizeInBytes(alignedWidth, alignedHeight);
        }

        if (dst.texelBytes > 0)
        {
            // Delete existing texels
            if (dst.texels)
            {
                delete[] dst.texels;
                dst.texels = nullptr;
            }

            // Copy each aligned mip level to the texel array
            size_t alignedOffset = 0;
            dst.texels = new uint8_t[dst.texelBytes];
            memset(dst.texels, 0, dst.texelBytes);
            for (uint32_t mipIndex = 0; mipIndex < dst.mips; mipIndex++)
            {
                size_t offset = 0;
                const Image* image = src.GetImage(mipIndex, 0, 0);

                uint32_t alignedHeight = ALIGN(4, static_cast<uint32_t>(image->height));

                // Copy the last mip level / block
                if (dst.mips > 1 && (mipIndex + 1) == dst.mips)
                {
                    memcpy(&dst.texels[alignedOffset], &image->pixels[offset], image->rowPitch);
                    alignedOffset += image->rowPitch;
                    assert(dst.texelBytes == alignedOffset);
                    break;
                }

                // Copy each row of the mip texture, padding for alignment
                size_t numRows = alignedHeight / 4;
                for (uint32_t rowIndex = 0; rowIndex < numRows; rowIndex++)
                {
                    memcpy(&dst.texels[alignedOffset], &image->pixels[offset], image->rowPitch);
                    offset += image->rowPitch;
                    alignedOffset += ALIGN(256, image->rowPitch);
                }

                alignedOffset = ALIGN(512, alignedOffset);
            }
            result = true;
        }

        src.Release();
        return result;
    }

    bool MipmapAndCompress(NTexture& texture, bool quick)
    {
        // DirectX::GenerateMipMaps does not support block compressed images
        if (texture.format != ETextureFormat::UNCOMPRESSED) return false;

        // B7 textures must be aligned to pixel 4x4 blocks
        assert(texture.width % 4 == 0);
        assert(texture.height % 4 == 0);

        Image source = {};
        source.width = texture.width;
        source.height = texture.height;
        source.rowPitch = (texture.width * texture.stride);
        source.slicePitch = (source.rowPitch * source.height);
        source.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        source.pixels = texture.texels;

        // Generate the mipmap chain
        ScratchImage mips;
        if (FAILED(DirectX::GenerateMipMaps(source, TEX_FILTER_DEFAULT, 0, mips))) return false;

        TEX_COMPRESS_FLAGS flags = TEX_COMPRESS_DEFAULT;
        if (quick) flags = TEX_COMPRESS_BC7_QUICK;

        // Compress the mip chain to BC7 format
        ScratchImage compressed;
#ifdef GPU_COMPRESSION
        if (FAILED(DirectX::Compress(d3d11Device, mips.GetImages(), mips.GetImageCount(), mips.GetMetadata(), DXGI_FORMAT_BC7_UNORM, flags, 1.f, compressed))) return false;
#else
        flags |= TEX_COMPRESS_PARALLEL;
        if (FAILED(DirectX::Compress(mips.GetImages(), mips.GetImageCount(), mips.GetMetadata(), DXGI_FORMAT_BC7_UNORM, flags, TEX_THRESHOLD_DEFAULT, compressed))) return false;
#endif

        mips.Release();
        texture.format = ETextureFormat::BC7;

        // Format the compressed image into our format, prepping it for use on the GPU
        return FormatCompressedTexture(compressed, texture);
    }*/
    /**
     * Parse a URI, removing escaped characters (e.g. %20 for spaces)
     */
    std::string ParseURI(const std::string& in)
    {
        std::string result = std::string(in.begin(), in.end());
        size_t pos = result.find("%20");
        while (pos != std::string::npos)
        {
            result.replace(pos, 3, 1, ' ');
            pos = result.find("%20");
        }
        return result;
    }

    void ParseGLTFCameras(const tinygltf::Model& gltfData, Scene& scene)
    {
        for (uint32_t cameraIndex = 0; cameraIndex < static_cast<uint32_t>(gltfData.cameras.size()); cameraIndex++)
        {
            // Get the glTF camera
            const tinygltf::Camera gltfCamera = gltfData.cameras[cameraIndex];
            if (strcmp(gltfCamera.type.c_str(), "perspective") == 0)
            {
                Camera camera;
                camera.data.fov = (float)gltfCamera.perspective.yfov * (180.f / XM_PI);
                camera.data.tanHalfFovY = tanf(camera.data.fov * (XM_PI / 180.f) * 0.5f);

                UpdateCamera(camera);

                scene.cameras.push_back(camera);
            }
        }
    }

    void ParseGLTFNodes(const tinygltf::Model& gltfData, Scene& scene)
    {
        // Get the default scene
        const tinygltf::Scene gltfScene = gltfData.scenes[gltfData.defaultScene];

        // Get the indices of the scene's root nodes
        for (uint32_t rootIndex = 0; rootIndex < static_cast<uint32_t>(gltfScene.nodes.size()); rootIndex++)
        {
            scene.rootNodes.push_back(gltfScene.nodes[rootIndex]);
        }

        // Get all the nodes
        for (uint32_t nodeIndex = 0; nodeIndex < static_cast<uint32_t>(gltfData.nodes.size()); nodeIndex++)
        {
            // Get the glTF node
            const tinygltf::Node gltfNode = gltfData.nodes[nodeIndex];

            // Create the scene node
            SceneNode node;

            // Get the node's local transform data
            if (gltfNode.matrix.size() > 0)
            {
                node.matrix = XMMATRIX(
                    (float)gltfNode.matrix[0], (float)gltfNode.matrix[1], (float)gltfNode.matrix[2], (float)gltfNode.matrix[3],
                    (float)gltfNode.matrix[4], (float)gltfNode.matrix[5], (float)gltfNode.matrix[6], (float)gltfNode.matrix[7],
                    (float)gltfNode.matrix[8], (float)gltfNode.matrix[9], (float)gltfNode.matrix[10], (float)gltfNode.matrix[11],
                    (float)gltfNode.matrix[12], (float)gltfNode.matrix[13], (float)gltfNode.matrix[14], (float)gltfNode.matrix[15]
                );
                node.hasMatrix = true;
            }
            else
            {
                if (gltfNode.translation.size() > 0) SetTranslation(gltfNode, node.translation);
                if (gltfNode.rotation.size() > 0) SetRotation(gltfNode, node.rotation);
                if (gltfNode.scale.size() > 0) SetScale(gltfNode, node.scale);
            }

            // Camera node, store the transforms
            if (gltfNode.camera != -1)
            {
                node.camera = gltfNode.camera;
                scene.cameras[node.camera].data.position = { node.translation.x, node.translation.y, node.translation.z };

                XMMATRIX xform = XMMatrixRotationQuaternion(XMLoadFloat4(&node.rotation));
                XMFLOAT3 right, up, forward;

                XMStoreFloat3(&right, xform.r[0]);
                XMStoreFloat3(&up, xform.r[1]);
                XMStoreFloat3(&forward, xform.r[2]);

                scene.cameras[node.camera].data.right = { right.x, right.y, right.z };
                scene.cameras[node.camera].data.up = { up.x, up.y, up.z };
                scene.cameras[node.camera].data.forward = { forward.x, forward.y, forward.z };
            }

            // When at a leaf node, add a mesh instance to the scene (if a mesh exists for the node)
            if (gltfNode.children.size() == 0 && gltfNode.mesh != -1)
            {
                // Write the instance data
                MeshInstance instance;
                instance.name = gltfNode.name;
                if (instance.name.compare("") == 0) instance.name = "Instance_" + std::to_string(static_cast<int>(scene.instances.size()));
                instance.meshIndex = gltfNode.mesh;

                node.instance = static_cast<int>(scene.instances.size());
                scene.instances.push_back(instance);
            }

            // Gather the child node indices
            for (uint32_t childIndex = 0; childIndex < static_cast<uint32_t>(gltfNode.children.size()); childIndex++)
            {
                node.children.push_back(gltfNode.children[childIndex]);
            }

            // Add the new node to the scene graph
            scene.nodes.push_back(node);
        }

        // Traverse the scene graph and update instance transforms
        for (size_t rootIndex = 0; rootIndex < scene.rootNodes.size(); rootIndex++)
        {
            XMMATRIX transform = XMMatrixIdentity();
            int nodeIndex = scene.rootNodes[rootIndex];
            Traverse(nodeIndex, transform, scene);
        }
    }

    void ParseGLTFMaterials(const tinygltf::Model& gltfData, Scene& scene)
    {
        for (uint32_t materialIndex = 0; materialIndex < static_cast<uint32_t>(gltfData.materials.size()); materialIndex++)
        {
            const tinygltf::Material gltfMaterial = gltfData.materials[materialIndex];
            const tinygltf::PbrMetallicRoughness pbr = gltfMaterial.pbrMetallicRoughness;

            // Transform glTF material into our material format
            Material material;
            material.name = gltfMaterial.name;
            if (material.name.compare("") == 0) material.name = "Material_" + std::to_string(materialIndex);

            material.data.doubleSided = (int)gltfMaterial.doubleSided;

            // Albedo and Opacity
            material.data.albedo = { (float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1], (float)pbr.baseColorFactor[2] };
            material.data.opacity = (float)pbr.baseColorFactor[3];
            material.data.albedoTexIdx = pbr.baseColorTexture.index;

            // Alpha
            material.data.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
            if (strcmp(gltfMaterial.alphaMode.c_str(), "OPAQUE") == 0) material.data.alphaMode = 0;
            else if (strcmp(gltfMaterial.alphaMode.c_str(), "BLEND") == 0) material.data.alphaMode = 1;
            else if (strcmp(gltfMaterial.alphaMode.c_str(), "MASK") == 0) material.data.alphaMode = 2;

            // Roughness and Metallic
            material.data.roughness = (float)pbr.roughnessFactor;
            material.data.metallic = (float)pbr.metallicFactor;
            material.data.roughnessMetallicTexIdx = pbr.metallicRoughnessTexture.index;

            // Normals
            material.data.normalTexIdx = gltfMaterial.normalTexture.index;

            // Emissive
            material.data.emissiveColor = { (float)gltfMaterial.emissiveFactor[0], (float)gltfMaterial.emissiveFactor[1], (float)gltfMaterial.emissiveFactor[2] };
            material.data.emissiveTexIdx = gltfMaterial.emissiveTexture.index;

            scene.materials.push_back(material);
        }

        // If there are no materials, create a default material
        if (scene.materials.size() == 0)
        {
            Material mat = {};
            mat.name = "Default Material";
            mat.data.albedo = { 1.f, 1.f, 1.f };
            mat.data.opacity = 1.f;
            mat.data.roughness = 1.f;
            mat.data.albedoTexIdx = -1;
            mat.data.roughnessMetallicTexIdx = -1;
            mat.data.normalTexIdx = -1;
            mat.data.emissiveTexIdx = -1;

            scene.materials.push_back(mat);
        }
    }

    bool ParseGLFTextures(const tinygltf::Model& gltfData, const Config& config, Scene& scene)
    {
        for (uint32_t textureIndex = 0; textureIndex < static_cast<uint32_t>(gltfData.textures.size()); textureIndex++)
        {
            // Get the GLTF texture
            const tinygltf::Texture& gltfTexture = gltfData.textures[textureIndex];

            // Skip this texture if the source image doesn't exist
            if (gltfTexture.source == -1 || gltfData.images.size() <= gltfTexture.source) continue;

            // Get the GLTF image
            const tinygltf::Image gltfImage = gltfData.images[gltfTexture.source];

            NTexture texture;
            texture.SetName(gltfImage.uri);
            texture.SetName(gltfImage.name);
            texture.SetName(gltfTexture.name);

            // Construct the texture image filepath
            texture.filepath = config.scene.path + ParseURI(gltfImage.uri);

            // Load the texture from disk
            if (!Load(texture)) return false;

            // if (!MipmapAndCompress(texture, false)) return false;

            // Add the texture to the scene
            scene.textures.push_back(texture);
        }
        return true;
    }

    void ParseGLTFMeshes(const tinygltf::Model& gltfData, Scene& scene)
    {
        // Note: GTLF 2.0's default coordinate system is Right Handed, Y-Up
        // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#coordinate-system-and-units
        // Meshes are converted from this coordinate system to the chosen coordinate system.

        uint32_t geometryIndex = 0;
        for (uint32_t meshIndex = 0; meshIndex < static_cast<uint32_t>(gltfData.meshes.size()); meshIndex++)
        {
            const tinygltf::Mesh& gltfMesh = gltfData.meshes[meshIndex];

            Mesh mesh;
            mesh.name = gltfMesh.name;
            mesh.numVertices = 0;
            mesh.numIndices = 0;

            if (mesh.name.compare("") == 0) mesh.name = "Mesh_" + std::to_string(meshIndex);

            // Initialize the mesh bounding box
            XMFLOAT3 mMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
            XMFLOAT3 mMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

            XMVECTOR mMin = XMLoadFloat3(&mMinf3);
            XMVECTOR mMax = XMLoadFloat3(&mMaxf3);

            uint32_t vertexByteOffset = 0;
            uint32_t indexByteOffset = 0;
            for (uint32_t primitiveIndex = 0; primitiveIndex < static_cast<uint32_t>(gltfMesh.primitives.size()); primitiveIndex++)
            {
                // Get a reference to the mesh primitive
                const tinygltf::Primitive& p = gltfMesh.primitives[primitiveIndex];

                MeshPrimitive mp;
                mp.index = geometryIndex;
                mp.material = p.material;
                mp.vertexByteOffset = vertexByteOffset;
                mp.indexByteOffset = indexByteOffset;

                // Initialize the mesh primitive bounding box
                XMFLOAT3 pMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
                XMFLOAT3 pMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
                XMVECTOR pMin = XMLoadFloat3(&pMinf3);
                XMVECTOR pMax = XMLoadFloat3(&pMaxf3);

                // Set the mesh primitive's material to the default material if one is not assigned or if no materials exist in the GLTF
                if (mp.material == -1) mp.material = 0;

                // Get a reference to the mesh primitive's material
                // If the mesh primitive material is blended or masked, it is not opaque
                const Material& mat = scene.materials[mp.material];
                if (mat.data.alphaMode != 0) mp.opaque = false;

                // Get data indices
                int indicesIndex = p.indices;
                int positionIndex = -1;
                int normalIndex = -1;
                int tangentIndex = -1;
                int uv0Index = -1;

                if (p.attributes.count("POSITION") > 0)
                {
                    positionIndex = p.attributes.at("POSITION");
                }

                if (p.attributes.count("NORMAL") > 0)
                {
                    normalIndex = p.attributes.at("NORMAL");
                }

                if (p.attributes.count("TANGENT"))
                {
                    tangentIndex = p.attributes.at("TANGENT");
                }

                if (p.attributes.count("TEXCOORD_0") > 0)
                {
                    uv0Index = p.attributes.at("TEXCOORD_0");
                }

                // Vertex positions
                const tinygltf::Accessor& positionAccessor = gltfData.accessors[positionIndex];
                const tinygltf::BufferView& positionBufferView = gltfData.bufferViews[positionAccessor.bufferView];
                const tinygltf::Buffer& positionBuffer = gltfData.buffers[positionBufferView.buffer];
                const uint8_t* positionBufferAddress = positionBuffer.data.data();
                int positionStride = tinygltf::GetComponentSizeInBytes(positionAccessor.componentType) * tinygltf::GetNumComponentsInType(positionAccessor.type);
                assert(positionStride == 12);

                // Vertex indices
                const tinygltf::Accessor& indexAccessor = gltfData.accessors[indicesIndex];
                const tinygltf::BufferView& indexBufferView = gltfData.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = gltfData.buffers[indexBufferView.buffer];
                const uint8_t* indexBufferAddress = indexBuffer.data.data();
                int indexStride = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) * tinygltf::GetNumComponentsInType(indexAccessor.type);
                mp.indices.resize(indexAccessor.count);

                // Vertex normals
                tinygltf::Accessor normalAccessor;
                tinygltf::BufferView normalBufferView;
                const uint8_t* normalBufferAddress = nullptr;
                int normalStride = -1;
                if (normalIndex > -1)
                {
                    normalAccessor = gltfData.accessors[normalIndex];
                    normalBufferView = gltfData.bufferViews[normalAccessor.bufferView];
                    normalStride = tinygltf::GetComponentSizeInBytes(normalAccessor.componentType) * tinygltf::GetNumComponentsInType(normalAccessor.type);
                    assert(normalStride == 12);

                    const tinygltf::Buffer& normalBuffer = gltfData.buffers[normalBufferView.buffer];
                    normalBufferAddress = normalBuffer.data.data();
                }

                // Vertex tangents
                tinygltf::Accessor tangentAccessor;
                tinygltf::BufferView tangentBufferView;
                const uint8_t* tangentBufferAddress = nullptr;
                int tangentStride = -1;
                if (tangentIndex > -1)
                {
                    tangentAccessor = gltfData.accessors[tangentIndex];
                    tangentBufferView = gltfData.bufferViews[tangentAccessor.bufferView];
                    tangentStride = tinygltf::GetComponentSizeInBytes(tangentAccessor.componentType) * tinygltf::GetNumComponentsInType(tangentAccessor.type);
                    assert(tangentStride == 16);

                    const tinygltf::Buffer& tangentBuffer = gltfData.buffers[tangentBufferView.buffer];
                    tangentBufferAddress = tangentBuffer.data.data();
                }

                // Vertex texture coordinates
                tinygltf::Accessor uv0Accessor;
                tinygltf::BufferView uv0BufferView;
                const uint8_t* uv0BufferAddress = nullptr;
                int uv0Stride = -1;
                if (uv0Index > -1)
                {
                    uv0Accessor = gltfData.accessors[uv0Index];
                    uv0BufferView = gltfData.bufferViews[uv0Accessor.bufferView];
                    uv0Stride = tinygltf::GetComponentSizeInBytes(uv0Accessor.componentType) * tinygltf::GetNumComponentsInType(uv0Accessor.type);
                    assert(uv0Stride == 8);

                    const tinygltf::Buffer& uv0Buffer = gltfData.buffers[uv0BufferView.buffer];
                    uv0BufferAddress = uv0Buffer.data.data();
                }

                // Get the vertex data
                for (uint32_t vertexIndex = 0; vertexIndex < static_cast<uint32_t>(positionAccessor.count); vertexIndex++)
                {
                    Vertex v;

                    const uint8_t* address = positionBufferAddress + positionBufferView.byteOffset + positionAccessor.byteOffset + (vertexIndex * positionStride);
                    memcpy(&v.position, address, positionStride);

                    if (normalIndex > -1)
                    {
                        address = normalBufferAddress + normalBufferView.byteOffset + normalAccessor.byteOffset + (vertexIndex * normalStride);
                        memcpy(&v.normal, address, normalStride);
                    }

                    if (tangentIndex > -1)
                    {
                        address = tangentBufferAddress + tangentBufferView.byteOffset + tangentAccessor.byteOffset + (vertexIndex * tangentStride);
                        memcpy(&v.tangent, address, tangentStride);
                    }

                    if (uv0Index > -1)
                    {
                        address = uv0BufferAddress + uv0BufferView.byteOffset + uv0Accessor.byteOffset + (vertexIndex * uv0Stride);
                        memcpy(&v.uv0, address, uv0Stride);
                    }

                    // Update the mesh primitive's bounding box
                    XMVECTOR P = XMLoadFloat3(&v.position);
                    pMin = XMVectorMin(pMin, P);
                    pMax = XMVectorMax(pMax, P);
                    //mp.boundingBox.min = XMVectorMin(mp.boundingBox.min, v.position);
                    //mp.boundingBox.max = XMVectorMax(mp.boundingBox.max, v.position);

                    mp.vertices.push_back(v);
                    mesh.numVertices++;
                }

                BoundingBox pBounds;
                XMStoreFloat3(&pBounds.Center, 0.5f * (pMin + pMax));
                XMStoreFloat3(&pBounds.Extents, 0.5f * (pMax - pMin));
                mp.boundingBox = pBounds;

                // Get the index data
                // Indices can be either unsigned char, unsigned short, or unsigned long
                // Converting to full precision for easy use on GPU
                const uint8_t* baseAddress = indexBufferAddress + indexBufferView.byteOffset + indexAccessor.byteOffset;
                if (indexStride == 1)
                {
                    std::vector<uint8_t> quarter;
                    quarter.resize(indexAccessor.count);

                    memcpy(quarter.data(), baseAddress, (indexAccessor.count * indexStride));

                    // Convert quarter precision indices to full precision
                    for (size_t i = 0; i < indexAccessor.count; i++)
                    {
                        mp.indices[i] = quarter[i];
                    }
                }
                else if (indexStride == 2)
                {
                    std::vector<uint16_t> half;
                    half.resize(indexAccessor.count);

                    memcpy(half.data(), baseAddress, (indexAccessor.count * indexStride));

                    // Convert half precision indices to full precision
                    for (size_t i = 0; i < indexAccessor.count; i++)
                    {
                        mp.indices[i] = half[i];
                    }
                }
                else
                {
                    memcpy(mp.indices.data(), baseAddress, (indexAccessor.count * indexStride));
                }

                // Update byte offsets
                vertexByteOffset += static_cast<uint32_t>(mp.vertices.size()) * sizeof(Vertex);
                indexByteOffset += static_cast<uint32_t>(mp.indices.size()) * sizeof(UINT);

                // Increment the triangle count
                mesh.numIndices += static_cast<int>(indexAccessor.count);
                scene.numTriangles += mesh.numIndices / 3;

                // Update the mesh's bounding box
                mMin = XMVectorMin(mMin, pMin);
                mMax = XMVectorMin(mMax, pMax);
                // mesh.boundingBox.min = XMVectorMin(mesh.boundingBox.min, mp.boundingBox.min);
                // mesh.boundingBox.max = XMVectorMax(mesh.boundingBox.max, mp.boundingBox.max);

                // Add the mesh primitive
                mesh.primitives.push_back(mp);

                geometryIndex++;
            }

            BoundingBox mBounds;
            XMStoreFloat3(&mBounds.Center, 0.5f * (mMin + mMax));
            XMStoreFloat3(&mBounds.Extents, 0.5f * (mMax - mMin));
            mesh.boundingBox = mBounds;

            mesh.index = static_cast<int>(scene.meshes.size());
            scene.meshes.push_back(mesh);
        }

        scene.numMeshPrimitives = geometryIndex;
    }

    void UpdateSceneBoundingBoxes(Scene& scene)
    {
        // Initialize the scene bounding box
        XMFLOAT3 sMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
        XMFLOAT3 sMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

        XMVECTOR sMin = XMLoadFloat3(&sMinf3);
        XMVECTOR sMax = XMLoadFloat3(&sMaxf3);
        // scene.boundingBox.min = { FLT_MAX, FLT_MAX };
        // scene.boundingBox.max = { -FLT_MAX, -FLT_MAX };

        XMFLOAT4 r3 = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
        for (uint32_t instanceIndex = 0; instanceIndex < static_cast<uint32_t>(scene.instances.size()); instanceIndex++)
        {
            // Get the mesh instance and mesh
            Scenes::MeshInstance& instance = scene.instances[instanceIndex];
            const Scenes::Mesh& mesh = scene.meshes[instance.meshIndex];

            // Initialize the mesh instance bounding box
            XMFLOAT3 iMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
            XMFLOAT3 iMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

            XMVECTOR iMin = XMLoadFloat3(&iMinf3);
            XMVECTOR iMax = XMLoadFloat3(&iMaxf3);
            // instance.boundingBox.min = { FLT_MAX, FLT_MAX };
            // instance.boundingBox.max = { -FLT_MAX, -FLT_MAX };

            // Instance transform rows
            XMFLOAT4 instanceXformR0 = XMFLOAT4(&instance.transform[0][0]);
            XMFLOAT4 instanceXformR1 = XMFLOAT4(&instance.transform[1][0]);
            XMFLOAT4 instanceXformR2 = XMFLOAT4(&instance.transform[2][0]);

            // Instance transform matrix
            XMMATRIX xform;
            xform.r[0] = DirectX::XMLoadFloat4(&instanceXformR0);
            xform.r[1] = DirectX::XMLoadFloat4(&instanceXformR1);
            xform.r[2] = DirectX::XMLoadFloat4(&instanceXformR2);
            xform.r[3] = DirectX::XMLoadFloat4(&r3);

            // Remove the transpose (transforms are transposed for copying to GPU)
            XMMATRIX transpose = XMMatrixTranspose(xform);

            // Get the mesh bounding box vertices
            XMFLOAT3 mCenter = mesh.boundingBox.Center;
            XMFLOAT3 mExtents = mesh.boundingBox.Extents;

            XMFLOAT3 min = XMFLOAT3(mCenter.x - mExtents.x, mCenter.y - mExtents.y, mCenter.z - mExtents.z);
            XMFLOAT3 max = XMFLOAT3(mCenter.x + mExtents.x, mCenter.y + mExtents.y, mCenter.z + mExtents.z);

            // Transform the mesh bounding box vertices
            DirectX::XMStoreFloat3(&min, XMVector3Transform(DirectX::XMLoadFloat3(&min), transpose));
            DirectX::XMStoreFloat3(&max, XMVector3Transform(DirectX::XMLoadFloat3(&max), transpose));

            // Update the mesh instance bounding box
            iMin = XMVectorMin(iMin, { max.x, max.y, max.z });
            iMin = XMVectorMin(iMin, { min.x, min.y, min.z });
            iMax = XMVectorMax(iMax, { max.x, max.y, max.z });
            iMax = XMVectorMax(iMax, { min.x, min.y, min.z });

            // instance.boundingBox.min = XMVectorMin(instance.boundingBox.min, { max.x, max.y, max.z });
            // instance.boundingBox.min = XMVectorMin(instance.boundingBox.min, { min.x, min.y, min.z });
            // instance.boundingBox.max = XMVectorMax(instance.boundingBox.max, { max.x, max.y, max.z });
            // instance.boundingBox.max = XMVectorMax(instance.boundingBox.max, { min.x, min.y, min.z });

            BoundingBox iBounds;
            XMStoreFloat3(&iBounds.Center, 0.5f * (iMin + iMax));
            XMStoreFloat3(&iBounds.Extents, 0.5f * (iMax - iMin));
            instance.boundingBox = iBounds;

            // Update the scene bounding box
            sMin = XMVectorMin(sMin, iMin);
            sMax = XMVectorMax(sMin, iMax);
        }
        BoundingBox sBounds;
        XMStoreFloat3(&sBounds.Center, 0.5f * (sMin + sMax));
        XMStoreFloat3(&sBounds.Extents, 0.5f * (sMax - sMin));
        scene.boundingBox = sBounds;
    }

    bool ParseGLTF(const tinygltf::Model& gltfData, const Config& config, const bool binary, Scene& scene)
    {
        if (binary && gltfData.textures.size() > 0)
        {
            std::wstring msg = L"\nFailed to load scene file! GLB format not supported for scenes with texture data. Use the *.gltf file format instead\n.";
            MessageBox(0, msg.c_str(), 0, 0);
            return false;
        }

        // Parse Cameras
        ParseGLTFCameras(gltfData, scene);

        // Parse Nodes
        ParseGLTFNodes(gltfData, scene);

        // Parse Materials
        ParseGLTFMaterials(gltfData, scene);

        // Parse and Load Textures
        if (!ParseGLFTextures(gltfData, config, scene)) return false;

        // Parse Meshes
        ParseGLTFMeshes(gltfData, scene);

        // Update the scene's bounding boxes, based on the instance transforms
        UpdateSceneBoundingBoxes(scene);

        return true;
    }

    /**
     * Create a texture resource on the default heap.
     */
    bool CreateTexture(ID3D12Device* device, const TextureDesc& info, Microsoft::WRL::ComPtr<ID3D12Resource>& resource)
    {
        // Describe the texture resource
        D3D12_RESOURCE_DESC desc = {};
        desc.Width = info.width;
        desc.Height = info.height;
        desc.MipLevels = info.mips;
        desc.Format = info.format;
        desc.DepthOrArraySize = (UINT16)info.arraySize;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Flags = info.flags;

        // Setup the optimized clear value
        D3D12_CLEAR_VALUE clear = {};
        clear.Color[3] = 1.f;
        clear.Format = info.format;

        // Create the texture resource
        bool useClear = (info.flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        const D3D12_HEAP_PROPERTIES defaultHeapProps = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
        ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProps, 
            D3D12_HEAP_FLAG_NONE, 
            &desc, 
            info.state, 
            useClear ? &clear : nullptr, 
            IID_PPV_ARGS(&resource)));

        return true;
    }
    /**
     * Create a buffer resource.
     */
    bool CreateBuffer(ID3D12Device* device, const BufferDesc& info, Microsoft::WRL::ComPtr<ID3D12Resource>& ppResource)
    {
        // Describe the buffer resource
        D3D12_RESOURCE_DESC desc = {};
        desc.Alignment = 0;
        desc.Height = 1;
        desc.Width = info.size;
        desc.MipLevels = 1;
        desc.DepthOrArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = info.flags;

        // Select the heap
        D3D12_HEAP_PROPERTIES heapProps;
        if (info.heap == EHeapType::DEFAULT)
        {
            heapProps = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
        }
        else if (info.heap == EHeapType::UPLOAD)
        {
            heapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
        }
        else if (info.heap == EHeapType::READBACK)
        {
            heapProps = { D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
        }

        // Create the buffer resource
        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, info.state, nullptr, IID_PPV_ARGS(&ppResource)));
        return true;
    }
    //----------------------------------------------------------------------------------------------------------
    // Public Functions
    //----------------------------------------------------------------------------------------------------------

    /**
     * Loads and parses a glTF 2.0 scene.
     */
    HRESULT Initialize(const Config& config, Scene& scene)
    {
        // Set the scene name
        scene.name = config.scene.name;

        // Check for valid file formats
        bool binary = false;
        const std::regex glbFile("^[\\w-]+\\.glb$");
        const std::regex gltfFile("^[\\w-]+\\.gltf$");
        if (std::regex_match(config.scene.file, glbFile)) binary = true;
        else if (std::regex_match(config.scene.file, gltfFile)) binary = false;
        else
        {
            // Unknown file format
            std::string msg = "Unknown file format \'" + config.scene.file + "'";
            MessageBoxA(0, msg.c_str(), 0, 0);
            return E_FAIL;
        }
        /*
        // Construct the cache file name
        std::string cacheName = "";
        if (binary)
        {
            const std::regex glbExtension("\\.glb$");
            std::regex_replace(back_inserter(cacheName), config.scene.file.begin(), config.scene.file.end(), glbExtension, "");
        }
        else
        {
            const std::regex gltfExtension("\\.gltf$");
            std::regex_replace(back_inserter(cacheName), config.scene.file.begin(), config.scene.file.end(), gltfExtension, "");
        }

        // Load the scene cache file, if it exists
        std::string sceneCache = config.app.root + config.scene.path + cacheName + ".cache";
        if (Deserialize(sceneCache, scene, log))
        {
            ParseConfigCamerasLights(config, scene);
            return true;
        }*/

        // Load the scene GLTF (no cache file exists or the existing cache file is invalid)
        tinygltf::Model gltfData;
        tinygltf::TinyGLTF gltfLoader;
        std::string err, warn, filepath;

        // Build the path to the GLTF file
        filepath = config.scene.path + config.scene.file;

        // Load the scene
        bool result = false;
        if (binary) result = gltfLoader.LoadBinaryFromFile(&gltfData, &err, &warn, filepath);
        else result = gltfLoader.LoadASCIIFromFile(&gltfData, &err, &warn, filepath);

        if (!result)
        {
            // An error occurred
            std::wstring msg1 = binary?L"result == null,binary\n": L"result == null\n";
            OutputDebugString(msg1.c_str());
            std::string msg = std::string(err.begin(), err.end());
            MessageBoxA(0, msg.c_str(),0,0);
            return E_FAIL;
        }
        else if (warn.length() > 0)
        {
            // Warning
            std::wstring msg1 = L"warn in line 918\n";
            OutputDebugString(msg1.c_str());
            std::string msg = std::string(warn.begin(), warn.end());
            MessageBoxA(0, msg.c_str(), 0, 0);
            return E_FAIL;
        }

        // Parse the GLTF data
        assert(ParseGLTF(gltfData, config, binary, scene), "parse scene file!\n");

        // Serialize the scene and store a cache file to speed up future loads
        // if (!Caches::Serialize(sceneCache, scene, log)) return false;

        // Add config specific cameras and lights
        // ParseConfigCamerasLights(config, scene);

        return S_OK;
    }

    /**
     * Traverse the scene graph and update the instance transforms.
     */
    void Traverse(size_t nodeIndex, XMMATRIX transform, Scene& scene)
    {
        // Get the node
        SceneNode node = scene.nodes[nodeIndex];

        // Get the node's local transform
        XMMATRIX nodeTransform;
        if (node.hasMatrix)
        {
            nodeTransform = node.matrix;
        }
        else
        {
            // Compose the node's local transform, M = T * R * S
            XMMATRIX t = XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);
            XMMATRIX r = XMMatrixRotationQuaternion(XMLoadFloat4(&node.rotation));
            XMMATRIX s = XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);    // Note: do not use negative scale factors! This will flip the object inside and cause incorrect normals.
            nodeTransform = XMMatrixMultiply(XMMatrixMultiply(s, r), t);
        }

        // Compose the global transform
        transform = XMMatrixMultiply(nodeTransform, transform);

        // When at a leaf node with a mesh, update the mesh instance's transform
        // Not currently supporting nested transforms for camera nodes
        if (node.children.size() == 0 && node.instance > -1)
        {
            // Update the instance's transform data
            MeshInstance* instance = &scene.instances[node.instance];
            XMMATRIX transpose = XMMatrixTranspose(transform);
            memcpy(instance->transform, &transpose, sizeof(XMFLOAT4) * 3);
            return;
        }

        // Recursively traverse the scene graph
        for (size_t i = 0; i < node.children.size(); i++)
        {
            Traverse(node.children[i], transform, scene);
        }
    }

    void UpdateCamera(Camera& camera)
    {
        XMFLOAT3 up = XMFLOAT3(0.f, 1.f, 0.f);
        XMFLOAT3 forward = XMFLOAT3(0.f, 0.f, -1.f);
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(-camera.pitch * (XM_PI / 180.f), -camera.yaw * (XM_PI / 180.f), 0.f);

        XMFLOAT3 cameraRight, cameraUp, cameraForward;
        XMStoreFloat3(&cameraForward, XMVector3Normalize(XMVector3Transform(XMLoadFloat3(&forward), rotation)));

        XMStoreFloat3(&cameraRight, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&cameraForward), XMLoadFloat3(&up))));
        XMStoreFloat3(&cameraUp, XMVector3Cross(-XMLoadFloat3(&cameraForward), XMLoadFloat3(&cameraRight)));

        camera.data.right = { cameraRight.x, cameraRight.y, cameraRight.z };
        camera.data.up = { cameraUp.x, cameraUp.y, cameraUp.z };
        camera.data.forward = { cameraForward.x, cameraForward.y, cameraForward.z };
    }

    /**
     * Releases memory used by the scene.
     */
    void Cleanup(Scene& scene)
    {
        // Release texture memory
        for (size_t textureIndex = 0; textureIndex < scene.textures.size(); textureIndex++)
        {
            Unload(scene.textures[textureIndex]);
        }
    }

    HRESULT CreateAndUploadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, 
        Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer, const NTexture& texture)
    {
        // std::vector<ID3D12Resource*>* tex = new std::vector<ID3D12Resource*>();
        // std::vector<ID3D12Resource*>* upload = new std::vector<ID3D12Resource*>();
        // if (texture.type == ETextureType::SCENE)
        // {
        //     tex = &resources.sceneTextures;
        //     upload = &resources.sceneTextureUploadBuffers;
        // }
        // else 
        if (texture.type == ETextureType::ENGINE)
        {
            // tex = &resources.textures;
            // upload = &resources.textureUploadBuffers;
            MessageBox(0, L"texture.type == ETextureType::ENGINE", 0, 0);
            return E_FAIL;
        }

        // tex->emplace_back();
        // upload->emplace_back();

        // ID3D12Resource*& resource = tex->back();
        // ID3D12Resource*& uploadBuffer = upload->back();

        // Create the default heap texture resource
        {
            TextureDesc desc = { texture.width, texture.height, 1, texture.mips, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE };
            if (texture.format == ETextureFormat::BC7) desc.format = DXGI_FORMAT_BC7_TYPELESS;
            if (!CreateTexture(device, desc, resource))
            {
                MessageBox(0, L"create the texture default heap resource!", 0, 0);
                return E_FAIL;
            };

            std::string name = "Texture: " + texture.name;
            std::wstring wname = std::wstring(name.begin(), name.end());
            resource->SetName(wname.c_str());

        }

        // Create the upload heap buffer resource
        {
            BufferDesc desc = { texture.texelBytes, 1, EHeapType::UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ , D3D12_RESOURCE_FLAG_NONE };
            if (!CreateBuffer(device, desc, uploadBuffer))
            {
                MessageBox(0, L"create the texture upload heap buffer!", 0, 0);
                return E_FAIL;
            }

            std::string name = " Texture Upload Buffer: " + texture.name;
            std::wstring wname = std::wstring(name.begin(), name.end());
            uploadBuffer->SetName(wname.c_str());

        }

        // Copy the texel data to the upload buffer resource
        {
            UINT8* pData = nullptr;
            D3D12_RANGE range = { 0, 0 };
            if (FAILED(uploadBuffer->Map(0, &range, reinterpret_cast<void**>(&pData)))) return E_FAIL;

            if (texture.format == ETextureFormat::BC7)
            {
                // Aligned, copy all the image pixels
                memcpy(pData, texture.texels, texture.texelBytes);
            }
            else if (texture.format == ETextureFormat::UNCOMPRESSED)
            {
                UINT rowSize = texture.width * texture.stride;
                UINT rowPitch = ALIGN(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, rowSize);
                if (rowSize == rowPitch)
                {
                    // Aligned, copy the all image pixels
                    memcpy(pData, texture.texels, texture.texelBytes);
                }
                else
                {
                    // RowSize is *not* aligned to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
                    // Copy each row of the image and add padding to match the row pitch alignment
                    UINT8* pSource = texture.texels;
                    for (UINT rowIndex = 0; rowIndex < texture.height; rowIndex++)
                    {
                        memcpy(pData, texture.texels, rowSize);
                        pData += rowPitch;
                        pSource += rowSize;
                    }
                }
            }

            uploadBuffer->Unmap(0, &range);
        }

        // Schedule a copy the of the upload heap resource to the default heap resource, then transition it to a shader resource
        {
            // Describe the texture
            D3D12_RESOURCE_DESC texDesc = {};
            texDesc.Width = texture.width;
            texDesc.Height = texture.height;
            texDesc.MipLevels = texture.mips;
            texDesc.DepthOrArraySize = 1;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (texture.format == ETextureFormat::BC7) texDesc.Format = DXGI_FORMAT_BC7_TYPELESS;

            // Get the resource footprints and total upload buffer size
            UINT64 uploadSize = 0;
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
            std::vector<UINT> numRows;
            std::vector<UINT64> rowSizes;
            footprints.resize(texture.mips);
            numRows.resize(texture.mips);
            rowSizes.resize(texture.mips);
            device->GetCopyableFootprints(&texDesc, 0, texture.mips, 0, footprints.data(), numRows.data(), rowSizes.data(), &uploadSize);
            assert(uploadSize <= texture.texelBytes);

            // Describe the upload buffer resource (source)
            D3D12_TEXTURE_COPY_LOCATION source = {};
            source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            source.pResource = uploadBuffer.Get();

            // Describe the default heap resource (destination)
            D3D12_TEXTURE_COPY_LOCATION destination = {};
            destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destination.pResource = resource.Get();

            // Copy each texture mip level from the upload heap to default heap
            for (UINT mipIndex = 0; mipIndex < texture.mips; mipIndex++)
            {
                // Update the mip level footprint
                source.PlacedFootprint = footprints[mipIndex];

                // Update the destination mip level
                destination.SubresourceIndex = mipIndex;

                // Copy the texture from the upload heap to the default heap
                cmdList->CopyTextureRegion(&destination, 0, 0, 0, &source, NULL);
            }

            // Transition the default heap texture resource to a shader resource
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = resource.Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            cmdList->ResourceBarrier(1, &barrier);
        }

        return S_OK;
    }
    /**
     * Create the scene textures.
    bool CreateSceneTextures(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TexResource& resources, const Scenes::Scene& scene, std::ofstream& log)
    {
        // Early out if there are no scene textures
        if (scene.textures.size() == 0) return true;

        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = resources.srvDescHeapStart.ptr + (DescriptorHeapOffsets::SRV_SCENE_TEXTURES * resources.srvDescHeapEntrySize);

        // Create the default and upload heap texture resources
        for (UINT textureIndex = 0; textureIndex < static_cast<UINT>(scene.textures.size()); textureIndex++)
        {
            // Get the texture
            const NTexture texture = scene.textures[textureIndex];

            // Create the GPU texture resources, upload the texture data, and schedule a copy
            if (!CreateAndUploadTexture(device, cmdList, resources, texture))
            {
                MessageBox(0, L"create and upload scene texture!\n", 0, 0);
            };

            // Add the texture SRV to the descriptor heap
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = texture.mips;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (texture.format == ETextureFormat::UNCOMPRESSED) srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            else if (texture.format == ETextureFormat::BC7) srvDesc.Format = DXGI_FORMAT_BC7_UNORM;

            device->CreateShaderResourceView(resources.sceneTextures[textureIndex], &srvDesc, handle);

            // Move to the next slot on the descriptor heap
            handle.ptr += resources.srvDescHeapEntrySize;
        }

        return true;
    }*/
}
