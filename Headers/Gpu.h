#pragma once

#pragma comment(lib, "dxcompiler.lib")

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "DirectXColors.h"
#include "Basics.h"
#include "dxgidebug.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <dxcapi.h>
#include <vector>
#include "../Shaders/Shared.h"

using namespace Microsoft::WRL;

inline void Check(HRESULT HR)
{
    if (FAILED(HR))
    {
#define MSG_LENGTH 512
        char ErrorMessage[MSG_LENGTH];
        memset(ErrorMessage, 0, MSG_LENGTH);
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, HR,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      ErrorMessage, MSG_LENGTH - 1, nullptr);
        OutputDebugStringA(ErrorMessage);
        MessageBox(nullptr, ErrorMessage, "Error", MB_OK);
        __debugbreak();
    }
}

#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* object, const char* name)
{
    if (object != nullptr)
    {
        wchar_t obj_name[MAX_PATH] = {};
        size_t chars_converted = 0;
        mbstowcs_s(&chars_converted,
                   obj_name, MAX_PATH,
                   name, strlen(name));
        object->SetName(obj_name);
    }
}

inline void SetName(ComPtr<ID3D12Object> object, const char* name)
{
    if (object.Get() != nullptr)
    {
        wchar_t obj_name[MAX_PATH] = {};
        size_t chars_converted = 0;
        mbstowcs_s(&chars_converted,
                   obj_name, MAX_PATH,
                   name, strlen(name));
        object->SetName(obj_name);
    }
}

inline void SetNameIndexed(ID3D12Object* object, const wchar_t* name, UINT index)
{
    WCHAR fullname[MAX_PATH];
    if (swprintf_s(fullname, L"%s[%u]", name, index) > 0 && object != nullptr)
    {
        object->SetName(fullname);
    }
}

inline void SetNameIndexed(ComPtr<ID3D12Object> object, const wchar_t* name, UINT index)
{
    WCHAR fullname[MAX_PATH];
    if (swprintf_s(fullname, L"%s[%u]", name, index) > 0 && object.Get() != nullptr)
    {
        object->SetName(fullname);
    }
}

#else
    inline void SetName(ID3D12Object *, const char *)
{
}
    inline void SetNameIndexed(ID3D12Object *, const wchar_t *, UINT)
{
}
    inline void SetName(ComPtr<ID3D12Object>, const char *)
{
}
    inline void SetNameIndexed(ComPtr<ID3D12Object>, const wchar_t *, UINT)
{
}
#endif
#define NAME_D3D12_OBJECT(x) SetName((x), #x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x), L#x, n)

struct Global
{
    // Devices.
    ComPtr<ID3D12Device10> Device;
    ComPtr<IDXGIFactory7> Factory;
    ComPtr<IDXGIAdapter4> Adapter;
    ComPtr<IDXGIOutput6> Output;
    ComPtr<ID3D12Debug5> Debug;

    // Queues.
    ComPtr<ID3D12CommandQueue> GraphicsQueue;
    ComPtr<ID3D12CommandQueue> CopyQueue;
    ComPtr<ID3D12CommandQueue> ComputeQueue;

    // Swap chain.
    ComPtr<IDXGISwapChain4> SwapChain;
    bool VSync = false;
    UINT NumVSyncIntervals = 1;

    // Synchronization.
    ComPtr<ID3D12Fence> Fence;
    HANDLE FenceEvent;
};

struct Frame
{
    ComPtr<ID3D12Resource> BackBuffer;
    ComPtr<ID3D12GraphicsCommandList7> GraphicsCmdList;
    ComPtr<ID3D12CommandAllocator> GraphicsCmdAlloc;
    ComPtr<ID3D12GraphicsCommandList7> ComputeCmdList;
    ComPtr<ID3D12CommandAllocator> ComputeCmdAlloc;
    UINT64 FenceValue;
    D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
};

