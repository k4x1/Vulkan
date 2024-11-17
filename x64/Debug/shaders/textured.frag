#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 texcoord;
} ps_in;

struct PointLight {
    vec3 position;
    float intensity;
    vec3 color;
    float constant;
    float linear; 
    float quadratic;
    float paddington1;
    float paddington2;
};

layout(binding = 1) uniform sampler2D texSampler;

layout(std140, binding = 2) uniform PointLightBuffer {
    PointLight pointLight[10];
    int lightCount;
} PLB;

layout(location = 0) out vec4 outColor;
 
void main()
{
    vec3 fragPos = ps_in.pos;
    vec3 norm = normalize(ps_in.normal);
    vec3 viewDir = normalize(-fragPos);

    vec3 result = vec3(0.0);
    for(int i = 0; i < PLB.lightCount; i++) {
        vec3 lightPos = PLB.pointLight[i].position;
        vec3 lightCol = PLB.pointLight[i].color;
        float lightInt = PLB.pointLight[i].intensity;
        float constant = PLB.pointLight[i].constant;
        float linear = PLB.pointLight[i].linear;
        float quadratic = PLB.pointLight[i].quadratic;

        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

        vec3 lightDir = normalize(lightPos - fragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = lightCol * diff * lightInt;

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
        vec3 specular = lightCol * spec * lightInt;

        diffuse *= attenuation;
        specular *= attenuation;
        result += diffuse + specular;
    }

    vec3 textureColor = texture(texSampler, ps_in.texcoord).rgb;
    outColor = vec4(result * textureColor, 1.0);
}
