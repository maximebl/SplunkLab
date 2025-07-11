struct TriangleVertex
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

static float4 TriangleVerticesPositions[] =
{
    float4(0.f, 0.f, 0.f, 1.f), // Screen center.
    float4(0.f, 1.f, 0.f, 1.f),// Screen center-top.
    float4(1.f, 0.f, 0.f, 1.f), // Sceen center-right.
};

static uint3 TriangleIndices[] =
{
    uint3(0,1,2),
};

[outputtopology("triangle")]
[numthreads(3,1,1)] 
void MSMain(
    uint gtid : SV_GroupThreadID,
    out vertices TriangleVertex OutTriangleVertex[3],
    out indices uint3 OutTriangleIndices[1]
    )
{
    uint NumOutputVertices = 3;
    uint NumOutputPrimitives = 1;
    SetMeshOutputCounts(NumOutputVertices, NumOutputPrimitives);

    if (gtid < NumOutputVertices)
    {
        // This will execute 8 times.
        float4 Position = TriangleVerticesPositions[gtid];
        OutTriangleVertex[gtid].Position = Position;
        OutTriangleVertex[gtid].Color = float4(1.f, 1.f, 0.f, 1.f);
    }

    OutTriangleIndices[gtid] = TriangleIndices[gtid];
}

float4 PSMain(TriangleVertex In) : SV_Target
{
    return In.Color;
}