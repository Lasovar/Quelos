import quelos;

[shader("vertex")]
[Pass("SelectedOutlineMask")]
float4 vertexMain(VertexIn input, uint instanceID: SV_InstanceID) : SV_Position {
    float4 worldPos = float4(input.position, 1.0);

    InstanceData instance = Instances[instanceID];
    return mul(mul(worldPos, instance.Transform), global.viewProj);
}

[shader("fragment")]
[Pass("SelectedOutlineMask")]
float fragmentMain(float4 pos: SV_Position) : SV_Target{
    return 1.0;
}
