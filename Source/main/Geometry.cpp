#include "geometry.h"
#include "./Geometry/GLTFLoader.h"
#include <assimp/include/assimp/cimport.h>
#include <assimp/include/assimp/Importer.hpp>
#include <assimp/include/assimp/scene.h>
#include <assimp/include/assimp/postprocess.h>

// #pragma comment(lib, "assimp/lib/assimp_release-dll_win32/assimp.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

LampGeo::LampGeo(Microsoft::WRL::ComPtr <ID3D12Device> d3dDevice)
{
    md3dDevice = d3dDevice;
}

LampGeo::~LampGeo()
{
    mRitemLayer->clear();
}

void LampGeo::BuildShapeGeometry(ID3D12GraphicsCommandList* mCommandList)
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
    GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    GeometryGenerator::MeshData ball = geoGen.CreateSphere(0.5f, 8, 8);

    //
    // We are concatenating all the geometry into one big vertex/index buffer.  So
    // define the regions in the buffer each submesh covers.
    //

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = (UINT)box.Vertices.size();
    UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
    UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
    UINT quadVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
    UINT ballVertexOffset = quadVertexOffset + (UINT)quad.Vertices.size();

    // Cache the starting index for each object in the concatenated index buffer.
    UINT boxIndexOffset = 0;
    UINT gridIndexOffset = (UINT)box.Indices32.size();
    UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
    UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
    UINT quadIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
    UINT ballIndexOffset = quadIndexOffset + (UINT)quad.Indices32.size();

    Submesh boxSubmesh;
    boxSubmesh.IndexCount = (UINT)box.Indices32.size();
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    Submesh gridSubmesh;
    gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;

    Submesh sphereSubmesh;
    sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

    Submesh cylinderSubmesh;
    cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
    cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

    Submesh quadSubmesh;
    quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
    quadSubmesh.StartIndexLocation = quadIndexOffset;
    quadSubmesh.BaseVertexLocation = quadVertexOffset;

    Submesh ballSubmesh;
    ballSubmesh.IndexCount = (UINT)ball.Indices32.size();
    ballSubmesh.StartIndexLocation = ballIndexOffset;
    ballSubmesh.BaseVertexLocation = ballVertexOffset;
    //
    // Extract the vertex elements we are interested in and pack the
    // vertices of all the meshes into one vertex buffer.
    //

    auto totalVertexCount =
        box.Vertices.size() +
        grid.Vertices.size() +
        sphere.Vertices.size() +
        cylinder.Vertices.size() +
        quad.Vertices.size() +
        ball.Vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
    {
        vertices[k].position = box.Vertices[i].Position;
        vertices[k].normal = box.Vertices[i].Normal;
        vertices[k].uv0 = box.Vertices[i].TexC;
        XMFLOAT3 temp = box.Vertices[i].TangentU;
        vertices[k].tangent = XMFLOAT4(temp.x, temp.y, temp.z, 1);
    }

    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
    {
        vertices[k].position = grid.Vertices[i].Position;
        vertices[k].normal = grid.Vertices[i].Normal;
        vertices[k].uv0 = grid.Vertices[i].TexC;
        XMFLOAT3 temp = grid.Vertices[i].TangentU;
        vertices[k].tangent = XMFLOAT4(temp.x, temp.y, temp.z, 1);
    }

    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
    {
        vertices[k].position = sphere.Vertices[i].Position;
        vertices[k].normal = sphere.Vertices[i].Normal;
        vertices[k].uv0 = sphere.Vertices[i].TexC;
        XMFLOAT3 temp = sphere.Vertices[i].TangentU;
        vertices[k].tangent = XMFLOAT4(temp.x, temp.y, temp.z, 1);
    }

    for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
    {
        vertices[k].position = cylinder.Vertices[i].Position;
        vertices[k].normal = cylinder.Vertices[i].Normal;
        vertices[k].uv0 = cylinder.Vertices[i].TexC;
        XMFLOAT3 temp = cylinder.Vertices[i].TangentU;
        vertices[k].tangent = XMFLOAT4(temp.x, temp.y, temp.z, 1);
    }

    for (size_t i = 0; i < quad.Vertices.size(); ++i, ++k)
    {
        vertices[k].position = quad.Vertices[i].Position;
        vertices[k].normal = quad.Vertices[i].Normal;
        vertices[k].uv0 = quad.Vertices[i].TexC;
        XMFLOAT3 temp = quad.Vertices[i].TangentU;
        vertices[k].tangent = XMFLOAT4(temp.x, temp.y, temp.z, 1);
    }

    for (size_t i = 0; i < ball.Vertices.size(); ++i, ++k)
    {
        vertices[k].position = ball.Vertices[i].Position;
        vertices[k].normal = ball.Vertices[i].Normal;
        vertices[k].uv0 = ball.Vertices[i].TexC;
        XMFLOAT3 temp = ball.Vertices[i].TangentU;
        vertices[k].tangent = XMFLOAT4(temp.x, temp.y, temp.z, 1);
    }

    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
    indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
    indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));
    indices.insert(indices.end(), std::begin(ball.GetIndices16()), std::end(ball.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<Mesh>();
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList, indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs["box"] = boxSubmesh;
    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["sphere"] = sphereSubmesh;
    geo->DrawArgs["cylinder"] = cylinderSubmesh;
    geo->DrawArgs["quad"] = quadSubmesh;
    geo->DrawArgs["ball"] = ballSubmesh;

    mGeometries[geo->Name] = std::move(geo);
}

void LampGeo::BuildSkullGeometry(ID3D12GraphicsCommandList* mCommandList)
{
    std::ifstream fin("Models/skull.txt");

    if (!fin)
    {
        MessageBox(0, L"Models/skull.txt not found.", 0, 0);
        return;
    }

    UINT vcount = 0;
    UINT tcount = 0;
    std::string ignore;

    fin >> ignore >> vcount;
    fin >> ignore >> tcount;
    fin >> ignore >> ignore >> ignore >> ignore;

    XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
    XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

    XMVECTOR vMin = XMLoadFloat3(&vMinf3);
    XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

    std::vector<Vertex> vertices(vcount);
    for (UINT i = 0; i < vcount; ++i)
    {
        fin >> vertices[i].position.x >> vertices[i].position.y >> vertices[i].position.z;
        fin >> vertices[i].normal.x >> vertices[i].normal.y >> vertices[i].normal.z;

        vertices[i].uv0 = { 0.0f, 0.0f };

        XMVECTOR P = XMLoadFloat3(&vertices[i].position);

        XMVECTOR N = XMLoadFloat3(&vertices[i].normal);

        // Generate a tangent vector so normal mapping works.  We aren't applying
        // a texture map to the skull, so we just need any tangent vector so that
        // the math works out to give us the original interpolated vertex normal.
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f)
        {
            XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
            XMStoreFloat4(&vertices[i].tangent, T);
        }
        else
        {
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
            XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
            XMStoreFloat4(&vertices[i].tangent, T);
        }


        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    BoundingBox bounds;
    XMStoreFloat3(&bounds.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&bounds.Extents, 0.5f * (vMax - vMin));

    fin >> ignore;
    fin >> ignore;
    fin >> ignore;

    std::vector<std::int32_t> indices(3 * tcount);
    for (UINT i = 0; i < tcount; ++i)
    {
        fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
    }

    fin.close();

    //
    // Pack the indices of all the meshes into one index buffer.
    //

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

    auto geo = std::make_unique<Mesh>();
    geo->Name = "skullGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList, indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    Submesh submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;
    submesh.Bounds = bounds;

    geo->DrawArgs["skull"] = submesh;

    mGeometries[geo->Name] = std::move(geo);
}

void LampGeo::BuildMaterials()
{
    auto bricks0 = std::make_unique<Material>();
    bricks0->Name = "bricks0";
    bricks0->MatCBIndex = 0;
    bricks0->DiffuseSrvHeapIndex = 0;
    bricks0->NormalSrvHeapIndex = 1;
    bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    bricks0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    bricks0->Roughness = 0.8f;

    auto tile0 = std::make_unique<Material>();
    tile0->Name = "tile0";
    tile0->MatCBIndex = 1;
    tile0->DiffuseSrvHeapIndex = 2;
    tile0->NormalSrvHeapIndex = 3;
    tile0->DiffuseAlbedo = XMFLOAT4(0.85f, 0.73f, 0.6f, 1.0f);
    tile0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    tile0->Roughness = 0.7f;

    auto mirror0 = std::make_unique<Material>();
    mirror0->Name = "mirror0";
    mirror0->MatCBIndex = 2;
    mirror0->DiffuseSrvHeapIndex = 4;
    mirror0->NormalSrvHeapIndex = 5;
    mirror0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    mirror0->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
    mirror0->Roughness = 0.001f;

    auto skullMat = std::make_unique<Material>();
    skullMat->Name = "skullMat";
    skullMat->MatCBIndex = 3;
    skullMat->DiffuseSrvHeapIndex = 4;
    skullMat->NormalSrvHeapIndex = 5;
    skullMat->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
    skullMat->FresnelR0 = XMFLOAT3(0.8f, 0.8f, 0.8f);
    skullMat->Roughness = 0.3f;

    auto sky = std::make_unique<Material>();
    sky->Name = "sky";
    sky->MatCBIndex = 4;
    sky->DiffuseSrvHeapIndex = 6;
    sky->NormalSrvHeapIndex = 7;
    sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    sky->Roughness = 1.0f;

    auto red0 = std::make_unique<Material>();
    red0->Name = "red0";
    red0->MatCBIndex = 5;
    red0->DiffuseSrvHeapIndex = 4;
    red0->NormalSrvHeapIndex = 5;
    red0->DiffuseAlbedo = XMFLOAT4(0.85f, 0.0f, 0.0f, 1.0f);
    red0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    red0->Roughness = 0.95f;

    auto green0 = std::make_unique<Material>();
    green0->Name = "green0";
    green0->MatCBIndex = 6;
    green0->DiffuseSrvHeapIndex = 4;
    green0->NormalSrvHeapIndex = 5;
    green0->DiffuseAlbedo = XMFLOAT4(0.0f, 0.85f, 0.0f, 1.0f);
    green0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    green0->Roughness = 0.95f;

    auto blue0 = std::make_unique<Material>();
    blue0->Name = "blue0";
    blue0->MatCBIndex = 7;
    blue0->DiffuseSrvHeapIndex = 4;
    blue0->NormalSrvHeapIndex = 5;
    blue0->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.85f, 1.0f);
    blue0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    blue0->Roughness = 0.95f;

    auto grey0 = std::make_unique<Material>();
    grey0->Name = "grey0";
    grey0->MatCBIndex = 8;
    grey0->DiffuseSrvHeapIndex = 4;
    grey0->NormalSrvHeapIndex = 5;
    grey0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    grey0->FresnelR0 = XMFLOAT3(0.04f, 0.04f, 0.04f);
    grey0->Roughness = 0.015f;

    auto latern0 = std::make_unique<Material>();
    latern0->Name = "latern0";
    latern0->MatCBIndex = 9;
    latern0->DiffuseSrvHeapIndex = 6;
    latern0->NormalSrvHeapIndex = 5;
    latern0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    latern0->FresnelR0 = XMFLOAT3(0.04f, 0.04f, 0.04f);
    latern0->Roughness = 0.03f;

    mMaterials["bricks0"] = std::move(bricks0);
    mMaterials["tile0"] = std::move(tile0);
    mMaterials["mirror0"] = std::move(mirror0);
    mMaterials["skullMat"] = std::move(skullMat);
    mMaterials["sky"] = std::move(sky);
    mMaterials["red0"] = std::move(red0);
    mMaterials["green0"] = std::move(green0);
    mMaterials["blue0"] = std::move(blue0);
    mMaterials["grey0"] = std::move(grey0);
    mMaterials["latern0"] = std::move(latern0);
}

