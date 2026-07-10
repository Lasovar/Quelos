import quelos;

__extern_cpp ConstantBuffer<float4x4> ViewProjection;

struct VSMainOut {
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
};

[shader("vertex")]
[Pass("ShadowDepth")]
[DepthEnable]
[DepthWriteEnable]
[CullMode(CullMode.Back)]
VSMainOut VertexMain(VertexIn v, uint instanceID : SV_InstanceID) {
    InstanceData instance = Instances[instanceID];
    VSMainOut output;
    output.Position = mul(mul(float4(v.position, 1.0), instance.Transform), ViewProjection);
    output.Normal = normalize(mul(v.normal, (float3x3)instance.Transform));
    return output;
}

struct PrePassOut {
    float4 Normal : SV_Target0;
};

[shader("fragment")]
[Pass("ShadowDepth")]
PrePassOut FragmentMain(VSMainOut v) {
    PrePassOut output;
    output.Normal = float4(v.Normal * 0.5 + 0.5, 0.0); // encode normal from snorm to unorm
    return output;
}
