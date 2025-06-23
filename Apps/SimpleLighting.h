#pragma once
#include "../Headers/Gpu.h"

namespace SimpleLighting
{
   void UpdateAndRender(ID3D12GraphicsCommandList7* CmdList, INT Width, INT Height);
}
