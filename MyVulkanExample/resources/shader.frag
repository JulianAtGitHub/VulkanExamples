#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_texcoord;

layout(location = 0) out vec4 out_color;

void main() {
    //out_color = vec4(frag_color * texture(tex_sampler, frag_texcoord).rgb, 1.0);
    out_color = vec4(texture(tex_sampler, frag_texcoord).rgb, 1.0);
}
