#version 450

layout(set = 1, binding = 1) uniform sampler2D base_col_tx;
layout(set = 1, binding = 2) uniform sampler2D normal_tx;

layout(location = 0) in vec4 col0;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec3 ws_P;
layout(location = 3) in vec3 ws_Ng;
layout(location = 4) in vec3 ws_cam_pos;
layout(location = 5) in mat3 ts_mat;

layout(location = 0) out vec4 out_COL;

// vec3 brdf_s_cook_torrance(in vec3 L; in vec3 V; in vec3 N; in float alpha; in vec3 base_col) {

// }

// vec3 d_trowbridge_reitz(in vec3 N; in vec3 H; in float alpha) {

// }

// vec3 g_schlick_ggx(in vec3 N; in vec3 V; in float k) {

// }

// vec3 f_schlick(in vec3 H; in vec3 V; in float f_0) {

// }

// vec3 brdf_d_lambert(in vec3 base_col) {

// }

void main() {
    vec3 ws_N = normalize(ts_mat * (texture(normal_tx, uv0).rgb * 2.0 - 1.0));

    vec4 light_col = vec4(4.0, 4.0, 4.0, 1.0);
    //vec4 light2_col = vec4(9.0, 9.0, 9.0, 1.0);

    vec3 light_pos = vec3(0.0, 0.0, -2.0);
    //vec3 light2_pos = vec3(0.0, 0.0, -2.0);

    float ambient_factor = 0.05;
    float diffuse_intensity = 0.9;
    float specular_intensity = 0.5;

    vec3 L = normalize(light_pos - ws_P);
    //vec3 L2 = normalize(light2_pos - ws_P);

    float diffuse_factor = max(dot(ws_N, L), 0.0) * diffuse_intensity;
    //float diffuse2_factor = max(dot(ws_N, L2), 0.0f) * diffuse_intensity;

    vec3 V = normalize(ws_cam_pos - ws_P);
    vec3 H = normalize(normalize(L) + normalize(V));
    //vec3 H2 = normalize(normalize(L2) + normalize(V));

    float specular_factor = (pow(max(dot(ws_N, H), 0.0), 16) * specular_intensity) * max(sign(diffuse_factor - 0.01), 0.0f);
    //float specular2_factor = (pow(max(dot(ws_N, H2), 0.0), 16) * specular_intensity) * max(sign(diffuse2_factor - 0.01), 0.0f);

    out_COL = texture(base_col_tx, uv0) * ((light_col * ((ambient_factor + diffuse_factor + specular_factor))));
}