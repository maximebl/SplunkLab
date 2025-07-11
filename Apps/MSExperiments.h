#pragma once
#include "../Headers/Gpu.h"
#include "SimpleCamera.h"
#include "StepTimer.h"

namespace MSExperiments
{
    struct MSExperimentsData
    {
        StepTimer Timer;
        SimpleCamera Camera;
        Shader SimpleMS;
        Shader SimplePS;
        ComPtr<ID3D12RootSignature> RootSig;
        ComPtr<ID3D12PipelineState> CubeInstancingPSO;
        ComPtr<ID3D12DescriptorHeap> CSUHeap;
        Constants* SceneConstants;
        ComPtr<ID3D12Resource> SceneConstantsBuffer;
    };

    void UpdateAndRender(MSExperimentsData& Data,
                         Frame* CurrentFrame,
                         ID3D12GraphicsCommandList7* CmdList,
                         INT Width, INT Height);
}
