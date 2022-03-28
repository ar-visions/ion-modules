#version 450

/// uniforms
layout(binding   =    COLOR) uniform  sampler2D  tx_color;
layout(binding   =     MASK) uniform  sampler2D  tx_mask;
layout(binding   =   NORMAL) uniform  sampler2D  tx_normal;
layout(binding   =    SHINE) uniform  sampler2D  tx_shine;
layout(binding   =    ROUGH) uniform  sampler2D  tx_rough;
layout(binding   = DISPLACE) uniform  sampler2D  tx_displace;

/// varying
layout(location  =        0) in       vec4       v_color;
layout(location  =        1) in       vec3       v_normal;
layout(location  =        2) in       vec2       v_uv;

/// resultant pixel value
layout(location  =        0) out      vec4       pixel;

///
void main() {
    vec4 s = texture(tx_color, v_uv);
    pixel  = vec4(s.xyz, 1.0);
}
