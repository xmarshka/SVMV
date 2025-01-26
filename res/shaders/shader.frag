#version 450

layout(set = 1, binding = 1) uniform sampler2D baseColorTextureSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 textureCoordinates;

layout(location = 2) in vec3 fragWorldPosition;
layout(location = 3) in vec3 fragNormal;

layout(location = 4) in vec3 cameraPosition;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 lightColor = vec4(5.0f, 0.0f, 0.0f, 1.0f);
    vec4 light2Color = vec4(0.0f, 0.0f, 5.0f, 1.0f);

    vec3 lightPosition = vec3(2.0f, 1.5f, 2.0f);
    vec3 light2Position = vec3(-1.0f, -1.5f, 2.0f);

    float ambientFactor = 0.01f;
    float diffuseIntensity = 0.3f;
    float specularIntensity = 1.0f;

    vec3 normal = normalize(fragNormal);
    vec3 lightDirection = normalize(lightPosition - fragWorldPosition);
    vec3 light2Direction = normalize(light2Position - fragWorldPosition);

    float diffuseFactor = max(dot(normal, lightDirection), 0.0f) * diffuseIntensity;
    float diffuse2Factor = max(dot(normal, light2Direction), 0.0f) * diffuseIntensity;

    vec3 viewDirection = normalize(cameraPosition - fragWorldPosition);
    vec3 halfwayDirection = normalize(normalize(lightDirection) + normalize(viewDirection));
    vec3 halfway2Direction = normalize(normalize(light2Direction) + normalize(viewDirection));

    float specularFactor = (pow(max(dot(normal, halfwayDirection), 0.0f), 128) * specularIntensity) * max(sign(diffuseFactor - 0.01f), 0.0f);
    float specular2Factor = (pow(max(dot(normal, halfway2Direction), 0.0f), 128) * specularIntensity) * max(sign(diffuse2Factor - 0.01f), 0.0f);


    outColor = texture(baseColorTextureSampler, textureCoordinates) * ((lightColor * ((ambientFactor + diffuseFactor + specularFactor))) + (light2Color * ((ambientFactor + diffuse2Factor + specular2Factor))));
}