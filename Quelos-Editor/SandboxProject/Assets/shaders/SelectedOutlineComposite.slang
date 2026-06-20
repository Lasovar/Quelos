struct OutlineSettings {
    float4 Color;
    float4 ViewportAndThickness; // xy = viewport size, z = thickness
};

ConstantBuffer<OutlineSettings> Settings;

Texture2D<float> FullMask;
SamplerState FullMask_sampler;

Texture2D<float> VisibleMask;
SamplerState VisibleMask_sampler;

[shader("vertex")]
float4 vertexMain(uint id: SV_VertexID) : SV_Position {
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * 2.0 - 1.0, 0.0, 1.0);
}

[shader("fragment")]
float4 fragmentMain(float4 pos: SV_Position) : SV_Target {
    uint width, height;
    FullMask.GetDimensions(width, height);
    float2 texelSize = 1.0 / float2(width, height);
    float2 uv = (pos.xy + 0.5) / float2(width, height);

    float thickness = Settings.ViewportAndThickness.z;
    float center = FullMask.Sample(FullMask_sampler, uv).r;

    const int kSamples = 16;
    float maxNeighbor = 0.0;
    float maxVisibleNeighbor = 0.0;

    [unroll]
    for (int i = 0; i < kSamples; i++) {
        float angle = (float(i) / float(kSamples)) * 6.28318530718; // 2*PI
        float2 dir = float2(cos(angle), sin(angle));
        float2 sampleUV = uv + dir * thickness * texelSize;

        maxNeighbor = max(maxNeighbor, FullMask.Sample(FullMask_sampler, sampleUV).r);
        maxVisibleNeighbor = max(maxVisibleNeighbor, VisibleMask.Sample(VisibleMask_sampler, sampleUV).r);
    }

    float outline = saturate(maxNeighbor - center) * 2.0;
    if (outline < 0.01) discard;

    float alpha = lerp(0.3, 1.0, maxVisibleNeighbor);
    return float4(Settings.Color.rgb, outline * alpha);
}
