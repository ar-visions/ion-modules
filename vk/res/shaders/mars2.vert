#version 450

struct Light {
    vec4  pos_rads;
};

struct Material {
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    vec4  shininess;
};

struct MVP {
    mat4  model;
    mat4  view;
    mat4  proj;
};

layout(binding = 0) uniform UniformBufferObject {
    MVP      mvp;
    Material mat;
    vec4     displace;
    Light    lights[2];
} ubo;

layout(binding  =    COLOR) uniform  sampler2D  tx_color;
layout(binding  =     MASK) uniform  sampler2D  tx_mask;
layout(binding  =   NORMAL) uniform  sampler2D  tx_normal;
layout(binding  =    SHINE) uniform  sampler2D  tx_shine;
layout(binding  =    ROUGH) uniform  sampler2D  tx_rough;
layout(binding  = DISPLACE) uniform  sampler2D  tx_displace;

layout(location =        0) in       vec3       a_position;
layout(location =        1) in       vec3       a_normal;
layout(location =        2) in       vec2       a_uv;
layout(location =        3) in       vec4       a_color;
layout(location =        4) in       vec3       a_right;
layout(location =        5) in       vec3       a_up;

layout(location =        0) out      vec4       v_color;
layout(location =        1) out      vec3       v_normal;
layout(location =        2) out      vec2       v_uv;

/// dont use.
void main() {
    mat4   vm = ubo.mvp.view * ubo.mvp.model;
    vec4 wpos = vm * vec4(a_position, 1.0);
    
    v_color       = vec4(1.0); //ubo.mat.ambient + a_color;
    v_normal      = (vm * vec4(a_normal, 0.0)).xyz;
    v_uv          = a_uv;
    
    gl_Position   = ubo.mvp.proj * wpos;
}
