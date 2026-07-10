import quelos;
import quelos.lighting;

[Material]
struct Material {
    [Color] float4 Color;
    float Power;
    [Texture2D] uint _MainTex;
    [Texture2D] uint _NormalTex;
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
    output.normal = mul(input.normal, (float3x3)instance.Transform);
    output.uv = input.texcoord0;
    output.instanceID = instanceID;

    return output;
}

struct FragmentOut {
    float4 color : SV_Target0;
}

[shader("fragment")]
[Pass("GBuffer")]
FragmentOut fragmentMain(VertexOut v) {
    FragmentOut output;

    Material mat = GetMaterial(v.instanceID);

    Texture2D mainTex = g_Textures[GetMaterial(v.instanceID)._MainTex];
    float3 N = normalize(v.normal);

    float shadow = clamp(GetShadowMask(v.position), 0.3, 1.0);

    float4 texColor = g_Textures[mat._MainTex].Sample(g_Textures_sampler, v.uv) * mat.Color;
    output.color = float4(texColor.rgb * shadow, texColor.a);

    return output;
}
