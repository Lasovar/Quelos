import quelos;

__extern_cpp ConstantBuffer<float4x4> LightViewProjection;

[shader("vertex")]
[Pass("ShadowDepth")]
[DepthEnable]
[DepthWriteEnable]
[CullMode(CullMode.Back)]
float4 VSMain(VertexIn v, uint instanceID : SV_InstanceID) : SV_Position {
    InstanceData instance = Instances[instanceID];
    return mul(mul(float4(v.position, 1.0), instance.Transform), LightViewProjection);
}
