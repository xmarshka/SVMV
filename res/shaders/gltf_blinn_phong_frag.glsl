#version 450

layout(set = 1, binding = 1) uniform sampler2D base_col_tx;
layout(set = 1, binding = 2) uniform sampler2D normal_tx;

layout(location = 0) in vec4 col0;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec3 ws_P;
layout(location = 3) in vec3 ws_Ng;
layout(location = 4) in vec3 ws_cam_pos;
layout(location = 5) in mat3 ts_mat;

layout(location = 0) out vec4 out_col;

void main() {
    vec3 ws_N = normalize(ts_mat * (texture(normal_tx, uv0).rgb * 2.0 - 1.0));

    vec4 light_col = vec4(4.0, 4.0, 4.0, 1.0);
    //vec4 light2_col = vec4(9.0, 9.0, 9.0, 1.0);

    vec3 light_pos = vec3(0.0, 0.0, -2.0);
    //vec3 light2_pos = vec3(0.0, 0.0, -2.0);

    float ambient_factor = 0.05;
    float diffuse_intensity = 0.9;
    float specular_intensity = 0.5;

    vec3 ws_L = normalize(light_pos - ws_P);
    //vec3 ws_L2 = normalize(light2_pos - ws_P);

    float diffuse_factor = max(dot(ws_N, ws_L), 0.0) * diffuse_intensity;
    //float diffuse2_factor = max(dot(ws_N, ws_L2), 0.0f) * diffuse_intensity;

    vec3 ws_V = normalize(ws_cam_pos - ws_P);
    vec3 ws_H = normalize(normalize(ws_L) + normalize(ws_V));
    //vec3 H2 = normalize(normalize(ws_L2) + normalize(ws_V));

    float specular_factor = (pow(max(dot(ws_N, ws_H), 0.0), 16) * specular_intensity) * max(sign(diffuse_factor - 0.01), 0.0f);
    //float specular2_factor = (pow(max(dot(ws_N, ws_H2), 0.0), 16) * specular_intensity) * max(sign(diffuse2_factor - 0.01), 0.0f);

    out_col = texture(base_col_tx, uv0) * ((light_col * ((ambient_factor + diffuse_factor + specular_factor))));
}
