import quelos;

// Stephen Hill's ACES fit (sRGB -> AP1 -> RRT+ODT -> sRGB). Column-vector mul on purpose:
static const float3x3 ACESInput = float3x3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

static const float3x3 ACESOutput = float3x3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

float3 RRTAndODTFit(float3 v) {
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

float3 ACESFitted(float3 c) {
    c = mul(ACESInput, c);
    c = RRTAndODTFit(c);
    return saturate(mul(ACESOutput, c));
}

float3 PBRNeutral(float3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation     = 0.15;

    float x      = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d  = 1.0 - startCompression;
    float newPeak  = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return lerp(color, newPeak.xxx, g);
}

// c already saturated, so no pow() on negatives
float3 LinearToSRGB(float3 c) {
    return select(c <= 0.0031308, c * 12.92, 1.055 * pow(c, 1.0 / 2.4) - 0.055);
}

[SceneColor] Texture2D<float4> _SceneColor;

[shader("vertex")]
[Pass("ToneMapping")]
float4 VSMain(uint vertexID: SV_VertexID) : SV_Position {
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    return float4(uv * 2.0 - 1.0, 0.0, 1.0);
}

[shader("fragment")]
[Pass("ToneMapping")]
float4 FSMain(float4 svPos : SV_Position) : SV_Target {
    float3 hdr = _SceneColor.Load(int3(int2(svPos.xy), 0)).rgb;
    return float4(LinearToSRGB(PBRNeutral(hdr)), 1.0);
}
