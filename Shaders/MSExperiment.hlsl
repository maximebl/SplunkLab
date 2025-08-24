#include "Shared.h"

// ConstantBuffer<Constants> Globals : register(b0);

struct TriangleVertex
{
    float4 PositionHS : SV_Position; // For rendering.
    float4 PositionSS : POSITION0; // For calculating outlines in screen space.
    float4 Color : COLOR0;
    float3 Normal : NORMAL0;
};

// Front is clockwise.
static float4 TriangleVerticesPositions[] =
{
    float4(0.f, 0.f, 0.f, 1.f), // Screen center.
    float4(0.f, 1.f, 0.f, 1.f), // Screen center-top.
    float4(1.f, 0.f, -1.f, 1.f), // Screen center-right.
    float4(-1.f, 0.f,-1.f, 1.f) // Screen center-left.
};

static uint3 TriangleIndices[] =
{
    uint3(0,1,2), // Triangle 0.
    uint3(0,3,1)  // Triangle 1.
};

// One triangle with one outlined edge.
[outputtopology("triangle")] // Primitive to output.
[numthreads(4, 1, 1)] 
void MSMain(
    uint gtid : SV_GroupThreadID,
    out vertices TriangleVertex OutTriangleVertex[4],
    out indices uint3 OutTriangleIndices[2]
    )
{
    ConstantBuffer<Constants> Globals = ResourceDescriptorHeap[0];
    
    uint NumOutputVertices = 4;
    uint NumOutputTriangles = 2; 
    SetMeshOutputCounts(NumOutputVertices, NumOutputTriangles);

    if (gtid < NumOutputVertices)
    {
        // Position.
        float4 Position = TriangleVerticesPositions[gtid];
        // Position = mul(Position, Globals.View);
        // OutTriangleVertex[gtid].PositionSS = Position;
        matrix MVP = mul(Globals.Model, Globals.ViewProjection);
        Position = mul(Position, MVP);
        OutTriangleVertex[gtid].PositionHS = Position;

        // Color
        OutTriangleVertex[gtid].Color = float4(Globals.TestColor, 1.f);

        // Normals.
        // float3 SummedNormals = 0;
        float3 TriNormal = 0;
        [unroll]
        for (int tri = 0; tri < NumOutputTriangles; tri++)
        {
            uint3 TriId = TriangleIndices[tri];
            // Does the current id belong to this triangle?
            if (any(gtid == TriId))
            {
                // Get the three vertex positions of this triangle.
                float3 vp0 = mul(TriangleVerticesPositions[TriId.x], Globals.Model).xyz;
                float3 vp1 = mul(TriangleVerticesPositions[TriId.y], Globals.Model).xyz;
                float3 vp2 = mul(TriangleVerticesPositions[TriId.z], Globals.Model).xyz;
                TriNormal = normalize(cross(vp2 - vp0, vp1 - vp0));
                // SummedNormals += TriNormal;
            }
        }
        // float3 SmoothNormal = normalize(SummedNormals);
        OutTriangleVertex[gtid].Normal = TriNormal;
    }

    // Assign indices.
    OutTriangleIndices = TriangleIndices;
}

float4 PSMain(TriangleVertex In) : SV_Target
{
    return float4(In.Normal, 1.f); 
}
