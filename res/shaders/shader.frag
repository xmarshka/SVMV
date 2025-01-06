#version 450

layout(set = 1, binding = 1) uniform sampler2D baseColorTextureSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 textureCoordinates;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(baseColorTextureSampler, textureCoordinates);
}