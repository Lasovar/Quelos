import surface;

__extern_cpp TextureCube<float4> _Irradiance;
__extern_cpp SamplerState _Irradiance_sampler;
__extern_cpp Texture2D<float4> _InDFG;
__extern_cpp SamplerState _InDFG_sampler;

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

float3 EvaluateIBL(SurfaceData s) {
    float3 dfg = _InDFG.SampleLevel(_InDFG_sampler, float2(s.NdotV, s.Roughness), 0).rgb;

    float3 diffuse  = _Irradiance.SampleLevel(_Irradiance_sampler, s.N, 0).rgb * s.DiffuseColor * dfg.z;
    float3 specular = 0.0;   // step 3: prefiltered * (s.F0 * dfg.x + s.F90 * dfg.y)

    return diffuse * s.AO + specular;
}
