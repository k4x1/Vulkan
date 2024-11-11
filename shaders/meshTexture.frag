#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D texSampler;

layout (location = 0) in vec3 fragPosition;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(texSampler, fragTexCoord).rgb;
    // Basic lighting could be added here
    outColor = vec4(color, 1.0);
}
