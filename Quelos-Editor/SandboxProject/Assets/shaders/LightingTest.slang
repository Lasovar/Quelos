import quelos;

struct Material {
    [Color] float4 Color;
}

StructuredBuffer<Material> Materials;

[shader("vertex")]
[Pass("GBuffer")]
VertexOut vertex(VertexIn input, uint instanceID: SV_InstanceID) {
    VertexOut output;

    float4 worldPos = float4(input.position, 1.0);

    InstanceData instance = Instances[instanceID];
    output.position = mul(mul(worldPos, instance.Transform), global.viewProj);
    output.normal = mul(input.normal, (float3x3)instance.Transform);
    output.uv = input.texcoord0;
    output.instanceID = instanceID;

    return output;
}

[Pass("GBuffer")]
[shader("fragment")]
float4 fragment(VertexOut v) {
    Material mat = Materials[Instances[v.instanceID].MaterialID];
    return mat.Color;
}
