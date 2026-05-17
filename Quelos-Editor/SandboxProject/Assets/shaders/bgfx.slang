module bgfx;

public struct bgfxSampler2D {
    SamplerState samplerState;
    Texture2D textureResource;
};

float4 bgfxTexture2D(bgfxSampler2D s, float2 uv) {
    return s.textureResource.Sample(s.samplerState, uv);
}

float4 bgfxTexture2DLod(bgfxSampler2D s, float2 uv, float lod) {
    return s.textureResource.SampleLevel(s.samplerState, uv, lod);
}

static const int BGFX_CONFIG_MAX_BONES = 64;

public uniform float4 u_viewRect;
public uniform float4 u_viewTexel;

public uniform float4x4 u_view;
public uniform float4x4 u_invView;

public uniform float4x4 u_proj;
public uniform float4x4 u_invProj;

public uniform float4x4 u_viewProj;
public uniform float4x4 u_invViewProj;

public uniform float4x4 u_modelView;
public uniform float4x4 u_invModelView;

public uniform float4x4 u_modelViewProj;

public uniform float4 u_alphaRef4;

public uniform float4x4 u_model[BGFX_CONFIG_MAX_BONES];
