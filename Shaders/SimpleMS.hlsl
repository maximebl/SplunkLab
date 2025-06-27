struct CubeVertex
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

static float4 CubeVerticesPositions[] =
{
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, 0.5f, 1.0f),
    float4(-0.5f, 0.5f, -0.5f, 1.0f),
    float4(-0.5f, 0.5f, 0.5f, 1.0f),
    float4(0.5f, -0.5f, -0.5f, 1.0f),
    float4(0.5f, -0.5f, 0.5f, 1.0f),
    float4(0.5f, 0.5f, -0.5f, 1.0f),
    float4(0.5f, 0.5f, 0.5f, 1.0f)
};

static uint3 CubeIndices[] =
{
    uint3(0, 2, 1),
    uint3(1, 2, 3),
    uint3(4, 5, 6),
    uint3(5, 7, 6),
    uint3(0, 1, 5),
    uint3(0, 5, 4),
    uint3(2, 6, 7),
    uint3(2, 7, 3),
    uint3(0, 4, 6),
    uint3(0, 6, 2),
    uint3(1, 3, 7),
    uint3(1, 7, 5)
};

[outputtopology("triangle")]
[numthreads(12,1,1)] 
void MSMain(
    uint gtid : SV_GroupThreadID,
    out vertices CubeVertex OutCubeVertex[8],
    out indices CubeIndices OutCubeIndices[12]
    )
{
    uint NumOutputVertices = 8;
    uint NumOutputPrimitives = 12;
    SetMeshOutputCounts(NumOutputVertices, NumOutputPrimitives);

    if (gtid < NumOutputVertices) // There's only 8 vertices, but the gtid goes up to 12.
    {
        // This will execute 8 times.
        float4 Position = CubeVerticesPositions[gtid];
        OutCubeVertex[gtid].Position = Position;
        OutCubeVertex[gtid].Color = float4(1.f, 0.f, 0.f, 1.f);
    }

    // This will execute 12 times.
    OutCubeIndices[gtid] = CubeIndices[gtid];
}