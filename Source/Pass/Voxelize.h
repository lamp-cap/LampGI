#pragma once

#include "SceneRenderPass.h"

class Voxel final : public SceneRenderPass
{
public:
	Voxel(ComPtr<ID3D12Device> device,
		std::shared_ptr<DescriptorHeap> heaps,
		std::shared_ptr<LampPSO> PSOs,
		std::shared_ptr<LampGeo> Scene);

	Voxel(const Voxel& rhs) = delete;
	Voxel& operator=(const Voxel& rhs) = delete;
	~Voxel() = default;

	const UINT SRVNum = 7;

	void Draw(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame)override;

	BOOL OnResize(UINT newWidth, UINT newHeight, UINT newDepth = 1)override;

protected:
	void BuildRootSignatureAndPSO()override;
	void BuildResources()override;
	void BuildDescriptors()override;

private:

	const std::wstring mTempTarget = L"VoxelizeTarget";
	const std::wstring mVoxelColor = L"VoxelizedColor";
	const std::wstring mVoxelNormal = L"VoxelizedNormal";
	const std::wstring mVoxelMat = L"VoxelizedMat";

};

