import quelos;
import quelos.brdf;
import quelos.ibl;

__extern_cpp TextureCube<float4> _EnvCube;
__extern_cpp SamplerState _EnvCube_sampler;    // linear min/mag/mip, clamp
__extern_cpp RWTexture2DArray<float4> _OutPrefiltered;

static const uint k_SampleCount = 1024;

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    uint outW, outH, faces;
    _OutPrefiltered.GetDimensions(outW, outH, faces);
    if (id.x >= outW || id.y >= outH) {
        return;
    }

    float  mip       = log2(float(k_PrefilterBaseSize) / float(outW));
    float  roughness = mip / float(k_PrefilterMipCount - 1);
    float2 uv        = (float2(id.xy) + 0.5) / float2(outW, outH);
    float3 N         = normalize(CubeTexelDirection(id.z, uv));

    // Mip 0 is a mirror, copy the source
    if (roughness <= 0.0) {
        _OutPrefiltered[id] = float4(_EnvCube.SampleLevel(_EnvCube_sampler, N, 0).rgb, 1.0);
        return;
    }

    float alpha = roughness * roughness;

    uint srcSize, srcH, srcMips;
    _EnvCube.GetDimensions(0, srcSize, srcH, srcMips);
    float texelSolidAngle = 4.0 * float.getPi() / (6.0 * srcSize * srcSize);

    // Split-sum assumption: view direction = reflection direction = normal
    float3 sum    = 0.0;
    float  weight = 0.0;
    for (uint i = 0; i < k_SampleCount; ++i) {
        float3 H     = TangentToWorld(ImportanceSampleGGX(Hammersley(i, k_SampleCount), alpha), N);
        float3 L     = 2.0 * dot(N, H) * H - N;
        float  NdotL = dot(N, L);
        if (NdotL <= 0.0) continue;

        // pdf of L for GGX importance sampling with V = N: D * NdotH / (4 * VdotH) = D / 4.
        // Your D_GGX omits PI (Frostbite form), so divide it in here.
        float NdotH = saturate(dot(N, H));
        float pdf   = D_GGX(NdotH, alpha) / (4.0 * k_Pi);

        float sampleSolidAngle = 1.0 / (k_SampleCount * max(pdf, 1e-6));
        float lod = clamp(0.5 * log2(sampleSolidAngle / texelSolidAngle) + 1.0, 0.0, srcMips - 1);

        sum    += _EnvCube.SampleLevel(_EnvCube_sampler, L, lod).rgb * NdotL;
        weight += NdotL;
    }

    _OutPrefiltered[id] = float4(sum / max(weight, 1e-6), 1.0);
}