struct GlobalResources
{
    // Global data.
    static const INT NumBackBuffers = 2;
    static const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    ComPtr<ID3D12DescriptorHeap> RTVHeap;
    UINT RTVHeapHandleSize;
    ComPtr<ID3D12Resource> OutputTexture;

    // Frame data.
    Frame Frames[NumBackBuffers] = {};
};

struct ShaderCompiler
{
    // dxc::DxcDllSupport DllSupport;
    IDxcCompiler3* Compiler;
    IDxcUtils* Utils;
};

struct Shader
{
    IDxcBlob* Blob;
    const wchar_t* Filename;
    const wchar_t* Entry;
    const wchar_t* TargetProfile;
    DxcDefine* Defines;
};

struct BottomLevelASInfo
{
    ComPtr<ID3D12Resource> BottomLevel;
    ComPtr<ID3D12Resource> BottomLevelScratch;
    ComPtr<ID3D12Resource> VertexBuffer;
    UINT NumInstances = 1;
};

struct Material
{
    std::string Name = "";
	MaterialData Data;	
};

struct MeshPrimitive
{
};

struct Mesh
{
    std::string Name = "";
    MeshPrimitive Primitive;
};

struct Scene
{
    UINT NumGeometries = 0;
    std::vector<Material> Materials;
    std::vector<Mesh> Meshes;
};

namespace D3D
{
#define GPU_VALIDATION 1
#define INVALID_ID (-1)
#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_BLEND 1
#define ALPHA_MODE_MASK 2

    void CreateDevice(Global* Dx);
    void CreateCommandLists(ID3D12Device10* Device, Frame* Frames);
    void CreateSwapchain(IDXGIFactory7* Factory, ID3D12CommandQueue* GraphicsQueue, WindowInfo* Window,
                         IDXGISwapChain4** SwapChain);
    void CreateFences(ID3D12Device10* Device, ID3D12Fence** Fence, HANDLE* FenceEvent, Frame* Frames);
    void CreateRTVDescriptorHeap(ID3D12Device10* Device, ID3D12DescriptorHeap** RTVHeap, UINT* RTVHeapHandleSize);
    void CreateBackBufferRTV(ID3D12Device10* Device, IDXGISwapChain4* SwapChain, Frame* Frames, ID3D12DescriptorHeap* RTVHeap, UINT RTVHeapHandleSize);
    void Transition(ID3D12GraphicsCommandList* CmdList,
                    D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After,
                    ID3D12Resource* Resource, UINT Subresource = 0);
    void CreateCommittedBuffer(ID3D12Device10* Device, D3D12_HEAP_TYPE HeapType,
                               UINT64 Size,
                               ID3D12Resource** Buffer,
                               D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE);
    void CreateCommitted2DTexture(ID3D12Device10* Device, UINT64 Width, UINT Height,
                                  D3D12_RESOURCE_FLAGS Flags, D3D12_HEAP_TYPE HeapType,
                                  ID3D12Resource** Texture,
                                  D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COPY_SOURCE,
                                  DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT16 MipLevels = 1);
    void CreateShaderCompiler(ShaderCompiler* Compiler);
    void CompileShader(ShaderCompiler* Compiler, Shader* Program);
    void CreateRootSignature(ID3D12Device10* Device, D3D12_ROOT_SIGNATURE_DESC1* Desc, ID3D12RootSignature** RootSignature);
    void CreateTopLevel(ID3D12Device10* Device, ID3D12GraphicsCommandList7* CmdList,
                        UINT NumInstances, ID3D12Resource* InstanceDescs,
                        ID3D12Resource** TopLevelASScratch, ID3D12Resource** TopLevelAS);
    void UpdateTopLevel(ID3D12GraphicsCommandList7* CmdList,
                        BottomLevelASInfo* BottomLevelInfos, INT NumBottomLevelInfos,
                        ID3D12Resource* TopLevelASScratch, ID3D12Resource* TopLevelAS, ID3D12Resource* InstanceDescs);
    void LoadModel(const char* FileName, Scene* InScene);
}