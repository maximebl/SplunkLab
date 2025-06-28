#include "DXRTutorial.h"

using namespace DirectX;

void DXRTutorial::UpdateAndRender(TutorialData& DXRData,
                                  Frame* CurrentFrame,
                                  ID3D12GraphicsCommandList7* CmdList,
                                  ID3D12Resource* OutTexture,
                                  UINT Width, UINT Height)
{
    D3D::Transition(CmdList,
                    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    OutTexture);
    
    DXRTutorial::UpdateInstanceDescriptions(DXRData.InstanceDescs.Get(),
                                            &DXRData.BottomLevelInfos[0],
                                            DXRData.Rotation);
    DXRData.Rotation += 0.005f;

    D3D::UpdateTopLevel(CmdList,
                        DXRData.BottomLevelInfos, _countof(DXRData.BottomLevelInfos),
                        DXRData.TopLevelASScratch.Get(), DXRData.TopLevelAS.Get(),
                        DXRData.InstanceDescs.Get());

    CmdList->SetDescriptorHeaps(1, DXRData.CSUHeap.GetAddressOf()); // Output texture + TLAS descriptors.
    CmdList->SetPipelineState1(DXRData.RaytracingStateObject.Get());
    CmdList->SetComputeRootSignature(DXRData.EmptyGlobalRootsig.Get());

    // Dispatch.
    D3D12_DISPATCH_RAYS_DESC DispatchRaysDesc = {};
    DispatchRaysDesc.Depth = 1;
    DispatchRaysDesc.Width = Width;
    DispatchRaysDesc.Height = Height;

    // Raygen shader table.
    DispatchRaysDesc.RayGenerationShaderRecord.StartAddress = DXRData.ShaderTable->GetGPUVirtualAddress();
    DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = DXRData.ShaderTableEntrySize;

    // Miss shader table.
    UINT64 NumMissShaders = 2;
    UINT64 MissShaderOffsetInSBT = 1 * DXRData.ShaderTableEntrySize;
    DispatchRaysDesc.MissShaderTable.StartAddress = DXRData.ShaderTable->GetGPUVirtualAddress() + MissShaderOffsetInSBT;
    DispatchRaysDesc.MissShaderTable.StrideInBytes = DXRData.ShaderTableEntrySize;
    DispatchRaysDesc.MissShaderTable.SizeInBytes = NumMissShaders * DXRData.ShaderTableEntrySize;

    // Hit shader table.
    UINT64 NumHitShaders = DXRData.BottomLevelInfos[0].NumInstances * 2 + 1;
    UINT64 HitgroupOffsetInSBT = 3 * DXRData.ShaderTableEntrySize;
    DispatchRaysDesc.HitGroupTable.StartAddress = DXRData.ShaderTable->GetGPUVirtualAddress() + HitgroupOffsetInSBT;
    DispatchRaysDesc.HitGroupTable.StrideInBytes = DXRData.ShaderTableEntrySize;
    DispatchRaysDesc.HitGroupTable.SizeInBytes = NumHitShaders * DXRData.ShaderTableEntrySize;

    CmdList->DispatchRays(&DispatchRaysDesc);

    D3D::Transition(CmdList,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE,
                    OutTexture);
    D3D::Transition(CmdList,
                    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST,
                    CurrentFrame->BackBuffer.Get());

    CmdList->CopyResource(CurrentFrame->BackBuffer.Get(), OutTexture);

    D3D::Transition(CmdList,
                    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT,
                    CurrentFrame->BackBuffer.Get());
}

