#pragma once

#include "../D3D/d3dUtil.h"

class LampShader
{
public:
    LampShader(Microsoft::WRL::ComPtr <ID3D12Device> d3dDevice);
    LampShader(const LampShader& rhs) = delete;
    LampShader& operator=(const LampShader& rhs) = delete;
    ~LampShader() = default;

    LPVOID GetShader(std::string name);
    SIZE_T GetSize(std::string name);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;

    void LoadShaders();
    void CompileShaders();
    void LoadBlobs();
};