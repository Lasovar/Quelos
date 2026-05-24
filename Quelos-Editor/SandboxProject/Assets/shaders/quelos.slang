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
