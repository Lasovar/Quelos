import quelos;

struct EditorVSOut {
    float4 pos : SV_Position;
    uint entityId : TEXCOORD0;
};

[shader("vertex")]
[Pass("EditorEntityID")]
EditorVSOut vertexMain(VertexIn input, uint instanceID: SV_InstanceID) {
    InstanceData inst = Instances[instanceID];

    EditorVSOut o;
    o.pos = mul(mul(float4(input.position, 1.0), inst.Transform), global.viewProj);
    o.entityId = inst.EntityID;

    return o;
}

[shader("fragment")]
[Pass("EditorEntityID")]
uint fragmentMain(EditorVSOut input) : SV_Target {
    return input.entityId;
}
