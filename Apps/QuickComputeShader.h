#pragma once
#include "../Headers/Gpu.h"

namespace QuickComputeShader
{
    struct QuickComputeShaderData
    {
        Shader QuickShader;

        // Synchronization.
        ComPtr<ID3D12Fence> Fence;
        HANDLE FenceEvent;
        UINT64 FenceValue;
    };

    void UpdateAndRender(QuickComputeShaderData& Data, ID3D12GraphicsCommandList7* CmdList, INT Width, INT Height);
}