void DXRTutorial::InitializeAccelerationStructures(ID3D12Device10* Device, ID3D12GraphicsCommandList7* CmdList, TutorialData* DXRData)
{
    // Create the triangles vertex buffer.
    Vertex TriangleVertices[3] = {
        {XMFLOAT3(0, 1, 0)},
        {XMFLOAT3(0.866f, -0.5f, 0)},
        {XMFLOAT3(-0.866f, -0.5f, 0)}
    };
    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_UPLOAD, sizeof(TriangleVertices), &DXRData->TriangleVertexBuffer,
                               D3D12_RESOURCE_STATE_GENERIC_READ);
    NAME_D3D12_OBJECT(DXRData->TriangleVertexBuffer);

    UINT8* MappedVertices = nullptr;
    Check(DXRData->TriangleVertexBuffer->Map(0, nullptr, (void**)&MappedVertices));
    memcpy(MappedVertices, TriangleVertices, sizeof(TriangleVertices));
    DXRData->TriangleVertexBuffer->Unmap(0, nullptr);

    // Create the plane's vertex buffer.
    const Vertex PlaneVertices[6] = {
        XMFLOAT3(-100, -1, -2),
        XMFLOAT3(100, -1, 100),
        XMFLOAT3(-100, -1, 100),

        XMFLOAT3(-100, -1, -2),
        XMFLOAT3(100, -1, -2),
        XMFLOAT3(100, -1, 100)
    };
    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_UPLOAD,
                               sizeof(PlaneVertices), DXRData->PlaneVertexBuffer.GetAddressOf(),
                               D3D12_RESOURCE_STATE_GENERIC_READ);
    NAME_D3D12_OBJECT(DXRData->PlaneVertexBuffer);

    MappedVertices = nullptr;
    Check(DXRData->PlaneVertexBuffer->Map(0, nullptr, (void**)&MappedVertices));
    memcpy(MappedVertices, PlaneVertices, sizeof(PlaneVertices));
    DXRData->PlaneVertexBuffer->Unmap(0, nullptr);

    // Create acceleration structures.
    // Triangle BLAS.
    CreateBottomLevel(Device, CmdList, DXRData->TriangleVertexBuffer.Get(), _countof(TriangleVertices),
                      DXRData->TriangleBottomLevelASScratch.GetAddressOf(),
                      DXRData->TriangleBottomLevelAS.GetAddressOf());
    NAME_D3D12_OBJECT(DXRData->TriangleBottomLevelAS);
    NAME_D3D12_OBJECT(DXRData->TriangleBottomLevelASScratch);

    DXRData->BottomLevelInfos[0].VertexBuffer = DXRData->TriangleVertexBuffer;
    DXRData->BottomLevelInfos[0].BottomLevel = DXRData->TriangleBottomLevelAS;
    DXRData->BottomLevelInfos[0].BottomLevelScratch = DXRData->TriangleBottomLevelASScratch;
    DXRData->BottomLevelInfos[0].NumInstances = 3;

    // Plane BLAS.
    CreateBottomLevel(Device, CmdList, DXRData->PlaneVertexBuffer.Get(), _countof(PlaneVertices),
                                   DXRData->PlaneBottomLevelASScratch.GetAddressOf(),
                                   DXRData->PlaneBottomLevelAS.GetAddressOf());
    NAME_D3D12_OBJECT(DXRData->PlaneBottomLevelAS);
    NAME_D3D12_OBJECT(DXRData->PlaneBottomLevelASScratch);

    DXRData->BottomLevelInfos[1].VertexBuffer = DXRData->PlaneVertexBuffer;
    DXRData->BottomLevelInfos[1].BottomLevel = DXRData->PlaneBottomLevelAS;
    DXRData->BottomLevelInfos[1].BottomLevelScratch = DXRData->PlaneBottomLevelASScratch;
    DXRData->BottomLevelInfos[1].NumInstances = 1;

    // Instance descriptions for the 3 triangles and the plane.
    UINT NumInstances = 0;
    for (int i = 0; i < _countof(DXRData->BottomLevelInfos); ++i)
    {
        NumInstances += DXRData->BottomLevelInfos[i].NumInstances;
    }

    CreateInstanceDescriptions(Device,
                               DXRData->BottomLevelInfos, NumInstances,
                               DXRData->InstanceDescs.GetAddressOf());
    NAME_D3D12_OBJECT(DXRData->InstanceDescs);

    D3D::CreateTopLevel(Device, CmdList, NumInstances, DXRData->InstanceDescs.Get(),
                        DXRData->TopLevelASScratch.GetAddressOf(), DXRData->TopLevelAS.GetAddressOf());
    NAME_D3D12_OBJECT(DXRData->TopLevelASScratch);
    NAME_D3D12_OBJECT(DXRData->TopLevelAS);
}

void DXRTutorial::InitializeShaderBindings(ID3D12Device10* Device, TutorialData* DXRData, ID3D12Resource* OutputTexture)
{
    CreateRootSignatures(Device,
                         DXRData->RaygenShadersLocalRootsig.GetAddressOf(),
                         DXRData->ClosestHitLocalRootsig.GetAddressOf(),
                         DXRData->MissEmptyLocalRootsig.GetAddressOf(),
                         DXRData->EmptyGlobalRootsig.GetAddressOf());

    CreateRtStateObject(Device,
                        &DXRData->RtShader,
                        DXRData->RaygenShadersLocalRootsig.Get(),
                        DXRData->ClosestHitLocalRootsig.Get(),
                        DXRData->MissEmptyLocalRootsig.Get(),
                        DXRData->EmptyGlobalRootsig.Get(),
                        DXRData->RaytracingStateObject.GetAddressOf());

    CreateGPUVisibleCSUHeap(Device, 2, DXRData->CSUHeap.GetAddressOf());

    CreateRaygenShaderDescriptors(Device,
                                  DXRData->CSUHeap.Get(),
                                  DXRData->TopLevelAS.Get(), OutputTexture);

    CreateClosestHitShaderUploadResources(Device,
                                          DXRData->PerFrameUploadResource.GetAddressOf(),
                                          DXRData->PerInstanceUploadResource.GetAddressOf());

    CreateShaderTable(Device, DXRData->RaytracingStateObject.Get(), DXRData->CSUHeap.Get(),
                      DXRData->PerFrameUploadResource.Get(), DXRData->PerInstanceUploadResource.Get(),
                      DXRData->ShaderTable.GetAddressOf(), &DXRData->ShaderTableEntrySize);
}

