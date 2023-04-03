#version 450


layout(binding      =      COLOR) uniform  sampler2D  tx_color;

#ifdef NORMAL
    layout(binding  =       MASK) uniform  sampler2D  tx_mask;
    layout(binding  =     NORMAL) uniform  sampler2D  tx_normal;
#endif

#ifdef SHINE
    layout(binding  =      SHINE) uniform  sampler2D  tx_shine;
    layout(binding  =      ROUGH) uniform  sampler2D  tx_rough;
#endif

#ifdef DISPLACE
    layout(binding  =   DISPLACE) uniform  sampler2D  tx_displace;
#endif


///
layout(location = 0) in       vec4       v_color;
layout(location = 1) in       vec3       v_normal;
layout(location = 2) in       vec2       v_uv;

/// resultant pixel value
layout(location = 0) out      vec4       pixel;

///
void main() {
    pixel = texture(tx_color, v_uv);
}
