#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require

layout(location = 0) out vec4 outColor;

layout(buffer_reference, std430) readonly buffer PositionsBuffer {
    vec3 data[];
};

layout(buffer_reference, std430) readonly buffer NormalsBuffer {
    vec3 data[];
};

layout(buffer_reference, std430) readonly buffer TangentsBuffer {
    vec4 data[];
};

layout(buffer_reference, std430) readonly buffer Texcoords_0Buffer {
    vec2 data[];
};

layout(buffer_reference, std430) readonly buffer Colors_0Buffer {
    vec4 data[];
};

layout(push_constant) uniform constants {
    mat4 mvp;
    PositionsBuffer positions;
    NormalsBuffer normals;
    TangentsBuffer tangents;
    Texcoords_0Buffer texcoords_0;
    Colors_0Buffer colors_0;
} PushConstants;

void main() {
    vec3 position = PushConstants.positions.data[gl_VertexIndex];

    gl_Position = PushConstants.mvp * vec4(position, 1.0);
    gl_Position = gl_Position * vec4(1.0, -1.0, 1.0, 1.0);

    outColor = vec4(0.8, 0.7, 0.6, 1.0);
    uvec2 colorsAddress = uvec2(PushConstants.colors_0);
    if (colorsAddress != uvec2(0))
    {
        outColor = PushConstants.colors_0.data[gl_VertexIndex];
    }

}