void LampGeo::BuildRenderItems()
{
    UINT INDEX = 0;
    auto skyRitem = std::make_unique<RenderItem>();
    {
        XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
        skyRitem->TexTransform = MathHelper::Identity4x4();
        skyRitem->ObjCBIndex = INDEX++;
        skyRitem->Mat = mMaterials["sky"].get();
        skyRitem->Geo = mGeometries["shapeGeo"].get();
        skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
        skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
    }
    mRitemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
    mAllRitems.push_back(std::move(skyRitem));

    auto quadRitem = std::make_unique<RenderItem>();
    {
        quadRitem->World = MathHelper::Identity4x4();
        quadRitem->TexTransform = MathHelper::Identity4x4();
        quadRitem->ObjCBIndex = INDEX++;
        quadRitem->Mat = mMaterials["bricks0"].get();
        quadRitem->Geo = mGeometries["shapeGeo"].get();
        quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        quadRitem->IndexCount = quadRitem->Geo->DrawArgs["quad"].IndexCount;
        quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["quad"].StartIndexLocation;
        quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
    }
    mRitemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
    mAllRitems.push_back(std::move(quadRitem));

    auto boxRitem = std::make_unique<RenderItem>();
    {
        XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 1.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
        XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
        boxRitem->ObjCBIndex = INDEX++;
        boxRitem->Mat = mMaterials["bricks0"].get();
        boxRitem->Geo = mGeometries["shapeGeo"].get();
        boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
        boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
        boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    }
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    mAllRitems.push_back(std::move(boxRitem));

    auto boxRitem1 = std::make_unique<RenderItem>();
    {
        XMStoreFloat4x4(&boxRitem1->World, XMMatrixScaling(3.0f, 3.0f, 3.0f) * XMMatrixTranslation(5.0f, 1.5f, 1.0f));
        XMStoreFloat4x4(&boxRitem1->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
        boxRitem1->ObjCBIndex = INDEX++;
        boxRitem1->Mat = mMaterials["red0"].get();
        boxRitem1->Geo = mGeometries["shapeGeo"].get();
        boxRitem1->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        boxRitem1->IndexCount = boxRitem1->Geo->DrawArgs["box"].IndexCount;
        boxRitem1->StartIndexLocation = boxRitem1->Geo->DrawArgs["box"].StartIndexLocation;
        boxRitem1->BaseVertexLocation = boxRitem1->Geo->DrawArgs["box"].BaseVertexLocation;
    }
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem1.get());
    mAllRitems.push_back(std::move(boxRitem1));

    auto boxRitem2 = std::make_unique<RenderItem>();
    {
        XMStoreFloat4x4(&boxRitem2->World, XMMatrixScaling(3.0f, 3.0f, 3.0f) * XMMatrixTranslation(-3.0f, 1.5f, 0.0f));
        XMStoreFloat4x4(&boxRitem2->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
        boxRitem2->ObjCBIndex = INDEX++;
        boxRitem2->Mat = mMaterials["blue0"].get();
        boxRitem2->Geo = mGeometries["shapeGeo"].get();
        boxRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        boxRitem2->IndexCount = boxRitem2->Geo->DrawArgs["box"].IndexCount;
        boxRitem2->StartIndexLocation = boxRitem2->Geo->DrawArgs["box"].StartIndexLocation;
        boxRitem2->BaseVertexLocation = boxRitem2->Geo->DrawArgs["box"].BaseVertexLocation;
    }
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem2.get());
    mAllRitems.push_back(std::move(boxRitem2));


    auto skullRitem = std::make_unique<RenderItem>();
    {
        XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.4f, 0.4f, 0.4f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
        skullRitem->TexTransform = MathHelper::Identity4x4();
        skullRitem->ObjCBIndex = INDEX++;
        skullRitem->Mat = mMaterials["skullMat"].get();
        skullRitem->Geo = mGeometries["skullGeo"].get();
        skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
        skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
        skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
    }
    mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());
    mAllRitems.push_back(std::move(skullRitem));

    auto gridRitem = std::make_unique<RenderItem>();
    {
        gridRitem->World = MathHelper::Identity4x4();
        XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
        gridRitem->ObjCBIndex = INDEX++;
        gridRitem->Mat = mMaterials["tile0"].get();
        gridRitem->Geo = mGeometries["shapeGeo"].get();
        gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
        gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
        gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
    }
    mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
    mAllRitems.push_back(std::move(gridRitem));

    std::vector<std::string> mats = {  "grey0", "red0", "grey0", "grey0", "grey0", "grey0", "blue0", "grey0", "grey0"};
    for (int i = 0; i < 8; i++)
    {
        auto sibenikRitem = std::make_unique<RenderItem>();
        //boxRitem3->World = MathHelper::Identity4x4();
        sibenikRitem->TexTransform = MathHelper::Identity4x4();
        XMStoreFloat4x4(&sibenikRitem->World, XMMatrixScaling(0.6f, 0.6f, 0.6f) * XMMatrixTranslation(0.0f, 8.2f, 8.0f));
        // XMStoreFloat4x4(&boxRitem3->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
        sibenikRitem->ObjCBIndex = INDEX++;
        sibenikRitem->Mat = mMaterials[mats[i%9]].get();
        sibenikRitem->Geo = mGeometries["sibenik"].get();
        sibenikRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        sibenikRitem->IndexCount = sibenikRitem->Geo->DrawArgs[std::to_string(i)].IndexCount;
        sibenikRitem->StartIndexLocation = sibenikRitem->Geo->DrawArgs[std::to_string(i)].StartIndexLocation;
        sibenikRitem->BaseVertexLocation = sibenikRitem->Geo->DrawArgs[std::to_string(i)].BaseVertexLocation;
        mRitemLayer[(int)RenderLayer::Opaque].push_back(sibenikRitem.get());
        mAllRitems.push_back(std::move(sibenikRitem));
    }
    auto sibenikOutsideWallRitem = std::make_unique<RenderItem>();
    //boxRitem3->World = MathHelper::Identity4x4();
    sibenikOutsideWallRitem->TexTransform = MathHelper::Identity4x4();
    XMStoreFloat4x4(&sibenikOutsideWallRitem->World, XMMatrixScaling(0.6f, 0.6f, 0.6f)* XMMatrixTranslation(0.0f, 8.2f, 8.0f));
    // XMStoreFloat4x4(&boxRitem3->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
    sibenikOutsideWallRitem->ObjCBIndex = INDEX++;
    sibenikOutsideWallRitem->Mat = mMaterials[mats[0]].get();
    sibenikOutsideWallRitem->Geo = mGeometries["sibenik"].get();
    sibenikOutsideWallRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    sibenikOutsideWallRitem->IndexCount = sibenikOutsideWallRitem->Geo->DrawArgs[std::to_string(8)].IndexCount;
    sibenikOutsideWallRitem->StartIndexLocation = sibenikOutsideWallRitem->Geo->DrawArgs[std::to_string(8)].StartIndexLocation;
    sibenikOutsideWallRitem->BaseVertexLocation = sibenikOutsideWallRitem->Geo->DrawArgs[std::to_string(8)].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Wall].push_back(sibenikOutsideWallRitem.get());
    mAllRitems.push_back(std::move(sibenikOutsideWallRitem));

    auto sibenikRitem = std::make_unique<RenderItem>();
    //boxRitem3->World = MathHelper::Identity4x4();S
    sibenikRitem->TexTransform = MathHelper::Identity4x4();
    XMStoreFloat4x4(&sibenikRitem->World, XMMatrixScaling(0.6f, 0.6f, 0.6f)* XMMatrixTranslation(0.0f, 8.2f, 8.0f));
    // XMStoreFloat4x4(&boxRitem3->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
    sibenikRitem->ObjCBIndex = INDEX++;
    sibenikRitem->Mat = mMaterials[mats[0]].get();
    sibenikRitem->Geo = mGeometries["sibenik"].get();
    sibenikRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    sibenikRitem->IndexCount = sibenikRitem->Geo->DrawArgs[std::to_string(9)].IndexCount;
    sibenikRitem->StartIndexLocation = sibenikRitem->Geo->DrawArgs[std::to_string(9)].StartIndexLocation;
    sibenikRitem->BaseVertexLocation = sibenikRitem->Geo->DrawArgs[std::to_string(9)].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(sibenikRitem.get());
    mAllRitems.push_back(std::move(sibenikRitem));

    /*
    auto LaternRitem = std::make_unique<RenderItem>();
    //boxRitem3->World = MathHelper::Identity4x4();
    LaternRitem->TexTransform = MathHelper::Identity4x4();
    XMStoreFloat4x4(&LaternRitem->World, XMMatrixTranslation(-3.823154f, 13.016030f, 26.5f)* XMMatrixScaling(0.2f, 0.2f, 0.2f));
    // XMStoreFloat4x4(&boxRitem3->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
    LaternRitem->ObjCBIndex = INDEX++;
    LaternRitem->Mat = mMaterials["latern0"].get();
    LaternRitem->Geo = mGeometries["LanternPole_Body"].get();
    LaternRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    LaternRitem->IndexCount = LaternRitem->Geo->DrawArgs["LanternPole_Body"].IndexCount;
    LaternRitem->StartIndexLocation = LaternRitem->Geo->DrawArgs["LanternPole_Body"].StartIndexLocation;
    LaternRitem->BaseVertexLocation = LaternRitem->Geo->DrawArgs["LanternPole_Body"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(LaternRitem.get());
    mAllRitems.push_back(std::move(LaternRitem));

    auto LaternRitem1 = std::make_unique<RenderItem>();
    //boxRitem3->World = MathHelper::Identity4x4();
    LaternRitem1->TexTransform = MathHelper::Identity4x4();
    XMStoreFloat4x4(&LaternRitem1->World, XMMatrixTranslation(-9.582001f, 21.037872f, 26.5f)* XMMatrixScaling(0.2f, 0.2f, 0.2f));
    // XMStoreFloat4x4(&boxRitem3->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
    LaternRitem1->ObjCBIndex = INDEX++;
    LaternRitem1->Mat = mMaterials["latern0"].get();
    LaternRitem1->Geo = mGeometries["LanternPole_Chain"].get();
    LaternRitem1->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    LaternRitem1->IndexCount = LaternRitem1->Geo->DrawArgs["LanternPole_Chain"].IndexCount;
    LaternRitem1->StartIndexLocation = LaternRitem1->Geo->DrawArgs["LanternPole_Chain"].StartIndexLocation;
    LaternRitem1->BaseVertexLocation = LaternRitem1->Geo->DrawArgs["LanternPole_Chain"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(LaternRitem1.get());
    mAllRitems.push_back(std::move(LaternRitem1));

    auto LaternRitem2 = std::make_unique<RenderItem>();
    //boxRitem3->World = MathHelper::Identity4x4();
    LaternRitem2->TexTransform = MathHelper::Identity4x4();
    XMStoreFloat4x4(&LaternRitem2->World, XMMatrixTranslation(-9.582007, 18.009151, 26.5f)* XMMatrixScaling(0.2f, 0.2f, 0.2f));
    // XMStoreFloat4x4(&boxRitem3->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
    LaternRitem2->ObjCBIndex = INDEX++;
    LaternRitem2->Mat = mMaterials["latern0"].get();
    LaternRitem2->Geo = mGeometries["LanternPole_Lantern"].get();
    LaternRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    LaternRitem2->IndexCount = LaternRitem2->Geo->DrawArgs["LanternPole_Lantern"].IndexCount;
    LaternRitem2->StartIndexLocation = LaternRitem2->Geo->DrawArgs["LanternPole_Lantern"].StartIndexLocation;
    LaternRitem2->BaseVertexLocation = LaternRitem2->Geo->DrawArgs["LanternPole_Lantern"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(LaternRitem2.get());
    mAllRitems.push_back(std::move(LaternRitem2));
    
    auto ballRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&ballRitem->World, XMMatrixScaling(2.5f, 2.5f, 2.5f)* XMMatrixTranslation(0.0f, 6.5f, 8.0f));
    XMStoreFloat4x4(&ballRitem->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
    ballRitem->ObjCBIndex = INDEX++;
    ballRitem->Mat = mMaterials["mirror0"].get();
    ballRitem->Geo = mGeometries["shapeGeo"].get();
    ballRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    ballRitem->IndexCount = ballRitem->Geo->DrawArgs["sphere"].IndexCount;
    ballRitem->StartIndexLocation = ballRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
    ballRitem->BaseVertexLocation = ballRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(ballRitem.get());
    mAllRitems.push_back(std::move(ballRitem));
    */
    XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
    UINT objCBIndex = INDEX++;
    for (int i = 0; i < 3; ++i)
    {
        auto leftCylRitem = std::make_unique<RenderItem>();
        auto rightCylRitem = std::make_unique<RenderItem>();
        auto leftSphereRitem = std::make_unique<RenderItem>();
        auto rightSphereRitem = std::make_unique<RenderItem>();

        XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
        XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

        XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
        XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
        leftCylRitem->ObjCBIndex = objCBIndex++;
        leftCylRitem->Mat = mMaterials["bricks0"].get();
        leftCylRitem->Geo = mGeometries["shapeGeo"].get();
        leftCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
        XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
        rightCylRitem->ObjCBIndex = objCBIndex++;
        rightCylRitem->Mat = mMaterials["bricks0"].get();
        rightCylRitem->Geo = mGeometries["shapeGeo"].get();
        rightCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
        leftSphereRitem->TexTransform = MathHelper::Identity4x4();
        leftSphereRitem->ObjCBIndex = objCBIndex++;
        leftSphereRitem->Mat = mMaterials["mirror0"].get();
        leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
        leftSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
        rightSphereRitem->TexTransform = MathHelper::Identity4x4();
        rightSphereRitem->ObjCBIndex = objCBIndex++;
        rightSphereRitem->Mat = mMaterials["mirror0"].get();
        rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
        rightSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

        mAllRitems.push_back(std::move(leftCylRitem));
        mAllRitems.push_back(std::move(rightCylRitem));
        mAllRitems.push_back(std::move(leftSphereRitem));
        mAllRitems.push_back(std::move(rightSphereRitem));
    }
}

