import quelos;
import quelos.brdf;
import quelos.ibl;

__extern_cpp RWTexture2D<float4> _OutDFG;

static const uint k_SampleCount = 1024;

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    uint w, h;
    _OutDFG.GetDimensions(w, h);
    if (id.x >= w || id.y >= h) {
        return;
    }

    float NdotV     = max((id.x + 0.5) / w, 1e-4);
    float roughness = (id.y + 0.5) / h;               // perceptual
    float alpha     = roughness * roughness;

    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    float3 r = 0.0;

    for (uint i = 0; i < k_SampleCount; ++i) {
        float2 xi = Hammersley(i, k_SampleCount);

        // Specular: GGX importance sampling
        float3 H = ImportanceSampleGGX(xi, alpha);
        float3 L = 2.0 * dot(V, H) * H - V;
        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));
        if (NdotL > 0.0) {
            float Vis = V_SmithGGXCorrelated(NdotL, NdotV, alpha);
            float Gv  = Vis * 4.0 * NdotL * VdotH / NdotH;   // BRDF * NdotL / pdf, without D and F
            float Fc  = pow(1.0 - VdotH, 5.0);
            r.x += (1.0 - Fc) * Gv;
            r.y += Fc * Gv;
        }

        // Diffuse: cosine-weighted sampling, so pdf cancels NdotL / PI
        float3 Ld     = SampleCosineHemisphere(xi);
        float  NdotLd = Ld.z;
        if (NdotLd > 0.0) {
            float LdotH = saturate(dot(Ld, normalize(V + Ld)));
            r.z += Fr_DisneyDiffuse(NdotV, NdotLd, LdotH, roughness);
        }
    }

    _OutDFG[id.xy] = float4(r / k_SampleCount, 1.0);
}
