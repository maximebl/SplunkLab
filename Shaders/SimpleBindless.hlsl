
[numthreads(32, 32, 1)]
void Main(uint3 DTid : SV_DispatchThreadID)
{
    RWTexture2D<float4> MyTexture = ResourceDescriptorHeap[0];
    MyTexture[DTid.xy] = 1.f;
}
