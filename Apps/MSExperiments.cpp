#include "MSExperiments.h"

using namespace DirectX;

void MSExperiments::UpdateAndRender(MSExperimentsData& Data,
                                     Frame* CurrentFrame,
                                     ID3D12GraphicsCommandList7* CmdList,
                                     INT Width, INT Height)
{
    // Time.
    Data.Timer.Tick();
    float DeltaTime = static_cast<float>(Data.Timer.GetElapsedSeconds());
    float ElapsedSeconds = static_cast<float>(Data.Timer.GetTotalSeconds());

    // Camera.
    Data.Camera.Update(DeltaTime);
    constexpr float FieldOfView = XM_PI / 2.f;
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    XMMATRIX Projection = Data.Camera.GetProjectionMatrix(FieldOfView, AspectRatio);
    XMMATRIX View = Data.Camera.GetViewMatrix();
    XMStoreFloat4x4(&Data.SceneConstants->View, XMMatrixTranspose(View));
    XMStoreFloat4x4(&Data.SceneConstants->ViewProjection, XMMatrixTranspose(View * Projection));
    Data.SceneConstants->TestColor = XMFLOAT3(0.f, 0.f, abs(XMScalarSinEst(ElapsedSeconds)));
    
    // Render.
    D3D12_VIEWPORT Viewport = {0.f, 0.f, (float)Width, (float)Height, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
    D3D12_RECT ScissorRect = {0, 0, Width, Height};

    CmdList->SetDescriptorHeaps(1, Data.CSUHeap.GetAddressOf());
    CmdList->SetGraphicsRootSignature(Data.RootSig.Get());
    CmdList->SetPipelineState(Data.CubeInstancingPSO.Get());
    CmdList->OMSetRenderTargets(1, &CurrentFrame->RTVHandle, FALSE, &CurrentFrame->DSVHandle);
    CmdList->ClearDepthStencilView(CurrentFrame->DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->DispatchMesh(1, 1, 1);
}
