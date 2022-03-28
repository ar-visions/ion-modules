#version 450

///
/// use of the pre-processor here, for permutable shaders based on use of optional attribs and assets
///

struct Light {
    vec4     pos_rads;
    vec4     intensity;
};

struct Material {
    vec4     ambient;
    vec4     diffuse;
    vec4     specular;
    vec4     shininess;
};

struct MVP {
    mat4     model;
    mat4     view;
    mat4     proj;
};

layout(binding      = 0) uniform UniformBufferObject {
    MVP      mvp;
    Material mat;
    vec4     displace;
    Light    lights[2];
} ubo;

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

#ifdef LOCATION_0
    layout(location = LOCATION_0) in       vec3       a_position;
    layout(location = LOCATION_1) in       vec3       a_normal;
    layout(location = LOCATION_2) in       vec2       a_uv;
#endif

#ifdef NORMAL
    layout(location = LOCATION_3) in       vec4       a_color;
    layout(location = LOCATION_4) in       vec3       a_right;
    layout(location = LOCATION_5) in       vec3       a_up;
#endif

layout(location     =          0) out      vec4       v_color;
layout(location     =          1) out      vec3       v_normal;
layout(location     =          2) out      vec2       v_uv;

void main() {
    mat4 vm = ubo.mvp.view * ubo.mvp.model;
    
    #ifdef DISPLACE
        /// move position along normal of sampled value from displacement map
        vec4   dv = texture(tx_displace, a_uv);
        vec4 wpos = vm * vec4(a_position + a_normal * (dv.xyz - 0.5) * ubo.displace.x, 1.0);
    #else
        vec4 wpos = vm * vec4(a_position, 1.0);
    #endif
    
    v_color       = vec4(1.0); //ubo.mat.ambient + a_color;
    v_normal      = (vm * vec4(a_normal, 0.0)).xyz;
    v_uv          = a_uv;
    
    gl_Position   = ubo.mvp.proj * wpos;
}
