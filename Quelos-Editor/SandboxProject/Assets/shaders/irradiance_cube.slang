import quelos;
import quelos.ibl;

__extern_cpp TextureCube<float4>      _EnvCube;
__extern_cpp SamplerState             _EnvCube_sampler;   // linear min/mag/mip, clamp
__extern_cpp RWTexture2DArray<float4> _OutIrradiance;

static const uint k_SampleCount = 512;

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    uint outW, outH, faces;
    _OutIrradiance.GetDimensions(outW, outH, faces);
    if (id.x >= outW || id.y >= outH) return;

    float2 uv = (float2(id.xy) + 0.5) / float2(outW, outH);
    float3 N  = normalize(CubeTexelDirection(id.z, uv));

    // Source cube info, used to choose which mip each sample reads
    uint srcSize, srcH, srcMips;
    _EnvCube.GetDimensions(0, srcSize, srcH, srcMips);
    float texelSolidAngle = 4.0 * float.getPi() / (6.0 * srcSize * srcSize);

    float3 sum = 0.0;
    for (uint i = 0; i < k_SampleCount; ++i) {
        float3 l     = SampleCosineHemisphere(Hammersley(i, k_SampleCount));
        float3 L     = TangentToWorld(l, N);
        float  NdotL = l.z;

        // Filtered importance sampling: each sample stands for 1/(count * pdf) of the sphere,
        // so read the mip whose texels are about that size
        float pdf               = NdotL / float.getPi();
        float sampleSolidAngle  = 1.0 / (k_SampleCount * max(pdf, 1e-6));
        float lod = clamp(0.5 * log2(sampleSolidAngle / texelSolidAngle) + 1.0, 0.0, srcMips - 1);

        sum += _EnvCube.SampleLevel(_EnvCube_sampler, L, lod).rgb;
    }

    _OutIrradiance[id] = float4(sum / k_SampleCount, 1.0);
}