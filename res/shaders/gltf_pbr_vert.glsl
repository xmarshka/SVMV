#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require

layout(set = 0, binding = 0) uniform CameraMatrices {
    mat4 view_mat;
    mat4 view_proj_mat;
    vec4 ws_pos;
} cam_mats_buf;

layout(set = 1, binding = 0) uniform LightParameters {
    vec4 ws_pos_0;
    vec4 flux_0;
    vec4 ws_pos_1;
    vec4 flux_1;
    vec4 ws_pos_2;
    vec4 flux_2;
    vec4 ambient;
} light_params_buf;

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

layout(location = 0) out vec4 out_col_0;
layout(location = 1) out vec2 out_uv_0;
layout(location = 2) out vec3 out_ts_Ng;

layout(location = 3) out vec3 out_ts_P;
layout(location = 4) out vec3 out_ts_cam_pos;
layout(location = 5) out vec3 out_ts_light_pos_0;
layout(location = 6) out vec3 out_ts_light_pos_1;
layout(location = 7) out vec3 out_ts_light_pos_2;

void main() {
    vec3 ms_P = vec3(pc.P_buf.data[gl_VertexIndex * 3 + 0], pc.P_buf.data[gl_VertexIndex * 3 + 1], pc.P_buf.data[gl_VertexIndex * 3 + 2]);

    gl_Position = cam_mats_buf.view_proj_mat * pc.model_mat_buf.data[0] * vec4(ms_P, 1.0);

    out_uv_0 = vec2(pc.uv0_buf.data[gl_VertexIndex * 2], pc.uv0_buf.data[gl_VertexIndex * 2 + 1]);

    out_ts_Ng = vec3(0.0, 0.0, 0.0);

    mat3 normal_mat = mat3(pc.normal_mat_buf.data[0]);

    vec3 ws_Ng = normalize(normal_mat * vec3(pc.N_buf.data[gl_VertexIndex * 3 + 0], pc.N_buf.data[gl_VertexIndex * 3 + 1], pc.N_buf.data[gl_VertexIndex * 3 + 2]));

    vec3 ws_T = normalize(vec3(normal_mat * vec3(pc.T_buf.data[gl_VertexIndex * 4 + 0], pc.T_buf.data[gl_VertexIndex * 4 + 1], pc.T_buf.data[gl_VertexIndex * 4 + 2])));

    vec3 ws_B = normalize(cross(ws_T, ws_Ng) * pc.T_buf.data[gl_VertexIndex * 4 + 3]);

    mat3 ts_mat = transpose(mat3(ws_T, ws_B, ws_Ng));

    out_ts_Ng = ts_mat * ws_Ng;
    out_ts_P = ts_mat * vec3(pc.model_mat_buf.data[0] * vec4(ms_P, 1.0));
    out_ts_cam_pos = ts_mat * cam_mats_buf.ws_pos.xyz;
    out_ts_light_pos_0 = ts_mat * light_params_buf.ws_pos_0.xyz;
    out_ts_light_pos_1 = ts_mat * light_params_buf.ws_pos_1.xyz;
    out_ts_light_pos_2 = ts_mat * light_params_buf.ws_pos_2.xyz;

    out_col_0 = vec4(1.0, 1.0, 1.0, 1.0);

    if (uvec2(pc.col0_buf) != uvec2(0)) {
        out_col_0 = vec4(pc.col0_buf.data[gl_VertexIndex * 4], pc.col0_buf.data[gl_VertexIndex * 4 + 1], pc.col0_buf.data[gl_VertexIndex * 4 + 2],  pc.col0_buf.data[gl_VertexIndex * 4 + 3]);
    }
}