void LampGeo::LoadTextures(ID3D12GraphicsCommandList* mCommandList)
{
    std::vector<std::string> texNames =
    {
        "bricksDiffuseMap",
        "bricksNormalMap",
        "tileDiffuseMap",
        "tileNormalMap",
        "defaultDiffuseMap",
        "defaultNormalMap",
        "skyCubeMap"
    };

    std::vector<std::wstring> texFilenames =
    {
        L"../Textures/bricks2.dds",
        L"../Textures/bricks2_nmap.dds",
        L"../Textures/tile.dds",
        L"../Textures/tile_nmap.dds",
        L"../Textures/white1x1.dds",
        L"../Textures/default_nmap.dds",
        L"../Textures/grasscube1024.dds"
    };

    for (int i = 0; i < (int)texNames.size(); ++i)
    {
        auto texMap = std::make_unique<Texture>();
        texMap->Name = texNames[i];
        texMap->Filename = texFilenames[i];
        ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
            mCommandList, texMap->Filename.c_str(),
            texMap->Resource, texMap->UploadHeap));

        mTextures[texMap->Name] = std::move(texMap);
    }
}

std::vector<RenderItem*>& LampGeo::RenderItems(RenderLayer layer)
{
    return mRitemLayer[(int)layer];
}

Microsoft::WRL::ComPtr<ID3D12Resource> LampGeo::TextureRes(std::string name)
{
    return mTextures[name]->Resource;
}

