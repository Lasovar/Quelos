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
    float2 uv = pos.xy * texelSize;

    float thickness = Settings.ViewportAndThickness.z;
    float center = FullMask.Sample(FullMask_sampler, uv).r;

    const int k_Directions = 16;
    const int k_MaxSteps = 8;
    int steps = clamp(int(ceil(thickness)), 1, k_MaxSteps); // apprx. 1px per ring
    float stepSize = thickness / float(steps);

    float maxFull = 0.0;
    float nearestVisible = 0.0;
    bool found = false;

    for (int step = 1; step <= steps; step++) {
        float r = step * stepSize;
        float ringFull = 0.0;
        float ringVisible = 0.0;

        [unroll]
        for (int i = 0; i < k_Directions; i++) {
            float angle = (float(i) / float(k_Directions)) * 6.28318530718;
            float2 dir = float2(cos(angle), sin(angle));
            float2 sampleUV = uv + dir * thickness * texelSize;
            float full =  FullMask.Sample(FullMask_sampler, sampleUV).r;
            float visible = VisibleMask.Sample(VisibleMask_sampler, sampleUV).r;
            ringFull += full;
            ringVisible += visible;
            maxFull = max(maxFull, full);
        }

        if (!found && ringFull > 0.25) {
            nearestVisible = saturate(ringVisible / ringFull);
            found = true;
        }
    }

    float outline = saturate((maxFull - center) * 2.0);
    if (outline < 0.01) {
        discard;
    }

    float alpha = lerp(0.3, 1.0, nearestVisible);
    return float4(Settings.Color.rgb, saturate(outline * alpha));
}
