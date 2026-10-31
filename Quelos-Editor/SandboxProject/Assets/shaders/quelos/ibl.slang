import surface;

static const uint k_PrefilterBaseSize = 256;
static const uint k_PrefilterMipCount = 6;

__extern_cpp TextureCube<float4> _Irradiance;
__extern_cpp SamplerState _Irradiance_sampler;
__extern_cpp Texture2D<float4> _InDFG;
__extern_cpp SamplerState _InDFG_sampler;
__extern_cpp TextureCube<float4> _InPrefilteredMap;
__extern_cpp SamplerState _InPrefilteredMap_sampler;

float3 CubeTexelDirection(uint face, float2 uv) {
    float s = uv.x * 2.0 - 1.0;
    float t = uv.y * 2.0 - 1.0;

    switch (face) {
        case 0:  return float3( 1.0,   -t,   -s);   // +X
        case 1:  return float3(-1.0,   -t,    s);   // -X
        case 2:  return float3(   s,  1.0,    t);   // +Y
        case 3:  return float3(   s, -1.0,   -t);   // -Y
        case 4:  return float3(   s,   -t,  1.0);   // +Z
        default: return float3(  -s,   -t, -1.0);   // -Z
    }
}

float2 Hammersley(uint i, uint n) {
    return float2(float(i) / float(n), float(reversebits(i)) * 2.3283064365386963e-10);
}

float3 SampleCosineHemisphere(float2 xi) {
    float phi = 2.0 * float.getPi() * xi.x;
    float r   = sqrt(xi.y);
    return float3(r * cos(phi), r * sin(phi), sqrt(1.0 - xi.y));
}

// Builds an orthonormal basis around N, then rotates a tangent-space vector into world space
float3 TangentToWorld(float3 v, float3 N) {
    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 T  = normalize(cross(up, N));
    float3 B  = cross(N, T);
    return T * v.x + B * v.y + N * v.z;
}

// Tangent space, N = +Z
float3 ImportanceSampleGGX(float2 xi, float alpha) {
    float phi      = 2.0 * float.getPi() * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Frostbite: shift the lookup toward N for rough surfaces, matching where the lobe actually peaks
float3 GetSpecularDominantDirection(float3 N, float3 R, float alpha) {
    float smoothness = saturate(1.0 - alpha);
    float lerpFactor = smoothness * (sqrt(smoothness) + alpha);
    return lerp(N, R, lerpFactor);
}

// Frostbite: specular occlusion derived from AO
float ComputeSpecOcclusion(float NdotV, float ao, float alpha) {
    return saturate(pow(NdotV + ao, exp2(-16.0 * alpha - 1.0)) - 1.0 + ao);
}

float3 EvaluateIBL(SurfaceData s) {
    float3 dfg = _InDFG.SampleLevel(_InDFG_sampler, float2(s.NdotV, s.Roughness), 0).rgb;

    float3 diffuse  = _Irradiance.SampleLevel(_Irradiance_sampler, s.N, 0).rgb * s.DiffuseColor * dfg.z;
    float3 R        = GetSpecularDominantDirection(s.N, reflect(-s.V, s.N), s.Alpha);
    float  lod      = s.Roughness * (k_PrefilterMipCount - 1);
    float3 prefil   = _InPrefilteredMap.SampleLevel(_InPrefilteredMap_sampler, R, lod).rgb;
    float3 specular = prefil * (s.F0 * dfg.x + s.F90 * dfg.y);

    return diffuse * s.AO + specular;
}
