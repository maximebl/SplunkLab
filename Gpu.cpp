#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"
#include "Headers/Gpu.h"

namespace D3D
{
    void CreateDevice(Global* Dx)
    {
#ifdef _DEBUG
        Check(D3D12GetDebugInterface(IID_PPV_ARGS(&Dx->Debug)));
        Dx->Debug->EnableDebugLayer();
        Dx->Debug->SetEnableAutoName(true);
#if GPU_VALIDATION
        // GPU-based validation must be enabled before creating the device.
        Dx->Debug->SetEnableGPUBasedValidation(true);
#endif
#endif

        Check(CreateDXGIFactory1(IID_PPV_ARGS(&Dx->Factory)));
        Check(Dx->Factory->EnumAdapterByGpuPreference(0,
                                                      DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                      IID_PPV_ARGS(&Dx->Adapter)));

        IDXGIOutput* TempOutput;
        Check(Dx->Adapter->EnumOutputs(0, &TempOutput));
        Check(TempOutput->QueryInterface(IID_PPV_ARGS(&Dx->Output)));
        Check(D3D12CreateDevice(Dx->Adapter.Get(),
                                D3D_FEATURE_LEVEL_12_2,
                                IID_PPV_ARGS(&Dx->Device)));

        // The info queues must be created after the device has been created with the debug layer active.
#ifdef _DEBUG
        ComPtr<IDXGIInfoQueue> DxgiInfoQueue;
        Check(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DxgiInfoQueue)));
        Check(DxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true));
        Check(DxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true));
        Check(DxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true));

        ComPtr<ID3D12InfoQueue> InfoQueue;
        Check(Dx->Device.As<ID3D12InfoQueue>(&InfoQueue));

        D3D12_MESSAGE_ID DisabledMessages[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(DisabledMessages);
        filter.DenyList.pIDList = DisabledMessages;
        Check(InfoQueue->AddStorageFilterEntries(&filter));
        Check(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));
        Check(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
#endif
    }

    void CreateCommandLists(ID3D12Device10* Device, Frame* Frames)
    {
        for (int i = 0; i < GlobalResources::NumBackBuffers; i++)
        {
            Frame* FrameData = &Frames[i];

            // Graphics.
            ID3D12GraphicsCommandList7** GraphicsCmdList = FrameData->GraphicsCmdList.GetAddressOf();
            ID3D12CommandAllocator** GraphicsAllocator = FrameData->GraphicsCmdAlloc.GetAddressOf();
            Check(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                     IID_PPV_ARGS(GraphicsAllocator)));
            Check((*GraphicsAllocator)->Reset());
            NAME_D3D12_OBJECT_INDEXED(FrameData->GraphicsCmdAlloc, i);
            Check(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                *GraphicsAllocator, nullptr,
                                                IID_PPV_ARGS(GraphicsCmdList)));
            Check((*GraphicsCmdList)->Close());
            NAME_D3D12_OBJECT_INDEXED(FrameData->GraphicsCmdList, i);

            // Compute.
            ID3D12GraphicsCommandList7** ComputeCmdList = FrameData->ComputeCmdList.GetAddressOf();
            ID3D12CommandAllocator** ComputeAllocator = FrameData->ComputeCmdAlloc.GetAddressOf();
            Check(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                     IID_PPV_ARGS(ComputeAllocator)));
            Check((*ComputeAllocator)->Reset());
            NAME_D3D12_OBJECT_INDEXED(FrameData->ComputeCmdAlloc, i);
            Check(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                *ComputeAllocator, nullptr,
                                                IID_PPV_ARGS(ComputeCmdList)));
            Check((*ComputeCmdList)->Close());
            NAME_D3D12_OBJECT_INDEXED(FrameData->ComputeCmdList, i);
        }
    }

    void CreateSwapchain(IDXGIFactory7* Factory, ID3D12CommandQueue* GraphicsQueue, WindowInfo* Window,
                         IDXGISwapChain4** SwapChain)
    {
        DXGI_SWAP_CHAIN_DESC Desc = {};
        Desc.BufferCount = GlobalResources::NumBackBuffers;
        Desc.BufferDesc.Width = Window->Width;
        Desc.BufferDesc.Height = Window->Height;
        Desc.BufferDesc.Format = GlobalResources::BackBufferFormat;
        Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        Desc.OutputWindow = Window->Hwnd;
        Desc.SampleDesc.Count = 1;
        Desc.Windowed = true;
        Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH |
            DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING |
            DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        IDXGISwapChain* TempSwapChain = nullptr;
        Check(Factory->CreateSwapChain(GraphicsQueue, &Desc, &TempSwapChain));
        Check(TempSwapChain->QueryInterface(IID_PPV_ARGS(SwapChain)));
        TempSwapChain->Release();

        // Name the backbuffers.
        for (int i = 0; i < GlobalResources::NumBackBuffers; ++i)
        {
            ID3D12Resource* BackBuffer;
            Check((*SwapChain)->GetBuffer(i, IID_PPV_ARGS(&BackBuffer)));
            NAME_D3D12_OBJECT_INDEXED(BackBuffer, i);
        }
    }

    void CreateFences(ID3D12Device10* Device, ID3D12Fence** Fence, HANDLE* FenceEvent, Frame* Frames)
    {
        Check(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence)));
        *FenceEvent = CreateEventEx(nullptr, "Dx->FenceEvent", FALSE, EVENT_ALL_ACCESS);
        if (*FenceEvent == nullptr)
        {
            Check(HRESULT_FROM_WIN32(GetLastError()));
        }

        for (int i = 0; i < GlobalResources::NumBackBuffers; i++)
        {
            Frames[i].FenceValue = i;
        }
    }

    void Transition(ID3D12GraphicsCommandList* CmdList,
                    D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After,
                    ID3D12Resource* Resource, UINT Subresource)
    {
        D3D12_RESOURCE_BARRIER TransitionBarrier = {};
        TransitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        TransitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        TransitionBarrier.Transition.StateBefore = Before;
        TransitionBarrier.Transition.StateAfter = After;
        TransitionBarrier.Transition.pResource = Resource;
        TransitionBarrier.Transition.Subresource = Subresource;
        CmdList->ResourceBarrier(1, &TransitionBarrier);
    }

    void CreateCommittedBuffer(ID3D12Device10* Device, D3D12_HEAP_TYPE HeapType,
                               UINT64 Size, ID3D12Resource** Buffer,
                               D3D12_RESOURCE_STATES InitialState, D3D12_RESOURCE_FLAGS Flags)
    {
        D3D12_HEAP_PROPERTIES HeapDesc = {};
        HeapDesc.Type = HeapType;

        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Height = 1;
        ResourceDesc.Width = Size;
        ResourceDesc.DepthOrArraySize = 1;
        ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ResourceDesc.Flags = Flags;
        ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ResourceDesc.MipLevels = 1;
        ResourceDesc.SampleDesc.Count = 1;
        ResourceDesc.SampleDesc.Quality = 0;

        Check(Device->CreateCommittedResource(&HeapDesc, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
                                              InitialState, nullptr, IID_PPV_ARGS(Buffer)));
    }

    void CreateCommitted2DTexture(ID3D12Device10* Device, UINT64 Width, UINT Height,
                                  D3D12_RESOURCE_FLAGS Flags, D3D12_HEAP_TYPE HeapType,
                                  ID3D12Resource** Texture,
                                  D3D12_RESOURCE_STATES InitialState, DXGI_FORMAT Format, UINT16 MipLevels)
    {
        D3D12_HEAP_PROPERTIES HeapDesc = {};
        HeapDesc.Type = HeapType;

        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Height = Height;
        ResourceDesc.Width = Width;
        ResourceDesc.DepthOrArraySize = 1;
        ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        ResourceDesc.Flags = Flags;
        ResourceDesc.Format = Format;
        ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        ResourceDesc.MipLevels = MipLevels;
        ResourceDesc.SampleDesc.Count = 1;
        ResourceDesc.SampleDesc.Quality = 0;

        Check(Device->CreateCommittedResource(&HeapDesc, D3D12_HEAP_FLAG_NONE, &ResourceDesc, InitialState, nullptr,
                                              IID_PPV_ARGS(Texture)));
    }

    void CreateShaderCompiler(ShaderCompiler* Compiler)
    {
        // Check(Compiler->DllSupport.Initialize());
        // Check(Compiler->DllSupport.CreateInstance(CLSID_DxcCompiler, &Compiler->Compiler));
        // Check(Compiler->DllSupport.CreateInstance(CLSID_DxcUtils, &Compiler->Utils));
        Check(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler->Compiler)));
        Check(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&Compiler->Utils)));
    }

    void CompileShader(ShaderCompiler* Compiler, Shader* Program)
    {
        // Read the file.
        UINT32 CodePage = 0;
        IDxcBlobEncoding* BlobEncoding;
        Check(Compiler->Utils->LoadFile(Program->Filename, &CodePage, &BlobEncoding));

        DxcBuffer SourceBuffer;
        SourceBuffer.Ptr = BlobEncoding->GetBufferPointer();
        SourceBuffer.Size = BlobEncoding->GetBufferSize();

        BOOL IsEncodingKNown;
        Check(BlobEncoding->GetEncoding(&IsEncodingKNown, &CodePage));
        SourceBuffer.Encoding = CodePage;

        // Arguments.
        std::wstring ShaderDirectory = GetSolutionDirectory() + L"Shaders";
        wchar_t ExpandedShaderDirectory[1024] = {};
        GetFullPathNameW(ShaderDirectory.c_str(), _countof(ExpandedShaderDirectory), ExpandedShaderDirectory, nullptr);

        const wchar_t* Arguments[] = {
#if _DEBUG
            L"-Zs",
            L"-Od",
#else
           L"-O3",
#endif
            L"-T", Program->TargetProfile,
            L"-all_resources_bound",
            L"-WX",
            L"-I", ExpandedShaderDirectory // Additional includes.
        };

        IDxcResult* CompilationResult;
        Check(Compiler->Compiler->Compile(
            &SourceBuffer,
            Arguments, _countof(Arguments),
            nullptr,
            IID_PPV_ARGS(&CompilationResult)
        ));

        // Process outputs.
        for (UINT i = 0; i < CompilationResult->GetNumOutputs(); ++i)
        {
            DXC_OUT_KIND Kind = CompilationResult->GetOutputByIndex(i);

            if (Kind == DXC_OUT_ERRORS)
            {
                IDxcBlobUtf8* ErrorBlob;
                Check(CompilationResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&ErrorBlob), nullptr));

                if (ErrorBlob)
                {
                    const char* ErrorString = ErrorBlob->GetStringPointer();
                    if (strlen(ErrorString))
                    {
                        OutputDebugStringA(ErrorString);
                        __debugbreak();
                    }
                }
            }

            if (Kind == DXC_OUT_REMARKS)
            {
                IDxcBlobUtf8* RemarksBlob;
                Check(CompilationResult->GetOutput(DXC_OUT_REMARKS, IID_PPV_ARGS(&RemarksBlob), nullptr));

                if (RemarksBlob)
                {
                    const char* RemarksString = RemarksBlob->GetStringPointer();
                    printf(RemarksString);
                    __debugbreak(); // TODO: Just print remarks, don't break.
                }
            }

            if (Kind == DXC_OUT_OBJECT)
            {
                Check(CompilationResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&Program->Blob), nullptr));
            }
        }
    }

    void CreateRootSignature(ID3D12Device10* Device, D3D12_ROOT_SIGNATURE_DESC1* Desc, ID3D12RootSignature** RootSignature)
    {
        ID3DBlob* RootsigBlob = {};
        ID3DBlob* ErrorBlob = {};
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC VersionedRootsigDesc = {};
        VersionedRootsigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        VersionedRootsigDesc.Desc_1_1 = *Desc;
        // Check(D3D12SerializeVersionedRootSignature(&VersionedRootsigDesc, &RootsigBlob, &ErrorBlob));
        HRESULT Hr = D3D12SerializeVersionedRootSignature(&VersionedRootsigDesc, &RootsigBlob, &ErrorBlob);
        if (FAILED(Hr))
        {
            // const char* errString = error ? reinterpret_cast<const char*>(error->GetBufferPointer()) : "";

            const char* ErrorString = ErrorBlob ? (const char*)ErrorBlob->GetBufferPointer() : "";
            if (strlen(ErrorString))
            {
                OutputDebugStringA(ErrorString);
                __debugbreak();
            }
        }
        Check(Hr);
        Check(Device->CreateRootSignature(0,
                                          RootsigBlob->GetBufferPointer(), RootsigBlob->GetBufferSize(),
                                          IID_PPV_ARGS(RootSignature)));
    }

    void CreateRTVDescriptorHeap(ID3D12Device10* Device, ID3D12DescriptorHeap** RTVHeap, UINT* RTVHeapHandleSize)
    {
        D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
        Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Desc.NumDescriptors = GlobalResources::NumBackBuffers;
        Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        Check(Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(RTVHeap)));
        *RTVHeapHandleSize = Device->GetDescriptorHandleIncrementSize(Desc.Type);
    }

    void CreateBackBufferRTV(ID3D12Device10* Device, IDXGISwapChain4* SwapChain, Frame* Frames, ID3D12DescriptorHeap* RTVHeap, UINT RTVHeapHandleSize)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
        for (int i = 0; i < GlobalResources::NumBackBuffers; i++)
        {
            Frame* CurrentFrame = &Frames[i];
            Check(SwapChain->GetBuffer(i, IID_PPV_ARGS(CurrentFrame->BackBuffer.GetAddressOf())));
            Device->CreateRenderTargetView(CurrentFrame->BackBuffer.Get(), nullptr, RTVHandle);
            CurrentFrame->RTVHandle = RTVHandle;
            RTVHandle.ptr += RTVHeapHandleSize;
        }
    }

    void CreateTopLevel(ID3D12Device10* Device, ID3D12GraphicsCommandList7* CmdList,
                        UINT NumInstances, ID3D12Resource* InstanceDescs,
                        ID3D12Resource** TopLevelASScratch, ID3D12Resource** TopLevelAS )
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
        ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        ASInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        ASInputs.NumDescs = NumInstances;
        ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
        Device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &PrebuildInfo);

        CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_DEFAULT, PrebuildInfo.ScratchDataSizeInBytes + 5000,
                              TopLevelASScratch,
                              D3D12_RESOURCE_STATE_COMMON,
                              D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        CreateCommittedBuffer(Device, D3D12_HEAP_TYPE_DEFAULT, PrebuildInfo.ResultDataMaxSizeInBytes,
                              TopLevelAS,
                              D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                              D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        // Build the TLAS.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC ASDesc = {};
        ASDesc.ScratchAccelerationStructureData = (*TopLevelASScratch)->GetGPUVirtualAddress();
        ASDesc.DestAccelerationStructureData = (*TopLevelAS)->GetGPUVirtualAddress();
        ASDesc.Inputs = ASInputs;
        ASDesc.Inputs.InstanceDescs = InstanceDescs->GetGPUVirtualAddress();
        CmdList->BuildRaytracingAccelerationStructure(&ASDesc, 0, nullptr);

        // Wait for TopLevelAS to be created (all writes are done).
        D3D12_RESOURCE_BARRIER UAVBarrier = {};
        UAVBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        UAVBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        UAVBarrier.UAV.pResource = *TopLevelAS;
        CmdList->ResourceBarrier(1, &UAVBarrier);
    }
    
    void UpdateTopLevel(ID3D12GraphicsCommandList7* CmdList,
                        BottomLevelASInfo* BottomLevelInfos, INT NumBottomLevelInfos,
                        ID3D12Resource* TopLevelASScratch, ID3D12Resource* TopLevelAS,
                        ID3D12Resource* InstanceDescs)
    {
        UINT NumInstances = 0;
        for (INT i = 0; i < NumBottomLevelInfos; ++i)
        {
            NumInstances += BottomLevelInfos[i].NumInstances;
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
        ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        ASInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        ASInputs.NumDescs = NumInstances;
        ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

        // Update the TLAS.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC ASDesc = {};
        ASDesc.ScratchAccelerationStructureData = TopLevelASScratch->GetGPUVirtualAddress();
        ASDesc.DestAccelerationStructureData = TopLevelAS->GetGPUVirtualAddress();
        ASDesc.SourceAccelerationStructureData = TopLevelAS->GetGPUVirtualAddress();
        ASDesc.Inputs = ASInputs;
        ASDesc.Inputs.InstanceDescs = InstanceDescs->GetGPUVirtualAddress();
        CmdList->BuildRaytracingAccelerationStructure(&ASDesc, 0, nullptr);

        // Wait for TopLevelAS to be updated (all writes are done).
        D3D12_RESOURCE_BARRIER UAVBarrier = {};
        UAVBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        UAVBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        UAVBarrier.UAV.pResource = TopLevelAS;
        CmdList->ResourceBarrier(1, &UAVBarrier);
    }

    //// Helper to load UV adjustment for given texture and issue a warning if vertex needs multiple different UV adjustments for its textures 
    //void GetUVAdjustment(const int texIdx, Scene& scene, bool& uvAdjustmentNeeded, DirectX::XMFLOAT2& uvAdjustment)
    //{
    //    if (texIdx != INVALID_ID && scene.textureUVAdjustment.find(texIdx) != scene.textureUVAdjustment.end())
    //    {
    //        DirectX::XMFLOAT2 newUVAdjustment = scene.textureUVAdjustment[texIdx];
    //
    //        // Check if UV adjustment for this texture is same as for previously checked textures
    //        if (uvAdjustmentNeeded && (newUVAdjustment.x != uvAdjustment.x || newUVAdjustment.y != uvAdjustment.y))
    //        {
    //            // Different UV adjustments are required for different textures on same vertex - revert adjustment to identity
    //            printf(
    //                "Different UV adjustments are needed for emissive texture - adjustment won't be applied. Please use textures with sizes which are multiple of 4 to prevent texture distortion.");
    //            newUVAdjustment = DirectX::XMFLOAT2(1.0f, 1.0f);
    //        }
    //        uvAdjustmentNeeded = true;
    //        uvAdjustment = newUVAdjustment;
    //    }
    //}
    
    void ParseMeshes(const tinygltf::Model* GltfModel, Scene* InScene)
    {
    }
    
    void ParseMaterials(const tinygltf::Model* GltfModel, Scene* InScene)
    {
        for (int i = 0; i < GltfModel->materials.size(); ++i)
        {
            tinygltf::Material GltfMaterial = GltfModel->materials[i];
            tinygltf::PbrMetallicRoughness Pbr = GltfMaterial.pbrMetallicRoughness;
            Material Mat;
            
            // Name.
            Mat.Name = GltfMaterial.name;
            
            // Albedo and Opacity
            Mat.Data.BaseColor = {
                (float)Pbr.baseColorFactor[0],
                (float)Pbr.baseColorFactor[1],
                (float)Pbr.baseColorFactor[2] };
            Mat.Data.Opacity = (float)Pbr.baseColorFactor[3];
            Mat.Data.BaseColorTexId = Pbr.baseColorTexture.index;
            
            // Alpha.
            Mat.Data.AlphaCutoff = (float)GltfMaterial.alphaCutoff;
            if (strcmp(GltfMaterial.alphaMode.c_str(), "OPAQUE") == 0) Mat.Data.AlphaMode = ALPHA_MODE_OPAQUE;
            else if (strcmp(GltfMaterial.alphaMode.c_str(), "BLEND") == 0) Mat.Data.AlphaMode = ALPHA_MODE_BLEND;
            else if (strcmp(GltfMaterial.alphaMode.c_str(), "MASK") == 0) Mat.Data.AlphaMode = ALPHA_MODE_MASK;

            // Roughness and Metallic.
            Mat.Data.Roughness = (float)Pbr.roughnessFactor;
            Mat.Data.Metalness = (float)Pbr.metallicFactor;
            Mat.Data.RoughnessMetalnessTexId = Pbr.metallicRoughnessTexture.index;

            // Normals.
            Mat.Data.NormalTexId = GltfMaterial.normalTexture.index;
            
            // Emissive.
            Mat.Data.Emissive = {
                (float)GltfMaterial.emissiveFactor[0],
                (float)GltfMaterial.emissiveFactor[1],
                (float)GltfMaterial.emissiveFactor[2] };
            Mat.Data.EmissiveTexId = GltfMaterial.emissiveTexture.index;

            InScene->Materials.push_back(Mat);
        }
    }
    
    void LoadModel(const char* FileName, Scene* InScene)
    {
        // Load.
        tinygltf::Model GltfModel;
        tinygltf::TinyGLTF GltfLoader;
        std::string Error, Warning;
        if (!GltfLoader.LoadASCIIFromFile(&GltfModel, &Error, &Warning, std::string(FileName)))
        {
            std::string ErrorMessage = std::string(Error.begin(), Error.end());
            MessageBoxW(nullptr, std::wstring(ErrorMessage.begin(), ErrorMessage.end()).c_str(), L"Error", MB_OK);
        }
        else if (Warning.length() > 0)
        {
            std::string msg = std::string(Warning.begin(), Warning.end());
            MessageBoxW(nullptr, std::wstring(msg.begin(), msg.end()).c_str(), L"Warning", MB_OK);
        }

        // Parse.
        ParseMaterials(&GltfModel, InScene);
        ParseMeshes(&GltfModel, InScene);
    }
}
