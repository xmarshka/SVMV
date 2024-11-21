#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require

layout(location = 0) out vec4 outColor;

struct Vertex
{
    vec3 position;
    float texcoordX;
    vec3 normal;
    float texcoordY;
    vec4 tangent;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform constants {
    mat4 mvp;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    vec3 position = PushConstants.vertexBuffer.vertices[gl_VertexIndex].position;

    gl_Position = PushConstants.mvp * vec4(position, 1.0);
    gl_Position = gl_Position * vec4(1.0, -1.0, 1.0, 1.0);

    outColor = vec4(0.8, 0.7, 0.6, 1.0);
}
