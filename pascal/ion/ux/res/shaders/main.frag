#version 450

///
layout(binding  = 1) uniform  sampler2D  tx_color;
layout(binding  = 2) uniform  sampler2D  tx_elevation;
layout(binding  = 3) uniform  sampler2D  tx_normal;

///
layout(location = 0) in       vec4       v_color;
layout(location = 1) in       vec3       v_normal;
layout(location = 2) in       vec2       v_uv;

///
layout(location = 0) out      vec4       result;

///
void main() {
    vec4            s = texture(tx_color,     v_uv);
    float           e = texture(tx_elevation, v_uv).x;
    const float start = 0.25;
    const float shore = 0.15;
    ///
    /// if under start of surface
    if (e < start) {
        float w = 1.0;
        if (e > start - shore) {
            w = 1.0 - clamp((e - (start - shore)) / shore, 0.0, 1.0);
            result = vec4(vec3(w), 1.0);
        } else {
            w = 1.0;
            result = vec4(vec3(0.2, 0.0, 0.4) * w, 1.0);
        }
    } else
        result   = v_color;//vec4(s.xyz, 1.0);
}
