// #include "cuda_runtime.h"
#include "Headers/Gpu.h"
#include "Apps/DXRTutorial.h"
#include "Apps/HelloBindless.h"
#include "Apps/MSExperiments.h"
#include "Apps/MSHelloTriangle.h"

using namespace DirectX;

// TODO: New pattern:

 // struct CommittedBuffer
 // {
 //     ID3D12Resource* Resource;
 //     UINT64 Width = 0;
 // };
 
 // struct BufferParams
 // {
 //     ID3D12Device10* Device = nullptr;
 //     D3D12_HEAP_PROPERTIES HeapProps =
 //     {
 //         .Type = D3D12_HEAP_TYPE_DEFAULT,
 //     };
 //     UINT64 Width = 0;
 //     D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
 //     D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
 //     D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_NONE;
 // };
 
 // void CreateCommittedBuffer(BufferParams Params, CommittedBuffer* Buf)
 // {
 //     D3D12_RESOURCE_DESC ResourceDesc = {};
 //     ResourceDesc.Width = Params.Width;
 //     ResourceDesc.Flags = Params.ResourceFlags;
 //
 //     // Parameters that will never need to change:
 //     ResourceDesc.Height = 1;
 //     ResourceDesc.DepthOrArraySize = 1;
 //     ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
 //     ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
 //     ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 //     ResourceDesc.MipLevels = 1;
 //     ResourceDesc.SampleDesc.Count = 1;
 //     ResourceDesc.SampleDesc.Quality = 0;
 //
 //     Check(Params.Device->CreateCommittedResource(&Params.HeapProps,
 //                                                  Params.HeapFlags,
 //                                                  &ResourceDesc,
 //                                                  Params.InitialState,
 //                                                  nullptr,
 //                                                  IID_PPV_ARGS(&Buf->Resource)));
 //     Buf->Width = Params.Width;
 // }

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{

    // // TODO: Use this pattern for the next project.
    //  Global Dx;
    //  D3D::CreateDevice(&Dx);
    //  ID3D12Device10* Device = Dx.Device.Get();
    //
    // // Because the parameters are a struct, you can have:
    // //  - everything is wrapped in custom structs, cross-platform code MUST to do this in some form.
    // //  - better default parameters (the order of the parameter in the function signature that you are trying to specialize is not important).
    // //  - return a pointer to the result (out param).
    // //  - can pass a different Device or CommandList, yet not have to specify it everytime.
    //
    // CommittedBuffer Buffer;
    // CreateCommittedBuffer(
    //     {
    //         .Device = Device,
    //         .HeapProps = {.Type = D3D12_HEAP_TYPE_UPLOAD},
    //         .Width = 1000
    //     }, &Buffer);
    // NAME_D3D12_OBJECT(Buffer.Resource);
    
    {
        enum class Demo
        {
            None,
            NvidiaTutorial,
            MSHelloTriangle,
            HelloBindless,
            MSExperiments,
        };
        Demo CurrentDemo = Demo::MSExperiments;
        
        WindowInfo Window;
        Window.WindowName = "SplunkLab";
        Window.X = 100;
        Window.Y = 100;
        Window.Width = 2560;
        Window.Height = 1440;
        CreateWin32Window(&Window, hInstance, nCmdShow);

        Global Dx;
        GlobalResources Data;
        bool IsNvidiaTutorialInitialized = false;
        bool IsMSHelloTriangleInitialized = false;
        bool IsHelloBindlessInitialized = false;
        bool IsMSExperimentsInitialized = false;

        // TODO: Replace with instancing a compiler everytime we call CompileShader() like DxrPathTracer does to allow multithreading.
        ShaderCompiler Compiler;
        D3D::CreateShaderCompiler(&Compiler);

        D3D::CreateDevice(&Dx);
        NAME_D3D12_OBJECT(Dx.Device);
        ID3D12Device14* Device = Dx.Device.Get();

        D3D::CreateCommitted2DTexture(Device, Window.Width, Window.Height,
                                      D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                      D3D12_HEAP_TYPE_DEFAULT,
                                      Data.OutputTexture.GetAddressOf());
        NAME_D3D12_OBJECT(Data.OutputTexture);
        
        D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
        QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        Check(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Dx.GraphicsQueue)));
        NAME_D3D12_OBJECT(Dx.GraphicsQueue);
        
        QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        Check(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Dx.ComputeQueue)));
        NAME_D3D12_OBJECT(Dx.ComputeQueue);
        
        QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        Check(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Dx.CopyQueue)));
        NAME_D3D12_OBJECT(Dx.CopyQueue);
        
        D3D::CreateFences(Device, Dx.Fence.GetAddressOf(), &Dx.FenceEvent, Data.Frames);
        NAME_D3D12_OBJECT(Dx.Fence);
        
        D3D::CreateCommandLists(Device, Data.Frames);

        D3D::CreateSwapchain(Dx.Factory.Get(), Dx.GraphicsQueue.Get(), &Window,
                             Dx.SwapChain.GetAddressOf());
        
        D3D::CreateRTVDescriptorHeap(Device, Data.RTVHeap.GetAddressOf(), &Data.RTVHeapHandleSize);
        NAME_D3D12_OBJECT(Data.RTVHeap);
        
        D3D::CreateBackBufferRTV(Device, Dx.SwapChain.Get(), Data.Frames,
                                 Data.RTVHeap.Get(), Data.RTVHeapHandleSize);

        D3D::CreateDepthBuffers(Device, Data.Frames, &Window);
        
        D3D::CreateDSVDescriptorHeap(Device, Data.DSVHeap.GetAddressOf(), &Data.DSVHeapHandleSize);
        NAME_D3D12_OBJECT(Data.DSVHeap);

        D3D::CreateDepthBufferDSV(Device, Data.Frames, Data.DSVHeap.Get(), Data.DSVHeapHandleSize);
        
        UINT BackBufferIndex = 0;

        LONG_PTR WndprocData[] = {(LONG_PTR)&Window};
        SetWindowLongPtr(Window.Hwnd, GWLP_USERDATA, (LONG_PTR)WndprocData);
        MSG WindowMessage = {nullptr};

        // TODO: malloc and move pointer down the switch statement.
        DXRTutorial::TutorialData DXRData = {}; 
        HelloBindless::HelloBindlessData QCSData = {};
        MSHelloTriangle::MSHelloTriangleData SLData = {};
        MSExperiments::MSExperimentsData MSEData = {};
        
        do {
            if (WindowMessage.message == WM_KEYDOWN && CurrentDemo == Demo::MSExperiments)
            {
                MSEData.Camera.OnKeyDown(WindowMessage.wParam);
            }
            
            if (WindowMessage.message == WM_KEYDOWN && WindowMessage.wParam == 'N')
            {
                CurrentDemo = Demo::NvidiaTutorial;
            }
            if (WindowMessage.message == WM_KEYDOWN && WindowMessage.wParam == 'M')
            {
                CurrentDemo = Demo::MSHelloTriangle;
            }
            if (WindowMessage.message == WM_KEYDOWN && WindowMessage.wParam == 'B')
            {
                CurrentDemo = Demo::HelloBindless;
            }
            if (WindowMessage.message == WM_KEYDOWN && WindowMessage.wParam == 'E')
            {
                CurrentDemo = Demo::MSExperiments;
            }
            
            Frame* CurrentFrame = &Data.Frames[BackBufferIndex];
            ID3D12GraphicsCommandList7* CmdList = CurrentFrame->GraphicsCmdList.Get();
            Check(CurrentFrame->GraphicsCmdAlloc->Reset());
            Check(CmdList->Reset(CurrentFrame->GraphicsCmdAlloc.Get(), nullptr));

            switch (CurrentDemo)
            {
            case Demo::NvidiaTutorial:
                {
                    if (!IsNvidiaTutorialInitialized)
                    {
                        DXRTutorial::InitializeAccelerationStructures(Device, CmdList, &DXRData);
                
                        // Execute and flush.
                        CmdList->Close();
                        UINT64 FenceValueToWaitFor = CurrentFrame->FenceValue++;
                        Dx.GraphicsQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&CmdList);
                        Check(Dx.GraphicsQueue->Signal(Dx.Fence.Get(), FenceValueToWaitFor));
                        Check(Dx.Fence->SetEventOnCompletion(FenceValueToWaitFor, Dx.FenceEvent));
                        WaitForSingleObject(Dx.FenceEvent, INFINITE);
                
                        Check(CurrentFrame->GraphicsCmdAlloc->Reset());
                        Check(CmdList->Reset(CurrentFrame->GraphicsCmdAlloc.Get(), nullptr));
                
                        DXRData.RtShader.Filename = L"SimpleDXR.hlsl";
                        DXRData.RtShader.TargetProfile = L"lib_6_3";
                        DXRData.RtShader.Entry = L"";
                        D3D::CompileShader(&Compiler, &DXRData.RtShader);
                
                        DXRTutorial::InitializeShaderBindings(Device, &DXRData, Data.OutputTexture.Get());
                        NAME_D3D12_OBJECT(DXRData.ShaderTable);
                
                        IsNvidiaTutorialInitialized = true;
                    }

                    DXRTutorial::UpdateAndRender(DXRData,
                                                 CurrentFrame,
                                                 CmdList,
                                                 Data.OutputTexture.Get(),
                                                 Window.Width, Window.Height);
                }
                break;
            
            case Demo::MSHelloTriangle:
                {
                    if (!IsMSHelloTriangleInitialized)
                    {
                        // Load the scene.
                        // Scene LoadedScene;
                        // D3D::LoadModel(R"(Models\\cornell_box\\cornell_box.gltf)", &LoadedScene);

                        // Simple mesh + pixel shader.
                        SLData.SimpleMS.Filename = L"SimpleMS.hlsl";
                        SLData.SimpleMS.TargetProfile = L"ms_6_6";
                        SLData.SimpleMS.Entry = L"MSMain";
                        D3D::CompileShader(&Compiler, &SLData.SimpleMS);
                        
                        SLData.SimplePS.Filename = L"SimpleMS.hlsl";
                        SLData.SimplePS.TargetProfile = L"ps_6_6";
                        SLData.SimplePS.Entry = L"PSMain";
                        D3D::CompileShader(&Compiler, &SLData.SimplePS);

                        D3D12_ROOT_SIGNATURE_DESC1 RootSigDesc = {};
                        D3D::CreateRootSignature(Device, &RootSigDesc, &SLData.RootSig);
                        NAME_D3D12_OBJECT(SLData.RootSig);
                        
                        D3D::CreateMeshShaderPSO(Device,
                                                 &SLData.SimpleMS, &SLData.SimplePS,
                                                 SLData.RootSig.Get(),
                                                 &SLData.GreenTrianglePSO);
                        NAME_D3D12_OBJECT(SLData.GreenTrianglePSO);
                    }
                    MSHelloTriangle::UpdateAndRender(SLData, CurrentFrame, CmdList, Window.Width, Window.Height);
                }
                break;
                
            case Demo::HelloBindless:
                {
                    if (!IsHelloBindlessInitialized)
                    {
                        // Create a compute shader.
                        QCSData.BindlessShader.Filename = L"SimpleBindless.hlsl";
                        QCSData.BindlessShader.TargetProfile = L"cs_6_7";
                        QCSData.BindlessShader.Entry = L"Main";
                        D3D::CompileShader(&Compiler, &QCSData.BindlessShader);

                        // Create the compute synchronization objects specific to this app.
                        QCSData.FenceEvent = CreateEventEx(nullptr, "QCSData->FenceEvent", FALSE, EVENT_ALL_ACCESS);
                        Check(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&QCSData.Fence)));
                        NAME_D3D12_OBJECT(QCSData.Fence);

                        // Bindless setup.
                        D3D12_ROOT_SIGNATURE_DESC1 RootSigDesc = {};
                        RootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
                        D3D::CreateRootSignature(Device, &RootSigDesc, &QCSData.RootSig);
                        NAME_D3D12_OBJECT(QCSData.RootSig);

                        D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
                        PSODesc.CS.pShaderBytecode = QCSData.BindlessShader.Blob->GetBufferPointer();
                        PSODesc.CS.BytecodeLength = QCSData.BindlessShader.Blob->GetBufferSize();
                        PSODesc.pRootSignature = QCSData.RootSig.Get();
                        Check(Device->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&QCSData.PSO)));
                        NAME_D3D12_OBJECT(QCSData.PSO);
                        
                        D3D12_DESCRIPTOR_HEAP_DESC CSUHeapDesc = {};
                        CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                        CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                        CSUHeapDesc.NumDescriptors = 1;
                        Device->CreateDescriptorHeap(&CSUHeapDesc, IID_PPV_ARGS(&QCSData.CSUHeap));
                        NAME_D3D12_OBJECT(QCSData.CSUHeap);

                        D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
                        UAVDesc.Format = GlobalResources::BackBufferFormat;
                        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                        Device->CreateUnorderedAccessView(Data.OutputTexture.Get(), nullptr, &UAVDesc, QCSData.CSUHeap->GetCPUDescriptorHandleForHeapStart());

                        IsHelloBindlessInitialized = true; 
                    }

                    // Wait before resetting.
                    Check(QCSData.Fence->SetEventOnCompletion(QCSData.FenceValue, QCSData.FenceEvent));
                    WaitForSingleObject(QCSData.FenceEvent, INFINITE);

                    // Reset.
                    ID3D12GraphicsCommandList7* CCmdList = CurrentFrame->ComputeCmdList.Get();
                    Check(CurrentFrame->ComputeCmdAlloc->Reset());
                    Check(CCmdList->Reset(CurrentFrame->ComputeCmdAlloc.Get(), nullptr));

                    D3D::Transition(CmdList,
                                    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                    Data.OutputTexture.Get());
                    
                    // Render.
                    HelloBindless::UpdateAndRender(QCSData, CCmdList, Window.Width, Window.Height);
                    
                    Check(CCmdList->Close());
                    Dx.ComputeQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)(&CCmdList));
                    Dx.ComputeQueue->Signal(QCSData.Fence.Get(), QCSData.FenceValue + 1);
                    
                    D3D::Transition(CmdList,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE,  
                                    Data.OutputTexture.Get());
                    D3D::Transition(CmdList,
                                    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST,
                                    CurrentFrame->BackBuffer.Get());
                    
                    CmdList->CopyResource(CurrentFrame->BackBuffer.Get(), Data.OutputTexture.Get());
                    
                    D3D::Transition(CmdList,
                                    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT,
                                    CurrentFrame->BackBuffer.Get());
                        
                }
                break;
                
            case Demo::MSExperiments:
                {
                    if (!IsMSExperimentsInitialized)
                    {
                        MSEData.Camera.Init({0.f, 5.f, 35.f});
                        MSEData.Camera.SetMoveSpeed(10.f);
                        
                        // Simple mesh + pixel shader.
                        MSEData.SimpleMS.Filename = L"MSExperiment.hlsl";
                        MSEData.SimpleMS.TargetProfile = L"ms_6_6";
                        MSEData.SimpleMS.Entry = L"MSMain";
                        D3D::CompileShader(&Compiler, &MSEData.SimpleMS);
                        
                        MSEData.SimplePS.Filename = L"MSExperiment.hlsl";
                        MSEData.SimplePS.TargetProfile = L"ps_6_6";
                        MSEData.SimplePS.Entry = L"PSMain";
                        D3D::CompileShader(&Compiler, &MSEData.SimplePS);

                        D3D12_ROOT_SIGNATURE_DESC1 RootSigDesc = {};
                        RootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
                        D3D::CreateRootSignature(Device, &RootSigDesc, &MSEData.RootSig);
                        NAME_D3D12_OBJECT(MSEData.RootSig);
                        
                        D3D::CreateMeshShaderPSO(Device,
                                                 &MSEData.SimpleMS, &MSEData.SimplePS,
                                                 MSEData.RootSig.Get(),
                                                 &MSEData.CubeInstancingPSO);
                        NAME_D3D12_OBJECT(MSEData.CubeInstancingPSO);

                        D3D12_DESCRIPTOR_HEAP_DESC CSUHeapDesc = {};
                        CSUHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                        CSUHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                        CSUHeapDesc.NumDescriptors = 1;
                        Device->CreateDescriptorHeap(&CSUHeapDesc, IID_PPV_ARGS(&MSEData.CSUHeap));
                        NAME_D3D12_OBJECT(MSEData.CSUHeap);

                        // TODO: Confirm that we need unique data per frame.
                        D3D::CreateCommittedBuffer(Device,
                                                   D3D12_HEAP_TYPE_GPU_UPLOAD,
                                                   sizeof(Constants),
                                                   &MSEData.SceneConstantsBuffer);
                        NAME_D3D12_OBJECT(MSEData.SceneConstantsBuffer);
                        Check(MSEData.SceneConstantsBuffer->Map(0, nullptr, (void**)&MSEData.SceneConstants));

                        D3D12_CONSTANT_BUFFER_VIEW_DESC SceneConstantsCbvDesc = {};
                        SceneConstantsCbvDesc.BufferLocation = MSEData.SceneConstantsBuffer->GetGPUVirtualAddress();
                        SceneConstantsCbvDesc.SizeInBytes = sizeof(Constants);
                        Device->CreateConstantBufferView(&SceneConstantsCbvDesc, MSEData.CSUHeap->GetCPUDescriptorHandleForHeapStart());
                    }
                    MSExperiments::UpdateAndRender(MSEData, CurrentFrame, CmdList, Window.Width, Window.Height);
                }
                break;
                
            case Demo::None:
                {
                }
                break;
            }
            
            Check(CmdList->Close());
            Dx.GraphicsQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)(&CmdList));

            HRESULT Hr = Dx.SwapChain->Present(Dx.VSync ? Dx.NumVSyncIntervals : 0,
                                              !Dx.VSync ? DXGI_PRESENT_ALLOW_TEARING : 0);
            if (FAILED(Hr))
            {
                Check(Device->GetDeviceRemovedReason()); 
            }

            UINT64 CurrentFenceValue = CurrentFrame->FenceValue;
            Check(Dx.GraphicsQueue->Signal(Dx.Fence.Get(), CurrentFenceValue));

            // Don't go too fast, make sure the next frame is ready to be rendered
            // by making sure the graphics queue has completed executing and presenting the current frame.
            UINT64 CompletedValue = Dx.Fence->GetCompletedValue();
            if (CompletedValue < CurrentFenceValue)
            {
                // Wait.
                Check(Dx.Fence->SetEventOnCompletion(CurrentFenceValue, Dx.FenceEvent));
                WaitForSingleObject(Dx.FenceEvent, INFINITE);
            }

            BackBufferIndex = Dx.SwapChain->GetCurrentBackBufferIndex();
            UINT64 NextFrameFenceValue = CurrentFenceValue + 1;
            Data.Frames[BackBufferIndex].FenceValue = NextFrameFenceValue;

            WindowMessage = WindowMessageLoop();
        } while (WindowMessage.message != WM_QUIT);

        DestroyWindow(&Window, hInstance);

        for (int i = 0; i < GlobalResources::NumBackBuffers; ++i)
        {
            Data.Frames[i].BackBuffer->Release();
        }
    }
#ifdef _DEBUG
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
                                         DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_SUMMARY));
        }
#endif
}
