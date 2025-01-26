#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require

layout(set = 0, binding = 0) uniform CameraMatrices
{
    mat4 view;
    mat4 viewProjection;
    vec4 cameraPosition;
} cameraMatrices;

layout(set = 1, binding = 0) uniform MaterialUniformParameters
{
    vec4 baseColorFactor;
    vec4 roughnessMetallicFactors;
} materialUniformParameters;

layout(buffer_reference, std430) readonly buffer ModelMatrix {
    mat4 data[];
};

layout(buffer_reference, std430) readonly buffer PositionsBuffer {
    float data[]; // vec3s as float array
};

layout(buffer_reference, std430) readonly buffer NormalsBuffer {
    float data[]; // vec3s as float array
};

layout(buffer_reference, std430) readonly buffer TangentsBuffer {
    float data[]; // vec4s as float array
};

layout(buffer_reference, std430) readonly buffer Texcoords_0Buffer {
    float data[]; // vec2s as float array
};

layout(buffer_reference, std430) readonly buffer Colors_0Buffer {
    float data[]; // vec4s as float array
};

layout(push_constant) uniform PushConstants {
    PositionsBuffer positions;
    NormalsBuffer normals;
    TangentsBuffer tangents;
    Texcoords_0Buffer texcoords_0;
    Colors_0Buffer colors_0;
    ModelMatrix modelMatrix;
} pushConstants;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTextureCoordinates;

layout(location = 2) out vec3 outWorldPosition;
layout(location = 3) out vec3 outNormal;

layout(location = 4) out vec3 outCameraPosition;

void main() {
    vec3 position = vec3(pushConstants.positions.data[gl_VertexIndex * 3 + 0], pushConstants.positions.data[gl_VertexIndex * 3 + 1], pushConstants.positions.data[gl_VertexIndex * 3 + 2]);

    gl_Position = cameraMatrices.viewProjection * pushConstants.modelMatrix.data[0] * vec4(position, 1.0);

    outColor = vec4(0.4, 0.8, 0.3, 1.0);

    uvec2 texcoords_0Address = uvec2(pushConstants.texcoords_0);
    if (texcoords_0Address != uvec2(0))
    {
        outTextureCoordinates = vec2(pushConstants.texcoords_0.data[gl_VertexIndex * 2], pushConstants.texcoords_0.data[gl_VertexIndex * 2 + 1]);
        //outTextureCoordinates.y = 1 - outTextureCoordinates.y;
    }

    outWorldPosition = vec3(pushConstants.modelMatrix.data[0] * vec4(position, 1.0));

    outNormal = vec3(0.0f, 0.0f, 0.0f);

    uvec2 normalsAddress = uvec2(pushConstants.normals);
    if (normalsAddress != uvec2(0))
    {
        outNormal = mat3(transpose(inverse(pushConstants.modelMatrix.data[0]))) * vec3(pushConstants.normals.data[gl_VertexIndex * 3 + 0], pushConstants.normals.data[gl_VertexIndex * 3 + 1], pushConstants.normals.data[gl_VertexIndex * 3 + 2]);
    }

    outCameraPosition = cameraMatrices.cameraPosition.xyz;

    // uvec2 colorsAddress = uvec2(pushConstants.colors_0);
    // if (colorsAddress != uvec2(0))
    // {
    //     outColor = vec4(pushConstants.colors_0.data[gl_VertexIndex * 4], pushConstants.positions.data[gl_VertexIndex * 4 + 1], pushConstants.positions.data[gl_VertexIndex * 4 + 2],  pushConstants.positions.data[gl_VertexIndex * 4 + 3]);
    // }
}