HRESULT LampGeo::LoadOBJ(ID3D12GraphicsCommandList* mCommandList, const char* fileName)
{
    // UINT flags;
    
    const aiScene* pModel = aiImportFile(fileName, aiProcess_Triangulate);
    if (!pModel || pModel->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        MessageBox(0, L"Models/OBJ/sibenik/sibenik.obj not found.", 0, 0);
        return E_FAIL;
    }

    if (pModel->mNumMeshes)
    {
        UINT numMeshes = pModel->mNumMeshes;
        std::vector<std::pair<UINT, UINT>> meshMaterials(numMeshes);
        for (UINT pMesh = 0; pMesh < numMeshes; ++pMesh)
        {
            meshMaterials[pMesh] = std::pair<UINT, UINT>(pMesh, pModel->mMeshes[pMesh]->mMaterialIndex);
        }

        std::sort(meshMaterials.begin(), meshMaterials.end(), [](std::pair<UINT, UINT> a, std::pair<UINT, UINT> b) { return a.second < b.second; });

        m_IndexOffsets.resize(numMeshes);
        m_VertexOffsets.resize(numMeshes);

        UINT totalIndices = 0;
        UINT totalVertices = 0;

        // Count all the indices and vertices first
        for (UINT meshID = 0; meshID < numMeshes; ++meshID)
        {
            m_IndexOffsets[meshID] = totalIndices;
            m_VertexOffsets[meshID] = totalVertices;
            UINT sceneMesh = meshMaterials[meshID].first;

            totalIndices += pModel->mMeshes[sceneMesh]->mNumFaces * 3;
            totalVertices += pModel->mMeshes[sceneMesh]->mNumVertices;
            
            // std::wstring msg;
            // msg += L"m_IndexOffsets[" + std::to_wstring(meshID) + L"]: " + std::to_wstring(pModel->mMeshes[sceneMesh]->mNumFaces * 3) +
            // L"m_VertexOffsets[" + std::to_wstring(meshID) + L"]: " + std::to_wstring(pModel->mMeshes[sceneMesh]->mNumVertices) + L")\n";
            // OutputDebugString(msg.c_str());
        }

        std::vector<int32_t> indices(totalIndices);
        std::vector<Vertex> vertices(totalVertices);

        m_MeshToSceneMapping.resize(numMeshes);
        submeshes.resize(numMeshes);

        // Copy data into buffer images
        for (UINT meshID = 0; meshID < numMeshes; ++meshID)
        {

            UINT sceneMesh = meshMaterials[meshID].first;
            UINT indexOffset = m_IndexOffsets[meshID];
            UINT vertexOffset = m_VertexOffsets[meshID];

            const aiMesh* pMesh = pModel->mMeshes[sceneMesh];
            UINT numVertices = pMesh->mNumVertices;

            submeshes[meshID].IndexCount = pMesh->mNumFaces*3;
            submeshes[meshID].StartIndexLocation = indexOffset;
            submeshes[meshID].BaseVertexLocation = vertexOffset;

            m_MeshToSceneMapping[meshID] = meshMaterials[meshID].first;
            UINT index = indexOffset;
            // Indices
            for (UINT f = 0; f < pMesh->mNumFaces; ++f)
            {
                //memcpy(&indices[indexOffset + f * 3], pMesh->mFaces[f].mIndices, sizeof(int32_t) * 3);
                aiFace& face = pMesh->mFaces[f];
                for (UINT k = 0; k < face.mNumIndices; ++k)
                {
                    indices[index++] = face.mIndices[k];
                }
            }
            // vertices
            for (UINT v = 0; v < numVertices; v++)
            {
                Vertex& vertex = vertices[v + vertexOffset];
                aiVector3D position = pMesh->mVertices[v];
                vertex.position = XMFLOAT3(position.x, position.y, position.z);

                if (pMesh->HasNormals())
                {
                    aiVector3D normal = pMesh->mNormals[v];
                    vertex.normal = XMFLOAT3(normal.x, normal.y, normal.z);
                }

                if (pMesh->HasTangentsAndBitangents())
                {
                    aiVector3D tangent = pMesh->mTangents[v];
                    vertex.tangent = XMFLOAT4(tangent.x, tangent.y, tangent.z, 1);
                    //vertex.binormal = pMesh->mBitangents[v];
                }
                else
                {
                    XMVECTOR N = XMLoadFloat3(&vertex.normal);
                    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
                    if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 0.999f)
                    {
                        XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
                        XMStoreFloat4(&vertex.tangent, T);
                    }
                    else
                    {
                        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                        XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
                        XMStoreFloat4(&vertex.tangent, T);
                    }

                }

                if (pMesh->HasTextureCoords(0))
                {
                    vertex.uv0 = XMFLOAT2(pMesh->mTextureCoords[0][v].x, pMesh->mTextureCoords[0][v].y);
                }
            }
        }

        const UINT vbByteSize = totalVertices * sizeof(Vertex);

        const UINT ibByteSize = totalIndices * sizeof(std::int32_t);

        auto geo = std::make_unique<Mesh>();
        geo->Name = "sibenik";

        ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
        CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

        ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
        CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

        geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
            mCommandList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

        geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
            mCommandList, indices.data(), ibByteSize, geo->IndexBufferUploader);

        geo->VertexByteStride = sizeof(Vertex);
        geo->VertexBufferByteSize = vbByteSize;
        geo->IndexFormat = DXGI_FORMAT_R32_UINT;
        geo->IndexBufferByteSize = ibByteSize;

        Submesh submesh;
        submesh.IndexCount = (UINT)indices.size();
        submesh.StartIndexLocation = 0;
        submesh.BaseVertexLocation = 0;
        //submesh.Bounds = bounds;

        std::wstring msg;
        msg += L"numMesh: ( " + std::to_wstring(numMeshes) + L" )\n";
        OutputDebugString(msg.c_str());

        for (UINT i = 0; i < numMeshes; i++)
        {
            geo->DrawArgs[std::to_string(i)] = submeshes[i];

            std::wstring msg;
            msg += L"numMesh: ( " + std::to_wstring(i) 
                + L"), BaseVertexLocation: "
                + std::to_wstring(submeshes[i].BaseVertexLocation)
                + L", BaseIndexLocation: "
                + std::to_wstring(submeshes[i].StartIndexLocation)
                + L", IndexCount: "
                + std::to_wstring(submeshes[i].IndexCount) + L" )\n";
            OutputDebugString(msg.c_str());
        }

        mGeometries[geo->Name] = std::move(geo);
    }
    return S_OK;
}

