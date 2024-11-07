#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UBO {
        mat4 M;
        mat4 VP;
} ubo;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;


layout (location = 0) out VS_OUT {
    vec3 pos;
    vec2 texcoord; 
} vs_out;

out gl_PerVertex{
    vec4 gl_Position;
};

void main() 
{
    vs_out.pos = in_position;
    vs_out.texcoord = in_texcoord;

    gl_Position = ubo.VP * ubo.M * vec4(in_position, 1.0f);
}
