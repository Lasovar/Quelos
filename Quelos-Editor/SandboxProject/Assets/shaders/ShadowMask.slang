import quelos;

__extern_cpp Texture2DMS<float> SceneDepth;
__extern_cpp Texture2DMS SceneNormal;
__extern_cpp Texture2DArray<float> ShadowMaps;
__extern_cpp SamplerComparisonState ShadowMaps_sampler;

struct CascadeShadowData {
    row_major float4x4 LightViewProj[4];
    float4 CascadeParams[4];
    float4 SplitDepths;
    row_major float4x4 InvViewProj;
    row_major float4x4 View;
    float4 LightDirectionAndShadowMapSize; // xyz = light direction, w = shadow map size
}

__extern_cpp ConstantBuffer<CascadeShadowData> CascadeData;

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

    float2 uv = (svPos.xy) / float2(dims);
    float2 ndc = uv * float2(2, -2) + float2(-1, 1);
    float depth = SceneDepth.Load(uint2(svPos.xy), 0);

    if (depth <= 0.0) {
        return float4(1.0, 0, 0, 0);
    }

    // Reconstruct world pos
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 worldH = mul(clipPos, CascadeData.InvViewProj);
    float3 worldPos = worldH.xyz / worldH.w;

    float3 N = SceneNormal.Load(uint2(svPos.xy), 0).xyz * 2.0 - 1.0; // decode from unorm to snorm
    float3 L = normalize(-CascadeData.LightDirectionAndShadowMapSize.xyz);
    float NdotL = dot(N, L);

    if (NdotL <= 0.01) {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    float terminatorMask = smoothstep(0.0, 0.1, NdotL);

    // View-space Z for cascade selection
    float4 viewPos = mul(float4(worldPos, 1.0), CascadeData.View);
    float viewZ = viewPos.z;

    // Select cascade
    int c = 3;
    for (int i = 0; i < 4; i++) {
        if (viewZ < CascadeData.SplitDepths[i]) {
            c = i; break;
        }
    }

    float texelWorld = CascadeData.CascadeParams[c].x;
    float invRange = CascadeData.CascadeParams[c].y;

    const float NormalOffsetTexels = 1.0;
    const float DepthBiasTexels = 2.0;

    // Normal offset: move the receiver off the surface along its normal,
    // more at grazing angles where acne is worst
    float NdotLc   = saturate(NdotL);
    float sinTheta = sqrt(1.0 - NdotLc * NdotLc);
    float3 offsetPos = worldPos + N * (NormalOffsetTexels * texelWorld * sinTheta);

    float4 lsPos = mul(float4(offsetPos, 1.0), CascadeData.LightViewProj[c]);
    lsPos.xyz /= lsPos.w;
    float2 shadowUV = float2(lsPos.x * 0.5 + 0.5, 0.5 - lsPos.y * 0.5);

    // Constant depth bias: specified in texels, converted to this cascade's light-NDC depth
    float depthBias = DepthBiasTexels * texelWorld * invRange;
    float shadow = ShadowMaps.SampleCmpLevelZero(ShadowMaps_sampler, float3(shadowUV, float(c)), lsPos.z - depthBias);

    return float4(shadow * terminatorMask, 0, 0, 0);
}
