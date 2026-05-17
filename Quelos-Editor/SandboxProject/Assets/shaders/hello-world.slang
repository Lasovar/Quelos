import bgfx;

struct VertexIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord0 : TEXCOORD0;
};

struct VertexOut {
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOut vertexMain(VertexIn input) {
    VertexOut output;

    float3 position = input.position;
    float3 normal = input.normal;
    float2 uv = input.texcoord0;

    float4 worldPos = mul(u_model[0], float4(position, 1.0));

    output.position = mul(u_viewProj, worldPos);
    output.normal = mul(u_model[0], float4(normal, 0.0)).xyz;
    output.uv = uv;

    return output;
}

struct FragmentOut {
    float4 color : SV_Target0;
}

uniform float4 u_Color;

[shader("fragment")]
FragmentOut fragmentMain(VertexOut v) {
    FragmentOut output;

    float3 N = normalize(v.normal);
    output.color = float4(N * 0.5 + 0.5, 1.0) * u_Color;

    return output;
}
