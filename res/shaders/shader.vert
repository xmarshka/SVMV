#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require

layout(location = 0) out vec3 outColor;

layout(buffer_reference, std430) readonly buffer PositionBuffer {
    vec3 positions[];
};

layout(push_constant) uniform constants {
    mat4 mvp;
    PositionBuffer positionBuffer;
} PushConstants;

void main() {
    vec3 position = vec3(PushConstants.positionBuffer.positions[gl_VertexIndex].x, PushConstants.positionBuffer.positions[gl_VertexIndex].y, PushConstants.positionBuffer.positions[gl_VertexIndex].z);

    gl_Position = PushConstants.mvp * vec4(position, 1.0);
    gl_Position = gl_Position * vec4(1.0, -1.0, 1.0, 1.0);

    outColor = vec3(0.5, 0.4, 0.6);
}
