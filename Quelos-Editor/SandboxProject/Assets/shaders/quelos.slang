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

__extern_cpp struct GlobalUniforms {
    float4x4 viewProj;
};

__extern_cpp struct InstanceData {
    float4x4 Transform;
    uint4 MaterialIndex;
}

__extern_cpp ConstantBuffer<GlobalUniforms> global;
__extern_cpp StructuredBuffer<InstanceData> Instances;
