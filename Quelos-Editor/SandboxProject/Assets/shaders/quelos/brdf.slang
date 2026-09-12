static const float k_Pi = 3.14159265359;

// Trowbridge-Reitz / GGX normal distribution function
float D_GGX(float NdotH, float alpha) {
    float alpha2 = alpha * alpha;
    float d = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (k_Pi * d * d + 1e-7);
}

// Height-correlated Smith masking-shadowing, visibility form
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alpha) {
    float alpha2 = alpha * alpha;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alpha2) + alpha2);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alpha2) + alpha2);
    return 0.5 / max(GGXV + GGXL, 1e-5);
}

float3 F_Schlick(float VdotH, float3 F0) {
    float f = pow(1.0 - VdotH, 5.0);
    return F0 + (1.0 - F0) * f;
}

// Standard metallic-workflow Cook-Torrance, returns outgoing radiance
float3 EvaluateBRDF(
    float3 albedo,
    float metallic,
    float roughness,
    float NdotL,
    float NdotV,
    float NdotH,
    float VdotH
) {
    // clamp, never let alpha hit 0 - D_GGX and V_Smith both degenerate there
    float alpha = max(roughness * roughness, 0.0045);

    float3 F0 = lerp(float3(0.04), albedo, metallic);
    float3 diffuseColor = albedo * (1.0 - metallic);

    float D = D_GGX(NdotH, alpha);
    float Vis = V_SmithGGXCorrelated(NdotL, NdotV, alpha);
    float3 F = F_Schlick(VdotH, F0);

    float3 specular = D * Vis * F;
    float3 diffuse = diffuseColor * (1.0 - F) * (1.0 / k_Pi);

    return diffuse + specular;
}
