#include "SimpleLighting.h"

using namespace DirectX;

void SimpleLighting::UpdateAndRender(SimpleLightingData& Data,
                                     Frame* CurrentFrame,
                                     ID3D12GraphicsCommandList7* CmdList,
                                     INT Width, INT Height)
{
    CmdList->OMSetRenderTargets(1, &CurrentFrame->RTVHandle,
                                FALSE, &CurrentFrame->DSVHandle);
}
