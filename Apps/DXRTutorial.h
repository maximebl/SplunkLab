#pragma once
#include "../Headers/Gpu.h"

namespace DXRTutorial
{
    static const wchar_t* RayGenShaderEntry = L"RayGenerationMain";
    static const wchar_t* MissShaderEntry = L"MissMain";
    static const wchar_t* ClosestHitShaderEntry = L"ClosestHitMain";
    static const wchar_t* ClosestHitPlaneShaderEntry = L"ClosestHitMainPlane";
    static const wchar_t* HitGroup = L"HitGroup";
    static const wchar_t* HitGroupPlane = L"HitGroupPlane";
    static const wchar_t* HitGroupShadow = L"HitGroupShadow";
    static const wchar_t* ShadowClosestHitEntry = L"ShadowClosestHitMain";
    static const wchar_t* ShadowMissEntry = L"ShadowMissMain";
    
    struct Vertex
    {
        DirectX::XMFLOAT3 Position;
    };

    struct PerFrameData
    {
        DirectX::XMFLOAT4 A[3];
        DirectX::XMFLOAT4 B[3];
        DirectX::XMFLOAT4 C[3];
    };

    struct PerInstanceData
    {
        DirectX::XMFLOAT4 A;
        DirectX::XMFLOAT4 B;
        DirectX::XMFLOAT4 C;
    };

    struct TutorialData
    {
        BottomLevelASInfo BottomLevelInfos[2];
        float Rotation = 0;
        
        ComPtr<ID3D12Resource> TriangleVertexBuffer;
        ComPtr<ID3D12Resource> PlaneVertexBuffer;
        ComPtr<ID3D12Resource> TriangleBottomLevelAS;
        ComPtr<ID3D12Resource> PlaneBottomLevelAS;
        ComPtr<ID3D12Resource> TopLevelAS;
        ComPtr<ID3D12Resource> InstanceDescs;

        ComPtr<ID3D12StateObject> RaytracingStateObject;

        // Temp buffers.
        ComPtr<ID3D12Resource> TriangleBottomLevelASScratch;
        ComPtr<ID3D12Resource> PlaneBottomLevelASScratch;
        ComPtr<ID3D12Resource> TopLevelASScratch;

        // Shaders and bindings.
        Shader RtShader;
        ComPtr<ID3D12DescriptorHeap> CSUHeap;
        ComPtr<ID3D12RootSignature> EmptyGlobalRootsig;
        ComPtr<ID3D12RootSignature> RaygenShadersLocalRootsig;
        ComPtr<ID3D12RootSignature> MissEmptyLocalRootsig;
        ComPtr<ID3D12RootSignature> ClosestHitLocalRootsig;
        UINT64 ShaderTableEntrySize;
        ComPtr<ID3D12Resource> ShaderTable;
        ComPtr<ID3D12Resource> PerFrameUploadResource;
        ComPtr<ID3D12Resource> PerInstanceUploadResource;
    };

    void UpdateAndRender(TutorialData& DXRData, ID3D12GraphicsCommandList7* CmdList, UINT Width, UINT Height);
    void InitializeAccelerationStructures(ID3D12Device10* Device, ID3D12GraphicsCommandList7* CmdList, TutorialData* DXRData);
    void InitializeShaderBindings(ID3D12Device10* Device, TutorialData* DXRData, ID3D12Resource* OutputTexture);
    void CreateBottomLevel(ID3D12Device10* Device, ID3D12GraphicsCommandList7* CmdList, ID3D12Resource* VB, UINT VertexCount,
                           ID3D12Resource** BottomLevelASScratch, ID3D12Resource** BottomLevelAS);
    void CreateRootSignatures(ID3D12Device10* Device,
                              ID3D12RootSignature** RaygenShadersLocalRootsig,
                              ID3D12RootSignature** ClosestHitLocalRootsig,
                              ID3D12RootSignature** MissEmptyLocalRootsig,
                              ID3D12RootSignature** EmptyGlobalRootsig);
    void CreateRtStateObject(ID3D12Device10* Device, Shader* Program,
                             ID3D12RootSignature* RaygenShadersLocalRootsig,
                             ID3D12RootSignature* ClosestHitLocalRootsig,
                             ID3D12RootSignature* MissEmptyLocalRootsig,
                             ID3D12RootSignature* EmptyGlobalRootsig,
                             ID3D12StateObject** RaytracingStateObject);
    void CreateShaderTable(ID3D12Device10* Device,
                           ID3D12StateObject* RaytracingStateObject, ID3D12DescriptorHeap* CSUHeap,
                           ID3D12Resource* PerFrameUploadResource, ID3D12Resource* PerInstanceUploadResource,
                           ID3D12Resource** ShaderTable, UINT64* ShaderTableEntrySize);
    void CreateRaygenShaderDescriptors(ID3D12Device10* Device,
                                       ID3D12DescriptorHeap* CSUHeap,
                                       ID3D12Resource* TopLevelAS,
                                       ID3D12Resource* OutputTexture);
    void CreateGPUVisibleCSUHeap(ID3D12Device10* Device, UINT NumDescriptors,
                                 ID3D12DescriptorHeap** CSUHeap);
    void CreateClosestHitShaderUploadResources(ID3D12Device10* Device,
                                               ID3D12Resource** PerFrameUploadResource,
                                               ID3D12Resource** PerInstanceUploadResource);
    void CreateInstanceDescriptions(ID3D12Device10* Device, BottomLevelASInfo* BottomLevelInfos, UINT NumInstances,
                                    ID3D12Resource** InstanceDescs);
    void UpdateInstanceDescriptions(ID3D12Resource* InstanceDescs, BottomLevelASInfo* TriangleBottomLevelInfo,
                                    float Rotation);
}
