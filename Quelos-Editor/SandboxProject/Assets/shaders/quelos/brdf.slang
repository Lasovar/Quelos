import surface;

static const float k_Pi = 3.14159265359;

float D_GGX(float NdotH, float alpha) {
    // Divide by PI is apply later
    float alpha2 = alpha * alpha;
    float f = (NdotH * alpha2 - NdotH) * NdotH + 1.0;
    return alpha2 / (f * f) ;
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG) {
    // Original formulation of G_SmithGGX Correlated
    // lambda_v = (-1 + sqrt (alphaG2 * (1 - NdotL2 ) / NdotL2 + 1)) * 0.5f;
    // lambda_l = (-1 + sqrt (alphaG2 * (1 - NdotV2 ) / NdotV2 + 1)) * 0.5f;
    // G_ SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l ) ;
    // V_ SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0 f * NdotL * NdotV);

    // This is the optimize version
    float alphaG2 = alphaG * alphaG;
    // Caution : the " NdotL *" and " NdotV *" are explicitely inversed, this is not a mistake.
    float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5 / (Lambda_GGXV + Lambda_GGXL);
}

float3 F_Schlick(float3 f0, float f90, float cosTheta) {
    return f0 + (f90 - f0) * pow(1.0 - cosTheta, 5);
}

float Fr_DisneyDiffuse(
    float NdotV,
    float NdotL,
    float LdotH,
    float linearRoughness
) {
   float energyBias = lerp(0 , 0.5 ,linearRoughness);
   float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
   float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
   float3 f0 = float3(1.0, 1.0, 1.0);
   float lightScatter = F_Schlick(f0, fd90, NdotL).r;
   float viewScatter = F_Schlick(f0, fd90, NdotV).r;

   return lightScatter * viewScatter * energyFactor;
}

float3 EvaluateDirect(SurfaceData s, float3 L, float3 radiance) {
    float3 H     = normalize(s.V + L);
    float  NdotL = saturate(dot(s.N, L));
    float  NdotH = saturate(dot(s.N, H));
    float  LdotH = saturate(dot(L, H));

    float  D   = D_GGX(NdotH, s.Alpha);
    float  Vis = V_SmithGGXCorrelated(NdotL, s.NdotV, s.Alpha);
    float3 F   = F_Schlick(s.F0, s.F90, LdotH);
    float3 Fr  = D * Vis * F / k_Pi;
    float3 Fd  = s.DiffuseColor * Fr_DisneyDiffuse(s.NdotV, NdotL, LdotH, s.Roughness) / k_Pi;

    return (Fd + Fr) * radiance * NdotL;
}

