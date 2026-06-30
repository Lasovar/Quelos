import quelos;

__extern_cpp Texture2DMS<float> SceneDepth;
__extern_cpp Texture2DArray<float> ShadowMaps;
__extern_cpp SamplerComparisonState ShadowMaps_sampler;

struct CascadeShadowData {
    float4x4 LightViewProj[4];
    float4 SplitDepths;
    float4x4 InvViewProj;
    float4x4 View;
}

ConstantBuffer<CascadeShadowData> CascadeData;

[shader("vertex")]
[Pass("ShadowMask")]
float4 VSMain(uint vertexID: SV_VertexID) : SV_Position {
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    return float4(uv * 2.0 - 1.0, 0.0, 1.0);
}

[shader("fragment")]
[Pass("ShadowMask")]
[CullMode(CullMode.None)]
[DepthEnable(false)]
float4 PSMain(float4 svPos : SV_Position) : SV_Target {
    uint2 dims;
    uint sampleCount;
    SceneDepth.GetDimensions(dims.x, dims.y, sampleCount);

    float2 uv = svPos.xy / float2(dims);
    float2 ndc = uv * float2(2, -2) + float2(-1, 1);
    float depth = SceneDepth[uint2(svPos.xy)];

    // Reconstruct world pos
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 worldH = mul(clipPos, CascadeData.InvViewProj);
    float3 worldPos = worldH.xyz / worldH.w;

    // View-space Z for cascade selection
    float4 viewPos = mul(float4(worldPos, 1.0), CascadeData.View);
    float viewZ = viewPos.z;

    // Select cascade
    int c = 3;
    for (int i = 0; i < 4; i++) {
        if (viewZ < CascadeData.SplitDepths[i]) { c = i; break; }
    }

    // Project into light space
    float4 lsPos = mul(float4(worldPos, 1.0), CascadeData.LightViewProj[c]);
    lsPos.xyz /= lsPos.w;
    float2 shadowUV = float2(lsPos.x * 0.5 + 0.5, 0.5 - lsPos.y * 0.5);

    float shadow = ShadowMaps.SampleCmpLevelZero(ShadowMaps_sampler, float3(shadowUV, c), lsPos.z);

    // Binarize for NPR
    return float4(step(0.5, shadow), 0, 0, 0);
}
