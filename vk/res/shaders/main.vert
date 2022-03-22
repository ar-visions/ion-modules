#version 450

///
struct Light {
    vec4  pos_rads;
};

///
struct Material {
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    vec4  shininess;
};

///
struct MVP {
    mat4  model;
    mat4  view;
    mat4  proj;
};

///
layout(binding = 0) uniform UniformBufferObject {
    MVP      mvp;
    Material mat;
    vec4     displace;
    Light    lights[2];
} ubo;

/// no-op; the vector of texture just cant do for this, it must be a map
# pragma opt COLOR
layout(binding  = 1) uniform  sampler2D  tx_color;

/// this makes this a build-var (these are multi-compiled for the optimizers to do their thing)
# pragma opt DISPLACE
layout(binding  = 2) uniform  sampler2D  tx_displace;

///
# pragma opt NORMAL
layout(binding  = 3) uniform  sampler2D  tx_normal;

///
layout(location = 0) in       vec3       a_position;
layout(location = 1) in       vec3       a_normal;
layout(location = 2) in       vec2       a_uv;
layout(location = 3) in       vec4       a_color;
layout(location = 4) in       vec3       a_tangent;
layout(location = 5) in       vec3       a_bitangent;

///
layout(location = 0) out      vec4       v_color;
layout(location = 1) out      vec3       v_normal;
layout(location = 2) out      vec2       v_uv;

void main() {
    mat4       vm = ubo.mvp.view * ubo.mvp.model;
    
    #if DISPLACE
        vec4   dv = texture(tx_displace, a_uv);
        vec4 wpos = vm * vec4(a_position + a_normal * (dv.xyz - 0.5) * ubo.displace.x, 1.0);
    #else
        vec4 wpos = vm * vec4(a_position, 1.0);
    #endif
    
    v_color      = vec4(1.0); //ubo.mat.ambient + a_color;
    v_normal     = (vm * vec4(a_normal, 0.0)).xyz;
    v_uv         = a_uv;
    
    gl_Position  = ubo.mvp.proj * wpos;
}
