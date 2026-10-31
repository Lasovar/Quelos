import quelos;
import quelos.lighting;
import quelos.surface;
import quelos.shading;

[Material]
struct Material {
    [Texture2D] uint32_t BaseColor;
    [NormalMap] uint32_t NormalMap;
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

struct VOut {
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    uint instanceID : SV_InstanceID;
};

[Pass("GBuffer")]
[shader("vertex")]
VOut vertexMain(VertexIn input, uint instanceID : SV_InstanceID) {
    VOut output;

    float4 objectPos = float4(input.position, 1.0);

    InstanceData instance = Instances[instanceID];
    float4 world = mul(objectPos, instance.Transform);

    output.worldPos = world.xyz;
    output.position = mul(world, global.ViewProj);

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

float3 SampleNormalTS(Texture2D t, SamplerState s, float2 uv) {
    float2 xy = t.Sample(s, uv).rg * 2.0 - 1.0;
    float z = sqrt(saturate(1.0 - dot(xy, xy)));
    return float3(xy, z);
}

[shader("fragment")]
[Pass("GBuffer")]
FragmentOut fragmentMain(VOut input) {
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

    // Normal mapping
    float3 tangentNormal = SampleNormalTS(g_Textures[material.NormalMap], g_Textures_sampler, uv);

    float3x3 TBN = float3x3(normalize(input.Tangent),
                             normalize(input.Bitangent),
                             normalize(input.normal));

    float3 N = normalize(mul(tangentNormal, TBN));
    float3 V = normalize(global.CameraPositionAndEnvIntensity.xyz - input.worldPos);
    float3 L = normalize(-global.Sun.Direction.xyz);
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    NdotV = max(NdotV, 1e-4);
    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));

    SurfaceData surfaceData = MakeSurface(
        albedo,
        metallic,
        roughness,
        N,
        V,
        ao
    );

    float shadow = saturate(GetShadowMask(input.position));
    float3 sunRadiance = global.Sun.ColorAndIlluminanceLux.xyz
            * global.Sun.ColorAndIlluminanceLux.w
            * shadow;

    float3 colorHDR = ShadeSurface(
        surfaceData,
        global.Sun.Direction.xyz,
        sunRadiance,
        global.CameraPositionAndEnvIntensity.w
    );

    output.color = float4(colorHDR, 1.0f);

    return output;
}
