groupshared uint gsMin, gsMax;

__extern_cpp Texture2DMS<float, 4> DepthTex;
__extern_cpp RWByteAddressBuffer ReductionOut;

[shader("compute")]
[numthreads(16, 16, 1)]
void CSMain(uint3 DTid: SV_DispatchThreadID, uint GI: SV_GroupIndex) {
    if (GI == 0) { gsMin = 0x7F7FFFFF; gsMax = 0; }
    GroupMemoryBarrierWithGroupSync();

    uint2 dims;
    uint sampleCount;
    DepthTex.GetDimensions(dims.x, dims.y, sampleCount);
    if (all(DTid.xy < dims)) {
        float d = DepthTex.Load(DTid.xy, 0);
        // skip far plane
        if (d > 0.0) {
            uint du = asuint(d); // safe for d in [0,1]: IEEE bit order == float order for positive
            InterlockedMin(gsMin, du);
            InterlockedMax(gsMax, du);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    if (GI == 0) {
        ReductionOut.InterlockedMin(0, gsMin);
        ReductionOut.InterlockedMax(4, gsMax);
    }
}
