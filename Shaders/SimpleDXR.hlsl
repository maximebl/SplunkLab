RWTexture2D<float4> Output : register(u0);
RaytracingAccelerationStructure SceneAS : register(t0);

// One CB shared by all instances.
struct PerFrameData
{
    float3 A[3];
    float3 B[3];
    float3 C[3];
};
ConstantBuffer<PerFrameData> PerFrame : register(b0);

// One CB per-instance.
struct PerInstanceData
{
    float3 A;
    float3 B;
    float3 C;
};
ConstantBuffer<PerInstanceData> PerInstance : register(b1);

struct RayPayload
{
    float3 Color;
};

float3 LinearToSrgb(float3 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

[shader("raygeneration")]
void RayGenerationMain()
{
    uint3 RayIndex = DispatchRaysIndex(); // ([0..WindowWidth], [0..WindowHeight])
    uint3 RayDim = DispatchRaysDimensions(); // (WindowWidth, WindowHeight)

    float2 UVs = float2(RayIndex.xy) / float2(RayDim.xy);
    UVs = UVs * 2.f - 1.f; // Offset to fit [-1.f, 1.f]

    RayDesc Ray;
    Ray.Origin = float3(0.f, 0.f, -2.f);
    Ray.Direction = normalize(float3(UVs.x, -UVs.y, 1.f));
    Ray.TMin = 0.f;
    Ray.TMax = 10000.f;

    RayPayload Payload;
    uint InstanceInclusionMask = 0xFF;
    uint RayContributionToHitGroupIndex = 0;
    uint MultiplierForGeometryContributionToHitGroupIndex = 0;
    uint MissShaderIndex = 0;
    TraceRay(SceneAS,
             RAY_FLAG_NONE,
             InstanceInclusionMask, RayContributionToHitGroupIndex, MultiplierForGeometryContributionToHitGroupIndex,
             MissShaderIndex,
             Ray,
             Payload);

    Output[RayIndex.xy] = float4(LinearToSrgb(Payload.Color), 1.f);
}

[shader("miss")]
void MissMain(inout RayPayload Payload)
{
    Payload.Color = float3(0.5f, 0.5f, 0.9f);
}

[shader("closesthit")]
void ClosestHitMain(inout RayPayload Payload,
                    // Result of the built-in intersection shader.
                    in BuiltInTriangleIntersectionAttributes Intersection)
{

    float3 Barycentrics = float3(1.0 - Intersection.barycentrics.x - Intersection.barycentrics.y,
                                 Intersection.barycentrics.x,
                                 Intersection.barycentrics.y);

    uint Instance = InstanceID();
    
    // Payload.Color = PerFrame.A[Instance] * Barycentrics.x
    //     + PerFrame.B[Instance] * Barycentrics.y
    //     + PerFrame.C[Instance] * Barycentrics.z;

    Payload.Color = PerInstance.A * Barycentrics.x
        + PerInstance.B * Barycentrics.y
        + PerInstance.C * Barycentrics.z;

    // Payload.Color = PerFrame.A[Instance] * Barycentrics.x * PerInstance.A
    //     + PerFrame.B[Instance] * Barycentrics.y 
    //     + PerFrame.C[Instance] * Barycentrics.z  * PerInstance.C;
}

// Plane-specific shaders.
struct ShadowRayPayload
{
    bool Hit;
    uint ShadowOpacity;
};

[shader("closesthit")]
void ClosestHitMainPlane(inout RayPayload Payload,
                    // Result of the built-in intersection shader.
                    in BuiltInTriangleIntersectionAttributes Intersection)
{
    // Data coming from the RayGen TraceRay() call that hit the plane.
    float Distance = RayTCurrent();
    float3 Direction = WorldRayDirection();
    float3 Origin = WorldRayOrigin();
    float3 IntersectionPointOnPlane = Origin + Distance * Direction; // Where did the ray land on the plane?
    
    RayDesc Ray;
    Ray.Origin = IntersectionPointOnPlane;
    Ray.Direction = normalize(float3(0.f, 0.5f, 0.1f)); // Direction towards the light source.
    Ray.TMin = 0.01f;
    Ray.TMax = 10000.f;

    // Launch a ray from the plane in the direction of the light source.
    uint InstanceInclusionMask = 0xFF;
    uint RayContributionToHitGroupIndex = 1;
    uint MultiplierForGeometryContributionToHitGroupIndex = 0;
    uint MissShaderIndex = 1;
    
    ShadowRayPayload ShadowPayload;
    TraceRay(SceneAS,
             RAY_FLAG_NONE,
             InstanceInclusionMask, RayContributionToHitGroupIndex, MultiplierForGeometryContributionToHitGroupIndex,
             MissShaderIndex,
             Ray,
             ShadowPayload);

   float ShadowFactor = ShadowPayload.Hit
                            ? 0.3f // The shadow ray hit an object. This means the light source is occluded.
                            : 1.f; // The shadow ray hit nothing. There is nothing between this intersection point and the light source.

    float3 PlaneColor = float3(0.8f, 0.8f, 0.8f);
    Payload.Color = PlaneColor * ShadowFactor * ShadowPayload.ShadowOpacity;
}

[shader("closesthit")]
void ShadowClosestHitMain(inout ShadowRayPayload ShadowPayload,
                // Result of the built-in intersection shader.
                in BuiltInTriangleIntersectionAttributes Intersection)
{
    ShadowPayload.Hit = true;
    ShadowPayload.ShadowOpacity = InstanceID(); // The instance of what we hit.
}

[shader("miss")]
void ShadowMissMain(inout ShadowRayPayload ShadowPayload)
{
   ShadowPayload.Hit = false;
   ShadowPayload.ShadowOpacity = 1;
}
