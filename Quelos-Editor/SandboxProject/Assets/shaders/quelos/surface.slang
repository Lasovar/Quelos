struct SurfaceData {
    float3 N, V;
    float3 DiffuseColor;
    float3 F0;
    float  F90;
    float  Roughness;   // perceptual
    float  Alpha;       // roughness ^ 2, clamped
    float  NdotV;
    float  AO;
};

SurfaceData MakeSurface(float3 albedo, float metallic, float roughness, float3 N, float3 V, float ao) {
    SurfaceData s;
    s.N            = N;
    s.V            = V;
    s.DiffuseColor = albedo * (1.0 - metallic);
    s.F0           = lerp(float3(0.04), albedo, metallic);
    s.F90          = 1.0;
    s.Roughness    = roughness;
    s.Alpha        = max(roughness * roughness, 0.0045);
    s.NdotV        = abs(dot(N, V)) + 1e-5;
    s.AO           = ao;

    return s;
}
