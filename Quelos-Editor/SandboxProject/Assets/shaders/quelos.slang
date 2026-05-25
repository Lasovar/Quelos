struct VertexIn {
    float3 position : ATTRIB0;
    float3 normal : ATTRIB1;
    float3 tangent : ATTRIB2;
    float2 texcoord0 : ATTRIB3;
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

__extern_cpp ConstantBuffer<GlobalUniforms> global;
__extern_cpp StructuredBuffer<InstanceData> Instances;
