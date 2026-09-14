import quelos;
import quelos.lighting;
import quelos.brdf;

[Material]
struct Material {
    [Texture2D] uint32_t BaseColor;
    [Texture2D] uint32_t NormalMap;
    [Texture2D] uint32_t MetallicRoughness; // G = roughness, B = metallic
    [Texture2D] uint32_t AOTexIdx;
    [Color] float4 BaseColorFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float2 UVTiling;
    float2 UVOffset;
};

__extern_cpp StructuredBuffer<Material> Materials;

Material GetMaterial(uint instanceId) {
    return Materials[Instances[instanceId].MaterialID];
}

[Pass("GBuffer")]
[shader("vertex")]
VertexOut vertexMain(VertexIn input, uint instanceID : SV_InstanceID) {
    VertexOut output;

    float4 worldPos = float4(input.position, 1.0);

    InstanceData instance = Instances[instanceID];
    output.position = mul(mul(worldPos, instance.Transform), global.viewProj);
    float3x3 t = (float3x3)instance.Transform;
    output.normal = mul(input.normal, t);
    output.Tangent = mul(input.tangent, t);
    output.Bitangent = mul(input.Bitangent, t);

    output.uv = input.texcoord0;
    output.Tangent = input.tangent;
    output.instanceID = instanceID;

    return output;
}

struct FragmentOut {
    float4 color : SV_Target0;
}

[shader("fragment")]
[Pass("GBuffer")]
FragmentOut fragmentMain(VertexOut input) {
    FragmentOut output;

    Material material = GetMaterial(input.instanceID);
    float2 uv = input.uv * material.UVTiling + material.UVOffset;

    // PBR pixel shader body
    float3 albedo = g_Textures[material.BaseColor]
                        .Sample(g_Textures_sampler, uv).rgb
                        * material.BaseColorFactor.rgb;

    // (metallic, roughness)
    float2 mr = g_Textures[material.MetallicRoughness]
                        .Sample(g_Textures_sampler, uv).bg;

    float metallic  = mr.x * material.MetallicFactor;
    float roughness = mr.y * material.RoughnessFactor;

    float ao = g_Textures[material.AOTexIdx].Sample(g_Textures_sampler, uv).r;

    // Normal mapping TODO: make it so normal maps have a placeholder texture
    /*float3 tangentNormal = g_Textures[material.NormalMap]
                        .Sample(g_Textures_sampler, input.uv).rgb * 2.0 - 1.0;*/
    float3 tangentNormal = float3(0.5, 0.5, 1.0) * 2.0 - 1.0;

    float3x3 TBN = float3x3(normalize(input.Tangent),
                             normalize(input.Bitangent),
                             normalize(input.normal));
    float3 N = normalize(mul(tangentNormal, TBN));

    float3 V = normalize(global.CameraPosition.xyz - input.position.xyz);
    float3 L = normalize(-global.Sun.Direction.xyz);
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    NdotV = max(NdotV, 1e-4);
    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));

    float3 brdf = EvaluateBRDF(
        albedo,
        metallic,
        roughness,
        NdotL,
        NdotV,
        NdotH,
        LdotH
    );

    float shadow = 1.0f;// clamp(GetShadowMask(input.position), 0.3, 1.0);
    float3 sunRadiance = global.Sun.Color * global.Sun.IlluminanceLux;

    float3 Lo = brdf * sunRadiance * NdotL * shadow;

    // TODO: Placeholder ambient
    float3 ambient = albedo * ao * 0.03;
    float3 colorHDR = Lo + ambient;

    output.color = float4(colorHDR, 1.0f);

    return output;
}
