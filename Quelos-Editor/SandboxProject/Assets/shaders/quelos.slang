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
    uint instanceID : SV_InstanceID;
};

[__AttributeUsage(_AttributeTargets.Function)]
struct PassAttribute {
    string Name;
}

// Allows you to indicate that a struct is material
// Allows the reflection system to properly bind material data with the shader
// And show proper UI in the editor for editing the material fields
// usage: [Material] struct { float _Value; };
[__AttributeUsage(_AttributeTargets.Struct)]
struct MaterialAttribute {}

// Allows you to indicate that a vector is a color
// Allows the reflection system to show proper color pickers in the editor for the field
// usage: [Color] float4 _AlbedoColor;
[__AttributeUsage(_AttributeTargets.Var)]
struct ColorAttribute {}

// Allows you to indicate that a uint is a texture 2D
// Allows the reflection system to show proper asset pickers in the editor for the field
// usage: [Texture2D] uint _AlbedoTexture;
[__AttributeUsage(_AttributeTargets.Var)]
struct Texture2DAttribute {}

__extern_cpp struct GlobalUniforms {
    float4x4 viewProj;
    float4 LightDirection;
};

// Data accessed per instance from the Instances StructuredBuffer with SV_InstanceID
// MaterialID is accessed through MaterialID.x
// The other elements are for padding purposes and can be used for other data if needed
__extern_cpp struct InstanceData {
    float4x4 Transform;
    uint MaterialID;
    uint EntityID;
    float2 Padding;
};

__extern_cpp ConstantBuffer<GlobalUniforms> global;
__extern_cpp StructuredBuffer<InstanceData> Instances;

static const uint k_MaxTextures = 128;
static const uint k_MaxSamplers = 4;

__extern_cpp Texture2D g_Textures[k_MaxTextures];
__extern_cpp SamplerState g_Textures_sampler;
