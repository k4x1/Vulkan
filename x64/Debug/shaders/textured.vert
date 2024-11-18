#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UBO {
    mat4 M;  // Model matrix
    mat4 VP; // View-Projection matrix
} ubo;

layout(location = 0) in vec3 in_position;  // Matches VertexStandard::position
layout(location = 1) in vec3 in_normal;    // Matches VertexStandard::normal
layout(location = 2) in vec2 in_texcoord;  // Matches VertexStandard::texCoord

layout(location = 0) out VS_OUT {
    vec3 pos;
    vec3 normal; 
    vec2 texcoord;
} vs_out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() 
{
    // Transform position to world space
    vec4 worldPosition = ubo.M * vec4(in_position, 1.0f);
    vs_out.pos = worldPosition.xyz;

    // Transform normal to world space
    vs_out.normal = mat3(transpose(inverse(ubo.M))) * in_normal;

    vs_out.texcoord = in_texcoord;

    // Calculate final position in clip space
    gl_Position = ubo.VP * worldPosition;
}
