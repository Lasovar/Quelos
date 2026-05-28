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

// Allows you to indicate that a vector is a color
// Allows the reflection system to show proper color pickers in the editor for the field
// usage: [Color] float4 _AlbedoColor;
[__AttributeUsage(_AttributeTargets.Var)]
struct ColorAttribute {}

__extern_cpp struct GlobalUniforms {
    float4x4 viewProj;
};

// Data accessed per instance from the Instances StructuredBuffer with SV_InstanceID
// MaterialID is accessed through MaterialID.x
// The other elements are for padding purposes and can be used for other data if needed
__extern_cpp struct InstanceData {
    float4x4 Transform;
    uint MaterialID;
};

__extern_cpp ConstantBuffer<GlobalUniforms> global;
__extern_cpp StructuredBuffer<InstanceData> Instances;
