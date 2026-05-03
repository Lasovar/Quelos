$input v_normal, v_uv, v_worldPosition

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

uniform vec4 u_lightDir;
uniform vec4 u_lightColor;
uniform vec4 u_cameraPos;

uniform vec4 u_bandCount;      // 3–5
uniform vec4 u_shadowThreshold;// 0.2

void main() {
    vec3 N = normalize(v_normal.xyz);
    vec3 L = normalize(u_lightDir.xyz);

    float NdotL = dot(N, L);

    // Step 1: clamp
    float diffuse = max(NdotL, 0.0);
    //float diffuse = NdotL * 0.5 + 0.5;

    // Step 2: banding
    float bands = u_bandCount.x;
    float t = diffuse * bands;
    float stepped = floor(t) / bands;

    // soften transition
    float f = fract(t);
    stepped += smoothstep(0.0, 0.2, f) / bands;

    // Step 3: hard shadow cutoff (stylized)
    float shadow = step(u_shadowThreshold.x, diffuse);

    vec3 color = u_lightColor.xyz * stepped * shadow;

    vec3 ambient = vec3(0.1);
    color += ambient;

    vec3 V = normalize(u_cameraPos.xyz - v_worldPosition);
    //vec3 H = normalize(L + V);
    
    //float spec = pow(max(dot(N, H), 0.0), 64.0);
    
    //// Hard cutoff
    //spec = step(0.5, spec);
    
    //color += spec * u_lightColor.xyz;

    //float rim = 1.0 - dot(N, V);
    //rim = step(0.7, rim);
    //color += rim * vec4(1.0, 1.0, 1.0, 1.0); // rimColor;

    gl_FragColor = vec4(color, 1.0);
    //gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
    //gl_FragColor = vec4(normalize(v_normal) * 0.5 + 0.5, 1.0);  
}