HRESULT LampGeo::mLoadOBJ(ID3D12GraphicsCommandList* mCommandList)
{
    const char* fileName = "Models/OBJ/dragon.obj";

    std::ifstream fin(fileName);

    if (!fin)
    {
        MessageBox(0, L"Models/skull.txt not found.", 0, 0);
        return E_FAIL;
    }
    UINT vNum = 0;
    UINT vnNum = 0;
    UINT vtNum = 0;
    UINT tNum = 0;
    UINT fNum = 0;
    std::string s0;
    std::string s1;
    std::string s2;
    std::string s3;
    FLOAT f1;
    FLOAT f2;
    FLOAT f3;
    std::string sline;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> texcoord;
    std::vector<XMFLOAT3> positions;
    while (std::getline(fin, sline)) {//��ָ���ļ����ж�ȡ
        if (sline[0] == 'v') {
            if (sline[1] == 'n') {
                //vn
                std::istringstream ins(sline);
                ins >> s0 >> f1 >> f2 >> f3;
                normals.push_back(XMFLOAT3(f1, f2, f3));
                ++vnNum;
            }
            else if (sline[1] == 't') {
                //vt
                std::istringstream ins(sline);
                ins >> s0 >> f1 >> f2;
                texcoord.push_back(XMFLOAT2(f1, f2));
                ++vtNum;
            }
            else {
                //v
                std::istringstream ins(sline);
                ins >> s0 >> f1 >> f2 >> f3;
                positions.push_back(XMFLOAT3(f1, f2, f3));
                ++vNum;
            }
        }
        else if (sline[0] == 'f') {
            std::istringstream ins(sline);
            ins >> s0 >> s1 >> s2;
            fNum++;
        }
        else if (sline[0] == 't') {
            std::istringstream ins(sline);
            ins >> s0 >> s1 >> s2;
            tNum++;
        }
    }
    fin.close();

    std::vector<Vertex> vertices(fNum * 3);
    std::vector<int32_t> indices(fNum*3);

    std::ifstream infile2(fileName);
    UINT v1 = 0;
    UINT i1 = 0;
    BOOL hasTangent = tNum > 0;
    BOOL hasTexC = vtNum > 0;
    UINT fn = 0;
    while (getline(infile2, sline)) {
        if (sline[0] == 'f') {
            std::istringstream in(sline);
            float a;
            in >> s0;//ȥ��f
            UINT i, k;
            for (i = 0; i < 3; i++) {
                s0.clear();
                in >> s0;

                a = 0;
                for (k = 0; s0[k] != '/'; k++)
                    a = a * 10 + (s0[k] - 48); //0��ASCII����Ϊ48
                vertices[v1].position = positions[a - 1];
                indices[i1] = i1;
                if (hasTexC)
                {
                    a = 0;
                    for (k = k + 1; s0[k] != '/'; k++)
                        a = a * 10 + (s0[k] - 48);
                    vertices[v1].uv0 = XMFLOAT2(texcoord[a - 1].x, texcoord[a - 1].y);
                }
                else 
                {
                    k = k + 1;
                    vertices[v1].uv0 = XMFLOAT2(0, 0);
                }

                a = 0;
                for (k = k + 1; s0[k]; k++)
                    a = a * 10 + (s0[k] - 48);
                vertices[v1].normal = normals[a - 1];
                if (!hasTangent)
                {
                    XMVECTOR N = XMLoadFloat3(&vertices[v1].normal);
                    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
                    if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 0.999f)
                    {
                        XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
                        XMStoreFloat4(&vertices[v1].tangent, T);
                    }
                    else
                    {
                        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                        XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
                        XMStoreFloat4(&vertices[v1].tangent, T);
                    }
                }
                ++v1;
                ++i1;
            }
            ++fn;
        }
    }
    infile2.close();

    submeshes.resize(1);

    submeshes[0].IndexCount = i1;
    submeshes[0].StartIndexLocation = 0;
    submeshes[0].BaseVertexLocation = 0;

    const UINT vbByteSize = v1 * sizeof(Vertex);

    const UINT ibByteSize = i1 * sizeof(std::int32_t);

    auto geo = std::make_unique<Mesh>();
    geo->Name = "dragon";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList, indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    for (int i = 0; i < 1; i++)
    {
        geo->DrawArgs[std::to_string(i)] = submeshes[i];
    }

    mGeometries[geo->Name] = std::move(geo);

    return S_OK;
}

