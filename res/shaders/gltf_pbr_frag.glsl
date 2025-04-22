#version 450

#define PI 3.1415926538

layout(set = 1, binding = 0) uniform LightParameters {
    vec4 ws_pos;
    vec4 flux;
} light_params_buf;

layout(set = 2, binding = 0) uniform MaterialUniformParameters {
    vec4 baseColorFactor;
    vec4 roughnessMetallicFactors;
} mat_param_buf;

layout(set = 2, binding = 1) uniform sampler2D base_col_tx;
layout(set = 2, binding = 2) uniform sampler2D normal_tx;
layout(set = 2, binding = 3) uniform sampler2D metal_rough_tx;
layout(set = 2, binding = 4) uniform sampler2D occlusion_tx;
layout(set = 2, binding = 5) uniform sampler2D emissive_tx;

layout(location = 0) in vec4 col0;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec3 ws_Ng;
layout(location = 3) in vec3 ws_P;
layout(location = 4) in vec3 ws_cam_pos;
//layout(location = 4) in vec3 ts_cam_pos;
layout(location = 5) in vec3 ts_P;
layout(location = 6) in vec3 ts_cam_pos;
layout(location = 7) in vec3 ts_light_pos;

layout(location = 0) out vec4 out_col;

float d_trowbridge_reitz_ggx(in vec3 N, in vec3 H, in float alpha) {
    float alpha_sq = alpha * alpha;
    float dot_N_H = max(dot(N, H), 0.0);
    float dot_N_H_sq = dot_N_H * dot_N_H;

    float numerator = alpha_sq;
    float denominator = PI * (dot_N_H_sq * (alpha_sq - 1.0) + 1.0) * (dot_N_H_sq * (alpha_sq - 1.0) + 1.0);

    return numerator / denominator;
}

float g_smith_t_r(in vec3 A, in vec3 N, in vec3 H, in float alpha) {
    float alpha_sq = alpha * alpha;
    float dot_N_A = max(dot(N, A), 0.0);

    float numerator = 2.0 * dot_N_A;
    float denominator = dot_N_A + sqrt(alpha_sq + (1.0 - alpha_sq) * dot_N_A * dot_N_A);

    return numerator / denominator;
}

float g_smith_t_r_ms(in vec3 N, in vec3 V, in vec3 L, in vec3 H, in float alpha) {
    return g_smith_t_r(V, N, H, alpha) * g_smith_t_r(L, N, H, alpha);
}

float f_schlick_dielectric(in vec3 H, in vec3 V, in float f_0) {
    return f_0 + (1.0 - f_0) * pow(1.0 - max(dot(V, H), 0.0), 5);
}

vec3 f_schlick_metal(in vec3 H, in vec3 V, in vec3 f_0) {
    return f_0 + (1.0 - f_0) * pow(1.0 - max(dot(V, H), 0.0), 5);
}

vec3 brdf_s_cook_torrance(in vec3 L, in vec3 V, in vec3 N, in vec3 H, in float alpha) {
    return vec3(1.0, 1.0, 1.0) * ((d_trowbridge_reitz_ggx(N, H, alpha) * g_smith_t_r_ms(N, V, L, H, alpha)) / (4.0 * max(dot(N, L), 0.0) * max(dot(N, V), 0.0) + 0.0001));
}

vec3 brdf_d_lambert(in vec3 base_col) {
    return base_col / PI;
}

void main() {
    vec3 ts_N = normalize(texture(normal_tx, uv0).rgb * 2.0 - 1.0);
    //vec3 ts_N = ws_Ng;

    vec3 orm = texture(metal_rough_tx, uv0).rgb;

    //vec3 light_pos = vec3(2.0, 0.0, 1.0);
    vec3 light_col = light_params_buf.flux.rgb;

    float light_distance = length(ts_light_pos - ts_P);

    //vec3 ts_L = normalize(ts_light_pos);
    vec3 ts_L = normalize(ts_light_pos - ts_P);
    vec3 ts_V = normalize(ts_cam_pos - ts_P);
    vec3 ts_H = normalize(ts_L + ts_V);

    float roughness = orm.g;
    float metallicity = orm.b;
    vec3 base_col = vec3(texture(base_col_tx, uv0));

    float ambient_factor = 0.01 * (orm.r);

    vec3 radiance = light_col / (light_distance * light_distance + 0.001);

    vec3 diffuse = brdf_d_lambert(base_col);
    vec3 specular = brdf_s_cook_torrance(ts_L, ts_V, ts_N, ts_H, roughness * roughness);

    float f_s_dielectric = f_schlick_dielectric(ts_H, ts_V, 0.04);
    float f_d_dielectric = 1 - f_s_dielectric;
    vec3 f_s_metallic = f_schlick_metal(ts_H, ts_V, base_col);

    vec3 col_dielectric = f_d_dielectric * diffuse + f_s_dielectric * specular;
    vec3 col_metallic = f_s_metallic * specular;

    vec3 final_col = mix(col_dielectric, col_metallic, metallicity);

    vec3 emissive_col = vec3(0.0, 0.0, 0.0); // texture(emissive_tx, uv0).rgb;

    out_col = vec4(ambient_factor * base_col + final_col * radiance * max(dot(ts_N, ts_L), 0.0) + emissive_col, 1.0);

    //out_col = vec4(orm.b, orm.b, orm.b, 1.0);
}
