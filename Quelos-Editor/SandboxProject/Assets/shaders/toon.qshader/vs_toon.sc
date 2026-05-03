$input a_position, a_normal, a_texcoord0
$output v_normal, v_uv, v_worldPosition

#include <bgfx_shader.sh>

uniform mat4 u_normalMatrix;

void main() {
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    gl_Position = mul(u_viewProj, worldPos);

    v_worldPosition = worldPos.xyz;
    v_normal = mul(u_model[0], vec4(a_normal, 0.0)).xyz;

    v_uv = a_texcoord0;
}

