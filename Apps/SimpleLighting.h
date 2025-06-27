#pragma once
#include "../Headers/Gpu.h"

namespace SimpleLighting
{
    struct SimpleLightingData
    {
        Shader SimpleMS;
        Shader SimplePS;
        ComPtr<ID3D12RootSignature> RootSig;
        ComPtr<ID3D12PipelineState> PSO;
    };

    void UpdateAndRender(SimpleLightingData& Data,
                         Frame* CurrentFrame,
                         ID3D12GraphicsCommandList7* CmdList,
                         INT Width, INT Height);
}
