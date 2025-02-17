#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require

layout(set = 0, binding = 0) uniform CameraMatrices {
    mat4 view_mat;
    mat4 view_proj_mat;
    vec4 ws_pos;
} cam_mats;

layout(set = 1, binding = 0) uniform MaterialUniformParameters {
    vec4 baseColorFactor;
    vec4 roughnessMetallicFactors;
} mat_param_buf;

layout(buffer_reference, std430) readonly buffer PositionsBuffer { float data[]; }; // vec3s as float array
layout(buffer_reference, std430) readonly buffer NormalsBuffer { float data[]; }; // vec3s as float array
layout(buffer_reference, std430) readonly buffer TangentsBuffer { float data[]; }; // vec4s as float array
layout(buffer_reference, std430) readonly buffer Texcoords_0Buffer { float data[]; }; // vec2s as float array
layout(buffer_reference, std430) readonly buffer Colors_0Buffer { float data[]; }; // vec4s as float array
layout(buffer_reference, std430) readonly buffer ModelMatrix { mat4 data[]; };
layout(buffer_reference, std430) readonly buffer NormalMatrix { mat4 data[]; };

layout(push_constant) uniform PushConstants {
    PositionsBuffer P_buf;
    NormalsBuffer N_buf;
    TangentsBuffer T_buf;
    Texcoords_0Buffer uv0_buf;
    Colors_0Buffer col0_buf;
    ModelMatrix model_mat_buf;
    NormalMatrix normal_mat_buf;
} pc;

layout(location = 0) out vec4 out_col0;
layout(location = 1) out vec2 out_uv0;
layout(location = 2) out vec3 out_ws_P;
layout(location = 3) out vec3 out_ws_Ng;
layout(location = 4) out vec3 out_ws_cam_pos;
layout(location = 5) out mat3 out_ts_mat;

void main() {
    vec3 ls_P = vec3(pc.P_buf.data[gl_VertexIndex * 3 + 0], pc.P_buf.data[gl_VertexIndex * 3 + 1], pc.P_buf.data[gl_VertexIndex * 3 + 2]);

    gl_Position = cam_mats.view_proj_mat * pc.model_mat_buf.data[0] * vec4(ls_P, 1.0);
    out_ws_P = vec3(pc.model_mat_buf.data[0] * vec4(ls_P, 1.0));
    out_ws_cam_pos = cam_mats.ws_pos.xyz;

    if (uvec2(pc.uv0_buf) != uvec2(0)) {
        out_uv0 = vec2(pc.uv0_buf.data[gl_VertexIndex * 2], pc.uv0_buf.data[gl_VertexIndex * 2 + 1]);
    }

    out_ws_Ng = vec3(0.0, 0.0, 0.0);

    if (uvec2(pc.N_buf) != uvec2(0)){
        mat3 normal_mat = mat3(pc.normal_mat_buf.data[0]);

        vec3 ws_Ng = normalize(normal_mat * vec3(pc.N_buf.data[gl_VertexIndex * 3 + 0], pc.N_buf.data[gl_VertexIndex * 3 + 1], pc.N_buf.data[gl_VertexIndex * 3 + 2]));
        out_ws_Ng = ws_Ng;

        if (uvec2(pc.T_buf) != uvec2(0)) {
            vec3 ws_T = normalize(vec3(normal_mat * vec3(pc.T_buf.data[gl_VertexIndex * 4 + 0], pc.T_buf.data[gl_VertexIndex * 4 + 1], pc.T_buf.data[gl_VertexIndex * 4 + 2])));

            vec3 ws_B = normalize(cross(ws_T, ws_Ng) * pc.T_buf.data[gl_VertexIndex * 4 + 3]);

            out_ts_mat = mat3(ws_T, ws_B, ws_Ng);
        }
    }

    // if (uvec2(pc.col0_buf) != uvec2(0)) {
    //     out_col0 = vec4(pc.col0_buf.data[gl_VertexIndex * 4], pc.col0_buf.data[gl_VertexIndex * 4 + 1], pc.col0_buf.data[gl_VertexIndex * 4 + 2],  pc.col0_buf.data[gl_VertexIndex * 4 + 3]);
    // }
}
