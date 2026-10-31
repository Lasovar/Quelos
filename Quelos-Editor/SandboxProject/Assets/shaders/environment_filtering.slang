import quelos.ibl;

__extern_cpp Texture2D<float4> _Equirect;
__extern_cpp SamplerState _Equirect_sampler; // wrap U, clamp V
__extern_cpp RWTexture2DArray<float4> _OutCube;

float2 DirectionToEquirectUV(float3 d) {
    return float2(
        atan2(d.z, d.x) / (2.0 * float.getPi()) + 0.5,
        acos(clamp(d.y, -1.0, 1.0)) / float.getPi()
    );
}

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    uint w, h, faces;
    _OutCube.GetDimensions(w, h, faces);
    if (id.x >= w || id.y >= h) {
       return;
    }

    float2 uv = (float2(id.xy) + 0.5) / float2(w, h);
    float3 dir = normalize(CubeTexelDirection(id.z, uv));
    float3 c = _Equirect.SampleLevel(_Equirect_sampler, DirectionToEquirectUV(dir), 0).rgb;

    _OutCube[id] = float4(min(c, 65000.0), 1.0); // the sun can exceed half-float max
}
