#include "Shared.h"

// ConstantBuffer<Constants> Globals : register(b0);

struct TriangleVertex
{
    float4 PositionHS : SV_Position; // For rendering.
    float4 PositionSS : POSITION0; // For calculating outlines in screen space.
    float4 Color : COLOR0;
    float3 Normal : NORMAL0;
};

static float4 TriangleVerticesPositions[] =
{
    float4(0.f, 0.f, 0.f, 1.f),// Screen center.
    float4(0.f, 1.f, 0.f, 1.f),// Screen center-top.
    float4(1.f, 0.f, 0.f, 1.f) // Screen center-right.
};

static uint3 TriangleIndices[] =
{
    uint3(0,1,2)
};

// One triangle with one outlined edge.
[outputtopology("triangle")]
[numthreads(3, 1, 1)] 
void MSMain(
    uint gtid : SV_GroupThreadID,
    out vertices TriangleVertex OutTriangleVertex[3],
    out indices uint3 OutTriangleIndices[1]
    )
{
    ConstantBuffer<Constants> Globals = ResourceDescriptorHeap[0];
    
    uint NumOutputVertices = 3;
    uint NumOutputPrimitives = 1; 
    SetMeshOutputCounts(NumOutputVertices, NumOutputPrimitives);

    if (gtid < NumOutputVertices)
    {
        float4 Position = TriangleVerticesPositions[gtid];
        // Position = mul(Position, Globals.View);
        OutTriangleVertex[gtid].PositionSS = Position;
        
        // Position = mul(Position, Globals.ViewProjection);
        OutTriangleVertex[gtid].PositionHS = Position;
        
        OutTriangleVertex[gtid].Color = float4(Globals.TestColor, 1.f);
        OutTriangleVertex[gtid].Normal = float3(0.f, 0.f, -1.f);
    }

    OutTriangleIndices[gtid] = TriangleIndices[gtid];
}

float4 PSMain(TriangleVertex In) : SV_Target
{
    return In.Color;
}
