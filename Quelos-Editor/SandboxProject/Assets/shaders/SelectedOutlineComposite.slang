struct OutlineSettings {
    float4 Color;
    float4 ViewportAndThickness; // xy = viewport size, z = thickness
};

__extern_cpp ConstantBuffer<OutlineSettings> Settings;

__extern_cpp Texture2D<float> FullMask;
__extern_cpp SamplerState FullMask_sampler;

__extern_cpp Texture2D<float> VisibleMask;
__extern_cpp SamplerState VisibleMask_sampler;

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

    const int k_Samples = 16;
    float maxNeighbor = 0.0;

    [unroll]
    for (int i = 0; i < k_Samples; i++) {
        float angle = (float(i) / float(k_Samples)) * 6.28318530718;
        float2 dir = float2(cos(angle), sin(angle));
        float2 sampleUV = uv + dir * thickness * texelSize;
        maxNeighbor = max(maxNeighbor, FullMask.Sample(FullMask_sampler, sampleUV).r);
    }

    float outline = saturate(maxNeighbor - center) * 2.0;
    if (outline < 0.01) {
        discard;
    }

    const int k_VisSamples = 8;
    float visibleSum = 0.0;
    float visThickness = thickness * 2.0; // wider radius softens the visible/occluded boundary

    [unroll]
    for (int i = 0; i < k_VisSamples; i++) {
        float angle = (float(i) / float(k_VisSamples)) * 6.28318530718;
        float2 dir = float2(cos(angle), sin(angle));
        float2 sampleUV = uv + dir * visThickness * texelSize;
        visibleSum += VisibleMask.Sample(VisibleMask_sampler, sampleUV).r;
    }
    visibleSum += VisibleMask.Sample(VisibleMask_sampler, uv).r; // include center
    float avgVisible = visibleSum / float(k_VisSamples + 1);

    // Smooth transition instead of hard lerp
    float alpha = smoothstep(0.0, 1.0, avgVisible);
    alpha = lerp(0.3, 1.0, alpha);

    return float4(Settings.Color.rgb, outline * alpha);
}
