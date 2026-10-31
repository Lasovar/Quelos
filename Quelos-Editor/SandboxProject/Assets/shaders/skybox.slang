import quelos;

__extern_cpp TextureCube<float4> _SkyTexture;
__extern_cpp SamplerState _SkyTexture_sampler;

static const float k_NearDepth = 1.0;
static const float k_FarDepth  = 0.0;

struct VSOut {
    float4 Position : SV_Position;
    float2 Ndc : TEXCOORD0;
};

[shader("vertex")]
[Pass("Skybox")]
VSOut VSMain(uint vertexID : SV_VertexID) {
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    VSOut o;
    o.Ndc = uv * 2.0 - 1.0;
    o.Position = float4(o.Ndc, k_FarDepth, 1.0); // sit exactly on the far plane
    return o;
}

[shader("fragment")]
[Pass("Skybox")]
float4 FSMain(VSOut i) : SV_Target {
    // Any point along the pixel's ray gives the same direction; the near plane is always finite
    float4 p = mul(float4(i.Ndc, k_NearDepth, 1.0), global.InvViewProj);
    float3 dir = normalize(p.xyz / p.w - global.CameraPositionAndEnvIntensity.xyz);

    float3 sky = _SkyTexture.SampleLevel(_SkyTexture_sampler, dir, 0).rgb;
    return float4(sky * global.CameraPositionAndEnvIntensity.w, 1.0);
}
