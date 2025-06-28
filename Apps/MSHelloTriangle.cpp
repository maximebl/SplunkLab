#include "MSHelloTriangle.h"

using namespace DirectX;

void MSHelloTriangle::UpdateAndRender(MSHelloTriangleData& Data,
                                     Frame* CurrentFrame,
                                     ID3D12GraphicsCommandList7* CmdList,
                                     INT Width, INT Height)
{
    D3D12_VIEWPORT Viewport = {0.f, 0.f, (float)Width, (float)Height, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
    D3D12_RECT ScissorRect = {0, 0, Width, Height};
    
    CmdList->SetGraphicsRootSignature(Data.RootSig.Get());
    CmdList->SetPipelineState(Data.GreenTrianglePSO.Get());
    CmdList->OMSetRenderTargets(1, &CurrentFrame->RTVHandle,
                                FALSE, &CurrentFrame->DSVHandle);
    CmdList->ClearDepthStencilView(CurrentFrame->DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->DispatchMesh(1,1,1);
}
