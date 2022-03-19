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
    Light    lights[2];
} ubo;

///
layout(binding  = 1) uniform  sampler2D  tx_color;
layout(binding  = 2) uniform  sampler2D  tx_elevation;
layout(binding  = 3) uniform  sampler2D  tx_normal;

///
layout(location = 0) in       vec3       a_pos;
layout(location = 1) in       vec3       a_norm;
layout(location = 2) in       vec2       a_uv;
layout(location = 3) in       vec3       a_tangent;
layout(location = 4) in       vec3       a_bitangent;
layout(location = 5) in       vec4       a_color;

///
layout(location = 0) out      vec4       v_color;
layout(location = 1) out      vec3       v_normal;
layout(location = 2) out      vec2       v_uv;

///
void main() {
    ///
    vec4 o       = texture(tx_elevation, in_uv);
    mat4 vm      = ubo.mvp.view * ubo.mvp.model;
    
    ///
    v_color      = ubo.lights[1].color; /// * in_color
    v_normal     = (vm * vec4(at_norm, 0.0)).xyz;
    v_uv         = in_uv;
    
    ///
    vec3 pos     = vm * vec4(a_pos + a_norm * (o.xyz - 0.5) * 0.02, 1.0);
    
    ///
    gl_Position  = ubo.mvp.proj * pos;
}
