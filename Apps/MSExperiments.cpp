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
    float SinWave = XMScalarSinEst(ElapsedSeconds) - 0.5f;
    XMMATRIX Projection = Data.Camera.GetProjectionMatrix(FieldOfView, AspectRatio);
    XMMATRIX View = Data.Camera.GetViewMatrix();
    XMMATRIX World = XMMatrixIdentity();
    XMMATRIX XRotation = XMMatrixRotationX(SinWave);
    XMMATRIX YTranslation = XMMatrixTranslation(0.f, 5.f, 0.f);
    XMStoreFloat4x4(&Data.SceneConstants->View, XMMatrixTranspose(View));
    XMStoreFloat4x4(&Data.SceneConstants->ViewProjection, XMMatrixTranspose(View * Projection));
    XMStoreFloat4x4(&Data.SceneConstants->Model, XMMatrixTranspose(World * XRotation * YTranslation));
    Data.SceneConstants->CameraPosition = Data.Camera.Position;

    // Color.
    Data.SceneConstants->TestColor = XMFLOAT3(0.f, 0.f, SinWave);
    
    // Render.
    D3D::Transition(CmdList,
                    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET,
                    CurrentFrame->BackBuffer.Get());
    D3D12_VIEWPORT Viewport = {0.f, 0.f, (float)Width, (float)Height, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
    D3D12_RECT ScissorRect = {0, 0, Width, Height};

    CmdList->SetDescriptorHeaps(1, Data.CSUHeap.GetAddressOf());
    CmdList->SetGraphicsRootSignature(Data.RootSig.Get());
    CmdList->SetPipelineState(Data.CubeInstancingPSO.Get());
    CmdList->ClearRenderTargetView(CurrentFrame->RTVHandle, DirectX::Colors::Black, 1, &ScissorRect);
    CmdList->OMSetRenderTargets(1, &CurrentFrame->RTVHandle, FALSE, &CurrentFrame->DSVHandle);
    CmdList->ClearDepthStencilView(CurrentFrame->DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->DispatchMesh(1, 1, 1);

    D3D::Transition(CmdList,
                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
                    CurrentFrame->BackBuffer.Get());
}
