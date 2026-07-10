import quelos;
import quelos.lighting;

[Material]
struct ToonMaterial {
    [Color] float4 Color;
    [Texture2D] uint MainTex;
    float OutlineWidth;
    [Color] float4 OutlineColor;
};

__extern_cpp StructuredBuffer<ToonMaterial> Materials;

ToonMaterial GetMaterial(uint instanceId) {
    return Materials[Instances[instanceId].MaterialID];
}

[shader("vertex")]
[Pass("GBuffer", 0)]
VertexOut vertexMain(VertexIn input, uint instanceID : SV_InstanceID) {
    VertexOut output;

    float4 worldPos = float4(input.position, 1.0);

    InstanceData instance = Instances[instanceID];
    output.position = mul(mul(worldPos, instance.Transform), global.viewProj);
    output.normal = mul(input.normal, (float3x3)instance.Transform);
    output.uv = input.texcoord0;
    output.instanceID = instanceID;

    return output;
}

struct FragmentOut {
    float4 color : SV_Target0;
}

[shader("fragment")]
[Pass("GBuffer", 0)]
FragmentOut fragmentMain(VertexOut v) {
    FragmentOut output;

    ToonMaterial mat = GetMaterial(v.instanceID);

    float3 N = normalize(v.normal);
    float3 L = normalize(-global.LightDirection.xyz);

    float ndotl = saturate(dot(N, L));

    float shadow = GetShadowMask(v.position);
    shadow = smoothstep(0.49, 0.51, shadow);
    shadow = min(shadow, step(0.001, ndotl));
    shadow = clamp(shadow, 0.3, 1.0);

    float ToonLevels = 4.0;
    float shade = floor(ndotl * ToonLevels) / ToonLevels;
    shade = lerp(0.35, 1.0, shade) * shadow;

    float4 albedo = g_Textures[mat.MainTex].Sample(g_Textures_sampler, v.uv) * mat.Color;
    output.color = float4(albedo.rgb * shade, albedo.a);

    return output;
}

struct OutlineVSOut {
    float4 Position: SV_Position;
    uint InstanceID: SV_InstanceID;
}

[shader("vertex")]
[DepthEnable(true)]
[DepthWriteEnable(false)]
[CullMode(CullMode.Front)]
[Pass("GBuffer", -1)]
OutlineVSOut OutlineVS(VertexIn input, uint instanceID: SV_InstanceID) {
    InstanceData instance = Instances[instanceID];

    float3 worldPos = mul(float4(input.position, 1.0), instance.Transform).xyz;
    float3 worldSmoothNormal = normalize(mul(input.SmoothNormal, (float3x3)instance.Transform));

    float4 clipPos = mul(float4(worldPos, 1.0), global.viewProj);
    float width = Materials[instance.MaterialID].OutlineWidth;

    worldPos += worldSmoothNormal * width * clipPos.w;

    OutlineVSOut output;
    output.Position = mul(float4(worldPos, 1.0), global.viewProj);
    output.InstanceID = instanceID;
    return output;
}

[shader("fragment")]
[Pass("GBuffer", -1)]
float4 fragmentOutlineMask(OutlineVSOut v) : SV_Target {
    return Materials[Instances[v.InstanceID].MaterialID].OutlineColor;
}
