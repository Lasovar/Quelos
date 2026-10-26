import quelos;

__extern_cpp Texture2D _SourceTexture;
__extern_cpp SamplerState _SourceTexture_sampler;

struct VSOut {
    float2 UV : TEXCOORD0;
    float4 Position : SV_Position;
};

[shader("vertex")]
[Pass("Preview")]
VSOut VSMain(uint vertexID : SV_VertexID) {
    VSOut o;
    o.UV = float2((vertexID << 1) & 2, vertexID & 2);
    o.Position = float4(o.UV * float2(2.0) - float2(1.0), 0.0, 1.0);
    return o;
}

[shader("fragment")]
[Pass("Preview")]
float4 FSMain(VSOut input) : SV_Target {
    float4 color = _SourceTexture.SampleLevel(_SourceTexture_sampler, input.UV, 0);
    return float4(LinearToSRGB(color.rgb), color.a);
}