void DXRTutorial::CreateBottomLevel(ID3D12Device10* Device, ID3D12GraphicsCommandList7* CmdList, ID3D12Resource* VB, UINT VertexCount,
                                    ID3D12Resource** BottomLevelASScratch, ID3D12Resource** BottomLevelAS)
{
    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc = {};
    GeometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    // Prevent AnyHit shaders from executing on opaque geometry.
    GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress = VB->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
    GeometryDesc.Triangles.VertexCount = VertexCount;
    GeometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
    ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    ASInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    ASInputs.NumDescs = 1;
    ASInputs.pGeometryDescs = &GeometryDesc;
    ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    // Create the buffers.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPrebuildInfo = {};
    Device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPrebuildInfo);

    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_DEFAULT, ASPrebuildInfo.ScratchDataSizeInBytes, BottomLevelASScratch,
                               D3D12_RESOURCE_STATE_COMMON,
                               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_DEFAULT, ASPrebuildInfo.ResultDataMaxSizeInBytes, BottomLevelAS,
                               D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC ASDesc = {};
    ASDesc.Inputs = ASInputs;
    ASDesc.DestAccelerationStructureData = (*BottomLevelAS)->GetGPUVirtualAddress();
    ASDesc.ScratchAccelerationStructureData = (*BottomLevelASScratch)->GetGPUVirtualAddress();

    CmdList->BuildRaytracingAccelerationStructure(&ASDesc, 0, nullptr);

    // Wait for TriangleBottomLevelAS to be created (all writes are done).
    D3D12_RESOURCE_BARRIER UAVBarrier = {};
    UAVBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    UAVBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    UAVBarrier.UAV.pResource = *BottomLevelAS;
    CmdList->ResourceBarrier(1, &UAVBarrier);
}