HRESULT LampGeo::LoadGLTF(ID3D12GraphicsCommandList* mCommandList)
{
    Scenes::Config conf;
    conf.scene.name = "Latern";
    conf.scene.file = "Lantern.gltf";
    conf.scene.path = "Models/Lantern/";
    Scenes::Scene scene;
    ThrowIfFailed(Scenes::Initialize(conf, scene));
    if (scene.numMeshPrimitives)
    {
        /*{
            std::wstring msg = L"NumMeshPrimitives" + std::to_wstring(scene.numMeshPrimitives);
            msg += L"scene.meshes.num: " + std::to_wstring(scene.meshes.size()) + L"\n";
            msg += L"scene.meshes[0].name: " + std::wstring(scene.meshes[0].name.begin(), scene.meshes[0].name.end()) + L"\n";
            msg += L"scene.meshes[1].name: " + std::wstring(scene.meshes[1].name.begin(), scene.meshes[1].name.end()) + L"\n";
            msg += L"scene.meshes[2].name: " + std::wstring(scene.meshes[2].name.begin(), scene.meshes[2].name.end()) + L"\n";

            msg += L"scene.nodes.size: " + std::to_wstring(scene.nodes.size()) + L"\n";
            msg += L"scene.nodes[0].trainsition: ("
                + std::to_wstring(scene.nodes[0].translation.x) + L", "
                + std::to_wstring(scene.nodes[0].translation.y) + L", "
                + std::to_wstring(scene.nodes[0].translation.z) + L")\n";
            msg += L"scene.nodes[0].scale: ("
                + std::to_wstring(scene.nodes[0].scale.x) + L", "
                + std::to_wstring(scene.nodes[0].scale.y) + L", "
                + std::to_wstring(scene.nodes[0].scale.z) + L")\n";
            msg += L"scene.nodes[0].scale: ("
                + std::to_wstring(scene.nodes[0].children.size()) + L")\n";
            msg += L"scene.nodes[0].trainsition: ("
                + std::to_wstring(scene.nodes[1].translation.x) + L", "
                + std::to_wstring(scene.nodes[1].translation.y) + L", "
                + std::to_wstring(scene.nodes[1].translation.z) + L")\n";
            msg += L"scene.nodes[0].trainsition: ("
                + std::to_wstring(scene.nodes[2].translation.x) + L", "
                + std::to_wstring(scene.nodes[2].translation.y) + L", "
                + std::to_wstring(scene.nodes[2].translation.z) + L")\n";
            msg += L"scene.nodes[0].trainsition: ("
                + std::to_wstring(scene.nodes[3].translation.x) + L", "
                + std::to_wstring(scene.nodes[3].translation.y) + L", "
                + std::to_wstring(scene.nodes[3].translation.z) + L")\n";
            OutputDebugString(msg.c_str());
        }*/
        for (UINT i = 0; i < scene.numMeshPrimitives; i++)
        {
            const UINT vbByteSize = scene.meshes[i].numVertices * sizeof(Vertex);

            const UINT ibByteSize = scene.meshes[i].numIndices * sizeof(std::int32_t);

            auto geo = std::make_unique<Mesh>();
            geo->Name = scene.meshes[i].name;

            ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
            CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), scene.meshes[i].primitives[0].vertices.data(), vbByteSize);

            ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
            CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), scene.meshes[i].primitives[0].indices.data(), ibByteSize);

            geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
                mCommandList, scene.meshes[i].primitives[0].vertices.data(), vbByteSize, geo->VertexBufferUploader);

            geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
                mCommandList, scene.meshes[i].primitives[0].indices.data(), ibByteSize, geo->IndexBufferUploader);

            geo->VertexByteStride = sizeof(Vertex);
            geo->VertexBufferByteSize = vbByteSize;
            geo->IndexFormat = DXGI_FORMAT_R32_UINT;
            geo->IndexBufferByteSize = ibByteSize;

            Submesh submesh;
            submesh.IndexCount = (UINT)scene.meshes[i].numIndices;
            submesh.StartIndexLocation = 0;
            submesh.BaseVertexLocation = 0;
            submesh.Bounds = scene.boundingBox;

            geo->DrawArgs[scene.meshes[i].name] = submesh;

            mGeometries[geo->Name] = std::move(geo);
        }
    }
    {
        std::wstring msg = L"NumTextures: " + std::to_wstring(scene.textures.size());
        auto texMap = std::make_unique<Texture>();
        texMap->Name = "TestMap0";
        texMap->Filename = L"TestMap0Path";
        Scenes::CreateAndUploadTexture(md3dDevice.Get(), mCommandList, texMap->Resource, texMap->UploadHeap, scene.textures[0]);

        mTextures[texMap->Name] = std::move(texMap);
        // msg += L"\n";
        // OutputDebugString(msg.c_str());
    }
    return S_OK;
}

void LampGeo::LoadScene(ID3D12GraphicsCommandList* mCommandList)
{
    LoadTextures(mCommandList);
    LoadOBJ(mCommandList, "Models/OBJ/sibenik/sibenik.obj");
    // mLoadOBJ(mCommandList);
    BuildShapeGeometry(mCommandList);
    BuildSkullGeometry(mCommandList);
    LoadGLTF(mCommandList);
    BuildMaterials();
    BuildRenderItems();
}