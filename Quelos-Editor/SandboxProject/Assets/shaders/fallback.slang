import quelos;

[shader("vertex")]
VertexOut vertexMain(VertexIn input, uint instanceID : SV_InstanceID) {
    VertexOut output;

    float4 worldPos = float4(input.position, 1.0);

    InstanceData instance = Instances[instanceID];
    output.position = mul(mul(worldPos, instance.Transform), global.viewProj);
    output.normal = mul(input.normal, (float3x3)instance.Transform);
    output.uv = input.texcoord0;

    return output;
}


struct FragmentOut {
    float4 color : SV_Target0;
}

[shader("fragment")]
FragmentOut fragmentMain(VertexOut v) {
    FragmentOut output;

    output.color = float4(1.0, 0.0, 1.0, 1.0);
    return output;
}
