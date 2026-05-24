struct GlobalUniforms {
    float4x4 viewProj;
};

struct InstanceData {
    float4x4 Transform;
    uint MaterialID;
    uint3 Padding;
}

ConstantBuffer<GlobalUniforms> global;
StructuredBuffer<InstanceData> Instances;

struct VertexIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texcoord0 : TEXCOORD0;
};

struct VertexOut {
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

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

    float3 N = normalize(v.normal);
    output.color = float4(N * 0.5 + 0.5, 1.0);

    return output;
}
