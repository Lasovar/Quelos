struct OutlineSettings {
    float4 Color;
    float Thickness;
    float2 ViewportSize;
};

ConstantBuffer<OutlineSettings> Settings;

Texture2D<float> SelectionMask;
SamplerState SelectionMask_sampler;

[shader("vertex")]
float4 vertexMain(uint id: SV_VertexID) : SV_Position {
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * 2.0 - 1.0, 0.0, 1.0);
}

[shader("fragment")]
float4 fragmentMain(float4 pos: SV_Position) : SV_Target {
    uint width;
    uint height;

    SelectionMask.GetDimensions(width, height);

    float2 texelSize = 1.0 / float2(width, height);

    // Center of current pixel
    float2 uv = (pos.xy + 0.5) / float2(width, height);

    float center = SelectionMask.Sample(SelectionMask_sampler, uv).r;

    float maxNeighbor = 0.0;

    [unroll]
    for (int y = -1; y <= 1; y++) {
        [unroll]
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0) {
                continue; 
            }

            float2 offset = float2(x, y) * texelSize * Settings.Thickness;
            maxNeighbor = max(
                maxNeighbor,
                SelectionMask.Sample(
                                 SelectionMask_sampler,
                                 uv + offset
                ).r
            );
        }
    }

    // Outline = neighbor is selected but current pixel is not
    float outline = saturate(maxNeighbor - center);

    if (outline < 0.01)
        discard;

    return float4(Settings.Color.rgb, outline);
}
