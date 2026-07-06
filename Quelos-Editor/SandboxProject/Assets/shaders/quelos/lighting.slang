__extern_cpp Texture2D<float> ShadowMask;
__extern_cpp SamplerState ShadowMask_sampler;

float GetShadowMask(float4 position) {
    uint2 maskSize;
    ShadowMask.GetDimensions(maskSize.x, maskSize.y);

    float2 screenUV = position.xy / float2(maskSize.xy);
    return ShadowMask.Sample(ShadowMask_sampler, screenUV);
}
