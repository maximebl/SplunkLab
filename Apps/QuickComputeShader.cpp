#include "QuickComputeShader.h"

using namespace DirectX;

void QuickComputeShader::UpdateAndRender(QuickComputeShaderData& Data,
                                         ID3D12GraphicsCommandList7* CmdList,
                                         INT Width, INT Height)
{
    CmdList->SetDescriptorHeaps(1, Data.CSUHeap.GetAddressOf());
    CmdList->SetComputeRootSignature(Data.RootSig.Get());
    CmdList->SetPipelineState(Data.PSO.Get());
    CmdList->Dispatch(1,1,1);
}
