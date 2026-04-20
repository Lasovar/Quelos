$input v_normal, v_uv

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

void main() {
    vec3 N = normalize(v_normal);
    gl_FragColor = vec4(N * 0.5 + 0.5, 1.0);
    gl_FragColor = vec4(1.0);
}

