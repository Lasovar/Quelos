import quelos;

[Material]
struct ToonMaterial {
    [Color] float4 Color;
    [Texture2D] uint MainTex;
};

StructuredBuffer<ToonMaterial> Materials;

ToonMaterial GetMaterial(uint instanceId) {
    return Materials[Instances[instanceId].MaterialID];
}

[shader("vertex")]
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
FragmentOut fragmentMain(VertexOut v) {
    FragmentOut output;

    ToonMaterial mat = GetMaterial(v.instanceID);
    float3 N = normalize(v.normal);

    output.color = g_Textures[mat.MainTex].Sample(g_Textures_sampler, v.uv) * mat.Color;

    return output;
}