void DXRTutorial::CreateRootSignatures(ID3D12Device10* Device,
                                       ID3D12RootSignature** RaygenShadersLocalRootsig,
                                       ID3D12RootSignature** ClosestHitLocalRootsig,
                                       ID3D12RootSignature** MissEmptyLocalRootsig,
                                       ID3D12RootSignature** EmptyGlobalRootsig)
{
    // Ray generation shader local root signature (SceneAS, Output).
    // One root parameter with a range of two descriptors.

    // Output: PathTracer.hlsl: RWTexture2D<float4> Output : register(u0);
    D3D12_DESCRIPTOR_RANGE1 Ranges[2] = {};
    Ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; // u0.
    Ranges[0].NumDescriptors = 1;
    Ranges[0].OffsetInDescriptorsFromTableStart = 0;

    // SceneAS: PathTracer.hlsl: RaytracingAccelerationStructure SceneAS : register(t0);
    Ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t0.
    Ranges[1].NumDescriptors = 1;
    Ranges[1].OffsetInDescriptorsFromTableStart = 1;

    D3D12_ROOT_PARAMETER1 TableRootParam = {};
    TableRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    TableRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    TableRootParam.DescriptorTable.NumDescriptorRanges = _countof(Ranges);
    TableRootParam.DescriptorTable.pDescriptorRanges = Ranges;
    D3D12_ROOT_SIGNATURE_DESC1 RaygenLocalRootsigDesc = {};
    RaygenLocalRootsigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    RaygenLocalRootsigDesc.NumParameters = 1;
    RaygenLocalRootsigDesc.pParameters = &TableRootParam;
    D3D::CreateRootSignature(Device, &RaygenLocalRootsigDesc, RaygenShadersLocalRootsig);
    NAME_D3D12_OBJECT(*RaygenShadersLocalRootsig);

    // Closest-hit shader local root signature (PerFrame, PerInstance).
    // PerFrame: PathTracer.hlsl: ConstantBuffer<PerFrameData> PerFrame : register(b0);
    // PerFrame: PathTracer.hlsl: ConstantBuffer<PerInstanceData> PerInstance : register(b1);
    D3D12_ROOT_PARAMETER1 CBVRootParams[2] = {};
    for (int i = 0; i < _countof(CBVRootParams); ++i)
    {
        CBVRootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        CBVRootParams[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        CBVRootParams[i].Descriptor.ShaderRegister = i;
    }

    D3D12_ROOT_SIGNATURE_DESC1 ClosestHitLocalRootsigDesc = {};
    ClosestHitLocalRootsigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    ClosestHitLocalRootsigDesc.NumParameters = _countof(CBVRootParams);
    ClosestHitLocalRootsigDesc.pParameters = CBVRootParams;

    D3D::CreateRootSignature(Device, &ClosestHitLocalRootsigDesc, ClosestHitLocalRootsig);
    NAME_D3D12_OBJECT(*ClosestHitLocalRootsig);

    // Empty local root signature for the miss shader (it has no parameters).
    D3D12_ROOT_SIGNATURE_DESC1 EmptyLocalRootsigDesc = {};
    EmptyLocalRootsigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    D3D::CreateRootSignature(Device, &EmptyLocalRootsigDesc, MissEmptyLocalRootsig);
    NAME_D3D12_OBJECT(*MissEmptyLocalRootsig);

    // Empty global root signature.
    D3D12_ROOT_SIGNATURE_DESC1 EmptyGlobalRootsigDesc = {};
    D3D::CreateRootSignature(Device, &EmptyGlobalRootsigDesc, EmptyGlobalRootsig);
    NAME_D3D12_OBJECT(*EmptyGlobalRootsig);
}

void DXRTutorial::CreateRtStateObject(ID3D12Device10* Device, Shader* Program,
                                      ID3D12RootSignature* RaygenShadersLocalRootsig,
                                      ID3D12RootSignature* ClosestHitLocalRootsig,
                                      ID3D12RootSignature* MissEmptyLocalRootsig,
                                      ID3D12RootSignature* EmptyGlobalRootsig,
                                      _Out_ ID3D12StateObject** RaytracingStateObject)
{
    const UINT NumSubObjects = 14;
    UINT SubobjectIndex = 0;
    D3D12_STATE_SUBOBJECT Subobjects[NumSubObjects];

    const wchar_t* EntryPoints[6] = {
        MissShaderEntry,
        ClosestHitShaderEntry,
        ClosestHitPlaneShaderEntry,
        RayGenShaderEntry,
        ShadowClosestHitEntry,
        ShadowMissEntry
    };
    D3D12_EXPORT_DESC ExportDescs[_countof(EntryPoints)] = {};
    for (int i = 0; i < _countof(EntryPoints); ++i)
    {
        ExportDescs[i].Name = EntryPoints[i];
        ExportDescs[i].ExportToRename = nullptr;
    }
    D3D12_DXIL_LIBRARY_DESC DxilLibraryDesc = {};
    DxilLibraryDesc.NumExports = _countof(ExportDescs);
    DxilLibraryDesc.pExports = ExportDescs;
    DxilLibraryDesc.DXILLibrary.pShaderBytecode = Program->Blob->GetBufferPointer();
    DxilLibraryDesc.DXILLibrary.BytecodeLength = Program->Blob->GetBufferSize();
    D3D12_STATE_SUBOBJECT LibraryStateObject = {};
    LibraryStateObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    LibraryStateObject.pDesc = &DxilLibraryDesc;
    Subobjects[SubobjectIndex++] = LibraryStateObject;

    // Hit group: closest-hit.
    D3D12_HIT_GROUP_DESC HitGroupDesc = {};
    HitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    HitGroupDesc.HitGroupExport = HitGroup;
    HitGroupDesc.ClosestHitShaderImport = ClosestHitShaderEntry;
    // HitGroupDesc.IntersectionShaderImport =;
    // HitGroupDesc.AnyHitShaderImport = ;
    D3D12_STATE_SUBOBJECT HitGroupSubobject = {};
    HitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    HitGroupSubobject.pDesc = &HitGroupDesc;
    Subobjects[SubobjectIndex++] = HitGroupSubobject;
    
    // Hit group: Plane closest-hit.
    D3D12_HIT_GROUP_DESC HitGroupPlaneDesc = {};
    HitGroupPlaneDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    HitGroupPlaneDesc.HitGroupExport = HitGroupPlane;
    HitGroupPlaneDesc.ClosestHitShaderImport = ClosestHitPlaneShaderEntry;
    // HitGroupPlaneDesc.IntersectionShaderImport =;
    // HitGroupPlaneDesc.AnyHitShaderImport = ;
    D3D12_STATE_SUBOBJECT HitGroupPlaneSubobject = {};
    HitGroupPlaneSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    HitGroupPlaneSubobject.pDesc = &HitGroupPlaneDesc;
    Subobjects[SubobjectIndex++] = HitGroupPlaneSubobject;
    
    // Hit group: Shadow closest-hit.
    D3D12_HIT_GROUP_DESC HitGroupShadowDesc = {};
    HitGroupShadowDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    HitGroupShadowDesc.HitGroupExport = HitGroupShadow;
    HitGroupShadowDesc.ClosestHitShaderImport = ShadowClosestHitEntry;
    // HitGroupShadowDesc.IntersectionShaderImport =;
    // HitGroupShadowDesc.AnyHitShaderImport = ;
    D3D12_STATE_SUBOBJECT HitGroupShadowSubobject = {};
    HitGroupShadowSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    HitGroupShadowSubobject.pDesc = &HitGroupShadowDesc;
    Subobjects[SubobjectIndex++] = HitGroupShadowSubobject;

    D3D12_STATE_SUBOBJECT LocalRootsigSubobject = {};
    LocalRootsigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    LocalRootsigSubobject.pDesc = &RaygenShadersLocalRootsig;
    Subobjects[SubobjectIndex] = LocalRootsigSubobject;
    // Associate the PathTracer.hlsl local root signature to the ray generation shader.
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION RayGenExportsAssociation = {};
    const wchar_t* RaygenShaderExports[2] = {RayGenShaderEntry, ClosestHitPlaneShaderEntry};
    RayGenExportsAssociation.pExports = RaygenShaderExports;
    RayGenExportsAssociation.NumExports = _countof(RaygenShaderExports);
    RayGenExportsAssociation.pSubobjectToAssociate = &Subobjects[SubobjectIndex++];

    // Ray generation local rootsig subobject.
    D3D12_STATE_SUBOBJECT ExportsAssociationSubobject = {};
    ExportsAssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    ExportsAssociationSubobject.pDesc = &RayGenExportsAssociation;
    Subobjects[SubobjectIndex++] = ExportsAssociationSubobject;

    // Local root signature for the hit shader.
    D3D12_STATE_SUBOBJECT ClosestHitLocalRootsigSubobject = {};
    ClosestHitLocalRootsigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    ClosestHitLocalRootsigSubobject.pDesc = &ClosestHitLocalRootsig;
    Subobjects[SubobjectIndex] = ClosestHitLocalRootsigSubobject;
    // Associate the hit shader local root signature to the hit shader.
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ClosestHitExportsAssociation = {};
    const wchar_t* HitShaderExports[1] = {ClosestHitShaderEntry};
    ClosestHitExportsAssociation.pExports = HitShaderExports;
    ClosestHitExportsAssociation.NumExports = _countof(HitShaderExports);
    ClosestHitExportsAssociation.pSubobjectToAssociate = &Subobjects[SubobjectIndex++];
    D3D12_STATE_SUBOBJECT ClosestHitExportsAssociationSubobject = {};
    ClosestHitExportsAssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    ClosestHitExportsAssociationSubobject.pDesc = &ClosestHitExportsAssociation;
    Subobjects[SubobjectIndex++] = ClosestHitExportsAssociationSubobject;

    // Empty local root signature for shaders that don't bind any descriptors.
    D3D12_STATE_SUBOBJECT EmptyRootsigSubobject = {};
    EmptyRootsigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    EmptyRootsigSubobject.pDesc = &MissEmptyLocalRootsig;
    Subobjects[SubobjectIndex] = EmptyRootsigSubobject;
    // Associate the empty local root signature to the miss shader.
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION MissExportsAssociation = {};
    const wchar_t* EmptyShaderExports[2] = {
        ShadowClosestHitEntry,
        ShadowMissEntry
    };
    MissExportsAssociation.pExports = EmptyShaderExports;
    MissExportsAssociation.NumExports = _countof(EmptyShaderExports);
    MissExportsAssociation.pSubobjectToAssociate = &Subobjects[SubobjectIndex++];
    D3D12_STATE_SUBOBJECT MissExportsAssociationSubobject = {};
    MissExportsAssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    MissExportsAssociationSubobject.pDesc = &MissExportsAssociation;
    Subobjects[SubobjectIndex++] = MissExportsAssociationSubobject;

    // Attributes and Payload struct shader config.
    D3D12_RAYTRACING_SHADER_CONFIG RtShaderConfig = {};
    RtShaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2; // BuiltInTriangleIntersectionAttributes.
    RtShaderConfig.MaxPayloadSizeInBytes = sizeof(float) * 3; // Size of our payload struct (12 bytes, float3 color).
    D3D12_STATE_SUBOBJECT ShaderConfigSubobject = {};
    ShaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    ShaderConfigSubobject.pDesc = &RtShaderConfig;
    Subobjects[SubobjectIndex] = ShaderConfigSubobject;

    // Associate the shader config to ray tracing shaders.
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION RtShaderConfigAssociation = {};
    RtShaderConfigAssociation.pExports = EntryPoints;
    RtShaderConfigAssociation.NumExports = _countof(EntryPoints);
    RtShaderConfigAssociation.pSubobjectToAssociate = &Subobjects[SubobjectIndex++];
    D3D12_STATE_SUBOBJECT ShaderConfigAssociationSubobject = {};
    ShaderConfigAssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    ShaderConfigAssociationSubobject.pDesc = &RtShaderConfigAssociation;
    Subobjects[SubobjectIndex++] = ShaderConfigAssociationSubobject;

    // Ray tracing pipeline config.
    D3D12_RAYTRACING_PIPELINE_CONFIG RtPipelineConfig = {};
    RtPipelineConfig.MaxTraceRecursionDepth = 2;
    D3D12_STATE_SUBOBJECT RtPipelineConfigSubobject = {};
    RtPipelineConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    RtPipelineConfigSubobject.pDesc = &RtPipelineConfig;
    Subobjects[SubobjectIndex++] = RtPipelineConfigSubobject;

    // Empty global root signature.
    D3D12_GLOBAL_ROOT_SIGNATURE GlobalRootsig = {};
    GlobalRootsig.pGlobalRootSignature = EmptyGlobalRootsig;
    D3D12_STATE_SUBOBJECT GlobalRootsigSubobject = {};
    GlobalRootsigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    GlobalRootsigSubobject.pDesc = &GlobalRootsig;
    Subobjects[SubobjectIndex++] = GlobalRootsigSubobject;

    // Combine the subobjects into the final ray tracing state object.
    D3D12_STATE_OBJECT_DESC StateObjectDesc = {};
    StateObjectDesc.NumSubobjects = NumSubObjects;
    StateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    StateObjectDesc.pSubobjects = Subobjects;
    Check(Device->CreateStateObject(&StateObjectDesc, IID_PPV_ARGS(RaytracingStateObject)));
}

void DXRTutorial::CreateShaderTable(ID3D12Device10* Device,
                                    ID3D12StateObject* RaytracingStateObject, ID3D12DescriptorHeap* CSUHeap,
                                    ID3D12Resource* PerFrameUploadResource, ID3D12Resource* PerInstanceUploadResource,
                                    ID3D12Resource** ShaderTable, UINT64* ShaderTableEntrySize)
{
    // To figure out the the size of an entry in the SBT, figured out the largest possible entry size.
    // In our case, the hit group entry is likely to be the biggest entry.

    // It contains a shader ID:
    UINT64 TableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    // And 2 CBVs:
    TableEntrySize += 8; // Add the size of the first CBV.
    TableEntrySize += 8; // Add the size of the second CBV.

    TableEntrySize = AlignTo(TableEntrySize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    *ShaderTableEntrySize = TableEntrySize;

    const UINT NumPrograms = 10;
    UINT64 ShaderTableSize = TableEntrySize * NumPrograms; // One entry per program.

    // We are using an upload heap for simplicity, ideally transfer to a default heap.
    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_UPLOAD,
                               ShaderTableSize, ShaderTable,
                               D3D12_RESOURCE_STATE_GENERIC_READ);
    
    // Get the shader IDs.
    ComPtr<ID3D12StateObjectProperties> RtPsoProperties;
    RaytracingStateObject->QueryInterface(IID_PPV_ARGS(&RtPsoProperties));
    const void* RaygenShaderId = RtPsoProperties->GetShaderIdentifier(RayGenShaderEntry);
    const void* MissShaderId = RtPsoProperties->GetShaderIdentifier(MissShaderEntry);
    const void* ShadowMissShaderId = RtPsoProperties->GetShaderIdentifier(ShadowMissEntry);
    const void* HitGroupId = RtPsoProperties->GetShaderIdentifier(HitGroup);
    const void* HitGroupPlaneId = RtPsoProperties->GetShaderIdentifier(HitGroupPlane);
    const void* HitGroupShadowId = RtPsoProperties->GetShaderIdentifier(HitGroupShadow);

    UINT8* ShaderTableDataStart = nullptr;
    Check((*ShaderTable)->Map(0, nullptr, (void**)&ShaderTableDataStart));

    // Entry 0: Ray generation shader ID.
    memcpy(ShaderTableDataStart, RaygenShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Entry 0 data: output texture descriptor handle. Followed by the descriptor for the TLAS.
    UINT8* OutputTextureHandleOffset = ShaderTableDataStart + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    *(D3D12_GPU_VIRTUAL_ADDRESS*)OutputTextureHandleOffset = CSUHeap->GetGPUDescriptorHandleForHeapStart().ptr;

    // Entry 1: Miss shader ID.
    UINT8* MissEntry = ShaderTableDataStart + TableEntrySize;
    memcpy(MissEntry, MissShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    
    // Entry 2: Shadow Miss shader ID.
    UINT8* ShadowMissEntry = MissEntry + TableEntrySize;
    memcpy(ShadowMissEntry, ShadowMissShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 3: Instance 0 Closest-hit shader ID.
    UINT8* Instance0HitGroupEntry = ShadowMissEntry + TableEntrySize;
    memcpy(Instance0HitGroupEntry, HitGroupId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Entry 3 data: PerFrame root CBV.
    UINT8* Instance0PerFrameCBVOffset = Instance0HitGroupEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    *(D3D12_GPU_VIRTUAL_ADDRESS*)Instance0PerFrameCBVOffset = PerFrameUploadResource->GetGPUVirtualAddress();
    // Entry 3 data: PerInstance root CBV.
    UINT8* Instance0PerInstanceCBVOffset = Instance0PerFrameCBVOffset + sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    *(D3D12_GPU_VIRTUAL_ADDRESS*)Instance0PerInstanceCBVOffset = PerInstanceUploadResource->GetGPUVirtualAddress();

    // Entry 4: Instance 0 Shadow Closest-hit shader ID.
    UINT8* Instance0ShadowHitGroupEntry = Instance0HitGroupEntry + TableEntrySize;
    memcpy(Instance0ShadowHitGroupEntry, HitGroupShadowId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 5: Instance 1 Closest-hit shader ID.
    UINT8* Instance1HitGroupEntry = Instance0ShadowHitGroupEntry + TableEntrySize;
    memcpy(Instance1HitGroupEntry, HitGroupId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Entry 5 data: PerFrame root CBV.
    UINT8* Instance1PerFrameCBVOffset = Instance1HitGroupEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    *(D3D12_GPU_VIRTUAL_ADDRESS*)Instance1PerFrameCBVOffset = PerFrameUploadResource->GetGPUVirtualAddress();
    // Entry 5 data: PerInstance root CBV.
    UINT8* Instance1PerInstanceCBVOffset = Instance1PerFrameCBVOffset + sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    *(D3D12_GPU_VIRTUAL_ADDRESS*)Instance1PerInstanceCBVOffset = PerInstanceUploadResource->GetGPUVirtualAddress() + sizeof(PerInstanceData);

    // Entry 6: Instance 1 Shadow Closest-hit shader ID.
    UINT8* Instance1ShadowHitGroupEntry = Instance1HitGroupEntry + TableEntrySize;
    memcpy(Instance1ShadowHitGroupEntry, HitGroupShadowId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 7: Instance 2 Closest-hit shader ID.
    UINT8* Instance2HitGroupEntry = Instance1ShadowHitGroupEntry + TableEntrySize;
    memcpy(Instance2HitGroupEntry, HitGroupId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Entry 7 data: PerFrame root CBV.
    UINT8* Instance2PerFrameCBVOffset = Instance2HitGroupEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    *(D3D12_GPU_VIRTUAL_ADDRESS*)Instance2PerFrameCBVOffset = PerFrameUploadResource->GetGPUVirtualAddress();
    // Entry 7 data: PerInstance root CBV.
    UINT8* Instance2PerInstanceCBVOffset = Instance2PerFrameCBVOffset + sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    *(D3D12_GPU_VIRTUAL_ADDRESS*)Instance2PerInstanceCBVOffset = PerInstanceUploadResource->GetGPUVirtualAddress() + sizeof(PerInstanceData) * 2;

    // Entry 8 Instance 2 Shadow Closest-hit shader ID.
    UINT8* Instance2ShadowHitGroupEntry = Instance2HitGroupEntry + TableEntrySize;
    memcpy(Instance2ShadowHitGroupEntry, HitGroupShadowId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    
    // Entry 9: Plane hit shader ID.
    UINT8* PlaneHitGroupEntry = Instance2ShadowHitGroupEntry + TableEntrySize;
    memcpy(PlaneHitGroupEntry, HitGroupPlaneId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    (*ShaderTable)->Unmap(0, nullptr);
}

void DXRTutorial::CreateRaygenShaderDescriptors(ID3D12Device10* Device,
                                                ID3D12DescriptorHeap* CSUHeap,
                                                ID3D12Resource* TopLevelAS,
                                                ID3D12Resource* OutputTexture)
{
    // Output texture (Resource + UAV).
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = CSUHeap->GetCPUDescriptorHandleForHeapStart();
    Device->CreateUnorderedAccessView(OutputTexture, nullptr, &UAVDesc, CPUHandle);

    // TLAS (SRV).
    D3D12_SHADER_RESOURCE_VIEW_DESC TlasSrvDesc = {};
    TlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    TlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    TlasSrvDesc.RaytracingAccelerationStructure.Location = TopLevelAS->GetGPUVirtualAddress();

    CPUHandle.ptr += Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    Device->CreateShaderResourceView(nullptr, &TlasSrvDesc, CPUHandle);
}

void DXRTutorial::CreateGPUVisibleCSUHeap(ID3D12Device10* Device,UINT NumDescriptors, ID3D12DescriptorHeap** CSUHeap)
{
    // Contains the descriptors for:
    // 0: The output texture (UAV, RWTexture2D).
    // 1: The scene representation TLAS (SRV, RaytracingAccelerationStructure).
    D3D12_DESCRIPTOR_HEAP_DESC CSUHeapDesc = {};
    CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CSUHeapDesc.NumDescriptors = NumDescriptors;
    Check(Device->CreateDescriptorHeap(&CSUHeapDesc, IID_PPV_ARGS(CSUHeap)));
}

void DXRTutorial::CreateClosestHitShaderUploadResources(ID3D12Device10* Device,
                                                        ID3D12Resource** PerFrameUploadResource,
                                                        ID3D12Resource** PerInstanceUploadResource)
{
    // Create the per-frame constant buffer.
    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_UPLOAD, sizeof(PerFrameData),
                               PerFrameUploadResource);
    NAME_D3D12_OBJECT(*PerFrameUploadResource);

    PerFrameData PerFrame = {};
    PerFrame.A[0] = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    PerFrame.A[1] = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    PerFrame.A[2] = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

    PerFrame.B[0] = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
    PerFrame.B[1] = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
    PerFrame.B[2] = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);

    PerFrame.C[0] = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);
    PerFrame.C[1] = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
    PerFrame.C[2] = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);

    UINT* MappedPerFrameData = nullptr;
    Check((*PerFrameUploadResource)->Map(0, nullptr, (void**)&MappedPerFrameData));
    memcpy(MappedPerFrameData, &PerFrame, sizeof(PerFrameData));
    (*PerFrameUploadResource)->Unmap(0, nullptr);

    // Create the per-instance constant buffer.
    size_t PerInstanceDataSize = 3 * sizeof(PerInstanceData);
    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_UPLOAD, PerInstanceDataSize,
                               PerInstanceUploadResource);
    NAME_D3D12_OBJECT(*PerInstanceUploadResource);

    PerInstanceData PerInstance[3] = {};
    PerInstance[0].A = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    PerInstance[0].B = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    PerInstance[0].C = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

    PerInstance[1].A = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
    PerInstance[1].B = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
    PerInstance[1].C = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

    PerInstance[2].A = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
    PerInstance[2].B = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
    PerInstance[2].C = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);

    UINT* MappedPerInstanceData = nullptr;
    Check((*PerInstanceUploadResource)->Map(0, nullptr, (void**)&MappedPerInstanceData));
    memcpy(MappedPerInstanceData, &PerInstance, PerInstanceDataSize);
    (*PerInstanceUploadResource)->Unmap(0, nullptr);
}

void DXRTutorial::CreateInstanceDescriptions(ID3D12Device10* Device, BottomLevelASInfo* BottomLevelInfos, UINT NumInstances,
                                             ID3D12Resource** InstanceDescs)
{
    D3D::CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_UPLOAD,
                               sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * NumInstances,
                               InstanceDescs,
                               D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE);

    D3D12_RAYTRACING_INSTANCE_DESC* InstanceDesc;
    Check((*InstanceDescs)->Map(0, nullptr, (void**)&InstanceDesc));

    // Triangle instance descriptions.
    for (UINT InstanceId = 0; InstanceId < BottomLevelInfos[0].NumInstances; ++InstanceId)
    {
        InstanceDesc[InstanceId].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        // All instances use the same BLAS. They're all triangles with the same vertex buffers.
        InstanceDesc[InstanceId].AccelerationStructure = BottomLevelInfos[0].BottomLevel->GetGPUVirtualAddress();
        InstanceDesc[InstanceId].InstanceID = InstanceId;
        // This value will be exposed to the shader via InstanceID().
        InstanceDesc[InstanceId].InstanceMask = 0xFF; // Include all instances.
        // Same contribution to hit-group index, because we don't have per-instance hit-group data.
        InstanceDesc[InstanceId].InstanceContributionToHitGroupIndex = InstanceId * 2;

        float XOffset = (float)InstanceId;
        XMMATRIX T = XMMatrixTranslation(XOffset * 2.f, 0.f, 0.f);
        XMMATRIX S = XMMatrixScaling(0.2f, 0.5f, 0.5f);
        XMFLOAT3X4 StoredT;
        XMStoreFloat3x4(&StoredT, XMMatrixIdentity() * T * S);
        memcpy(InstanceDesc[InstanceId].Transform, &StoredT, sizeof(StoredT));
    }

    // Plane instance descriptions.
    InstanceDesc[3].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    // All instances use the same BLAS. They're all triangles with the same vertex buffers.
    InstanceDesc[3].AccelerationStructure = BottomLevelInfos[1].BottomLevel->GetGPUVirtualAddress();
    InstanceDesc[3].InstanceID = 0;
    // This value will be exposed to the shader via InstanceID().
    InstanceDesc[3].InstanceMask = 0xFF; // Include all instances.
    // Same contribution to hit-group index, because we don't have per-instance hit-group data.
    InstanceDesc[3].InstanceContributionToHitGroupIndex = BottomLevelInfos[0].NumInstances * 2;
    // 2: There's 2 hit shaders per triangle instance.

    float XOffset = (float)0;
    XMMATRIX T = XMMatrixTranslation(XOffset, 0.f, 0.f);
    XMFLOAT3X4 StoredT;
    XMStoreFloat3x4(&StoredT, XMMatrixIdentity() * T);
    memcpy(InstanceDesc[3].Transform, &StoredT, sizeof(StoredT));

    (*InstanceDescs)->Unmap(0, nullptr);
}

void DXRTutorial::UpdateInstanceDescriptions(ID3D12Resource* InstanceDescs, BottomLevelASInfo* TriangleBottomLevelInfo, float Rotation)
{
    D3D12_RAYTRACING_INSTANCE_DESC* InstanceDesc;
    Check(InstanceDescs->Map(0, nullptr, (void**)&InstanceDesc));

    // Update the triangle instance descriptions.
    for (UINT InstanceId = 0; InstanceId < TriangleBottomLevelInfo->NumInstances; ++InstanceId)
    {
        InstanceDesc[InstanceId].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

        // All instances use the same BLAS. They're all triangles with the same vertex buffers.
        InstanceDesc[InstanceId].AccelerationStructure = TriangleBottomLevelInfo->BottomLevel->GetGPUVirtualAddress();
        InstanceDesc[InstanceId].InstanceID = InstanceId;
        // This value will be exposed to the shader via InstanceID().
        InstanceDesc[InstanceId].InstanceMask = 0xFF; // Include all instances.
        // Same contribution to hit-group index, because we don't have per-instance hit-group data.
        InstanceDesc[InstanceId].InstanceContributionToHitGroupIndex = InstanceId * 2;

        float XOffset = (float)InstanceId;
        XMMATRIX T = XMMatrixTranslation(XOffset * 2.f, 0.f, 0.f);
        XMMATRIX S = XMMatrixScaling(0.2f, 0.5f, 0.5f);
        XMMATRIX R = XMMatrixRotationY((Rotation * (float)(InstanceId)));
        XMFLOAT3X4 StoredT;
        XMStoreFloat3x4(&StoredT, XMMatrixIdentity() * R * T * S);
        memcpy(InstanceDesc[InstanceId].Transform, &StoredT, sizeof(StoredT));
    }
}
