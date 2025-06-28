#pragma once
#include "../Headers/Gpu.h"

namespace MSHelloTriangle
{
    struct MSHelloTriangleData
    {
        Shader SimpleMS;
        Shader SimplePS;
        ComPtr<ID3D12RootSignature> RootSig;
        ComPtr<ID3D12PipelineState> GreenTrianglePSO;
    };

    void UpdateAndRender(MSHelloTriangleData& Data,
                         Frame* CurrentFrame,
                         ID3D12GraphicsCommandList7* CmdList,
                         INT Width, INT Height);
}
