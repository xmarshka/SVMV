#version 450

#define PI 3.1415926538

layout(set = 1, binding = 0) uniform LightParameters {
    vec4 ws_pos_0;
    vec4 flux_0;
    vec4 ws_pos_1;
    vec4 flux_1;
    vec4 ws_pos_2;
    vec4 flux_2;
    vec4 ambient;
} light_params_buf;

layout(set = 2, binding = 0) uniform MaterialUniformParameters {
    vec4 base_col_factor;
    vec4 roughness_metallic_normal_factor;
    vec4 emissive_factor;
} mat_param_buf;

layout(set = 2, binding = 1) uniform sampler2D base_col_tx;
layout(set = 2, binding = 2) uniform sampler2D normal_tx;
layout(set = 2, binding = 3) uniform sampler2D metal_rough_tx;
layout(set = 2, binding = 4) uniform sampler2D occlusion_tx;
layout(set = 2, binding = 5) uniform sampler2D emissive_tx;

layout(location = 0) in vec4 col_0;
layout(location = 1) in vec2 uv_0;
layout(location = 2) in vec3 ts_Ng;
layout(location = 3) in vec3 ts_P;
layout(location = 4) in vec3 ts_cam_pos;
layout(location = 5) in vec3 ts_light_pos_0;
layout(location = 6) in vec3 ts_light_pos_1;
layout(location = 7) in vec3 ts_light_pos_2;

layout(location = 0) out vec4 out_col;

float d_ggx(in vec3 N, in vec3 H, in float alpha) {
    float alpha_sq = alpha * alpha;
    float dot_N_H = max(dot(N, H), 0.0);
    float dot_N_H_sq = dot_N_H * dot_N_H;

    float numerator = alpha_sq;
    float denominator = PI * (dot_N_H_sq * (alpha_sq - 1.0) + 1.0) * (dot_N_H_sq * (alpha_sq - 1.0) + 1.0);

    return numerator / denominator;
}

float g_smith_ggx(in vec3 A, in vec3 N, in vec3 H, in float alpha) {
    float alpha_sq = alpha * alpha;
    float dot_N_A = max(dot(N, A), 0.0);

    float numerator = 2.0 * dot_N_A;
    float denominator = dot_N_A + sqrt(alpha_sq + (1.0 - alpha_sq) * dot_N_A * dot_N_A);

    return numerator / denominator;
}

float g_smith_ggx_ms(in vec3 N, in vec3 V, in vec3 L, in vec3 H, in float alpha) {
    return g_smith_ggx(V, N, H, alpha) * g_smith_ggx(L, N, H, alpha);
}

float f_schlick_dielectric(in vec3 H, in vec3 V, in float f_0) {
    return f_0 + (1.0 - f_0) * pow(1.0 - max(dot(V, H), 0.0), 5);
}

vec3 f_schlick_metal(in vec3 H, in vec3 V, in vec3 f_0) {
    return f_0 + (1.0 - f_0) * pow(1.0 - max(dot(V, H), 0.0), 5);
}

vec3 brdf_s_cook_torrance(in vec3 L, in vec3 V, in vec3 N, in vec3 H, in float alpha) {
    return vec3(1.0, 1.0, 1.0) * ((d_ggx(N, H, alpha) * g_smith_ggx_ms(N, V, L, H, alpha)) / (4.0 * max(dot(N, L), 0.0) * max(dot(N, V), 0.0) + 0.0001));
}

vec3 brdf_d_lambert(in vec3 base_col) {
    return base_col / PI;
}

void main() {
    vec3 ts_N = vec3(0.0);

    if (mat_param_buf.roughness_metallic_normal_factor.r == 0.0)
    {
        ts_N = normalize(texture(normal_tx, uv_0).rgb * 2.0 - 1.0);
    }
    else
    {
        ts_N = ts_Ng;
    }

    vec3 xrm = texture(metal_rough_tx, uv_0).rgb;
    float occlusion_factor = texture(occlusion_tx, uv_0).r;
    vec3 emissive_col = texture(emissive_tx, uv_0).rgb * mat_param_buf.emissive_factor.rgb;

    vec3 light_col_0 = light_params_buf.flux_0.rgb * light_params_buf.flux_0.w;
    float light_distance_0 = length(ts_light_pos_0 - ts_P);
    vec3 radiance_0 = light_col_0 / (light_distance_0 * light_distance_0 + 0.001);

    vec3 light_col_1 = light_params_buf.flux_1.rgb * light_params_buf.flux_1.w;
    float light_distance_1 = length(ts_light_pos_1 - ts_P);
    vec3 radiance_1 = light_col_1 / (light_distance_1 * light_distance_1 + 0.001);

    vec3 light_col_2 = light_params_buf.flux_2.rgb * light_params_buf.flux_2.w;
    float light_distance_2 = length(ts_light_pos_2 - ts_P);
    vec3 radiance_2 = light_col_2 / (light_distance_2 * light_distance_2 + 0.001);

    vec3 ts_V = normalize(ts_cam_pos - ts_P);

    vec3 ts_L_0 = normalize(ts_light_pos_0 - ts_P);
    vec3 ts_H_0 = normalize(ts_L_0 + ts_V);

    vec3 ts_L_1 = normalize(ts_light_pos_1 - ts_P);
    vec3 ts_H_1 = normalize(ts_L_1 + ts_V);

    vec3 ts_L_2 = normalize(ts_light_pos_2 - ts_P);
    vec3 ts_H_2 = normalize(ts_L_2 + ts_V);

    float roughness = xrm.g * mat_param_buf.roughness_metallic_normal_factor.g;
    float metalness = xrm.b * mat_param_buf.roughness_metallic_normal_factor.b;
    vec3 base_col = vec3(texture(base_col_tx, uv_0)) * mat_param_buf.base_col_factor.rgb;

    vec3 ambient_factor = light_params_buf.ambient.rgb * light_params_buf.ambient.w * (occlusion_factor);

    vec3 diffuse = brdf_d_lambert(base_col);

    vec3 specular_0 = brdf_s_cook_torrance(ts_L_0, ts_V, ts_N, ts_H_0, roughness * roughness);
    vec3 specular_1 = brdf_s_cook_torrance(ts_L_1, ts_V, ts_N, ts_H_1, roughness * roughness);
    vec3 specular_2 = brdf_s_cook_torrance(ts_L_2, ts_V, ts_N, ts_H_2, roughness * roughness);

    float f_s_dielectric_0 = f_schlick_dielectric(ts_H_0, ts_V, 0.04);
    float f_d_dielectric_0 = 1 - f_s_dielectric_0;
    vec3 f_s_metallic_0 = f_schlick_metal(ts_H_0, ts_V, base_col);

    float f_s_dielectric_1 = f_schlick_dielectric(ts_H_1, ts_V, 0.04);
    float f_d_dielectric_1 = 1 - f_s_dielectric_1;
    vec3 f_s_metallic_1 = f_schlick_metal(ts_H_1, ts_V, base_col);

    float f_s_dielectric_2 = f_schlick_dielectric(ts_H_2, ts_V, 0.04);
    float f_d_dielectric_2 = 1 - f_s_dielectric_2;
    vec3 f_s_metallic_2 = f_schlick_metal(ts_H_2, ts_V, base_col);

    vec3 col_dielectric_0 = f_d_dielectric_0 * diffuse + f_s_dielectric_0 * specular_0;
    vec3 col_metallic_0 = f_s_metallic_0 * specular_0;

    vec3 col_dielectric_1 = f_d_dielectric_1 * diffuse + f_s_dielectric_1 * specular_1;
    vec3 col_metallic_1 = f_s_metallic_1 * specular_1;

    vec3 col_dielectric_2 = f_d_dielectric_2 * diffuse + f_s_dielectric_2 * specular_2;
    vec3 col_metallic_2 = f_s_metallic_2 * specular_2;

    vec3 final_col_0 = mix(col_dielectric_0, col_metallic_0, metalness);
    vec3 final_col_1 = mix(col_dielectric_1, col_metallic_1, metalness);
    vec3 final_col_2 = mix(col_dielectric_2, col_metallic_2, metalness);

    out_col = vec4(ambient_factor * base_col, 1.0);
    out_col = out_col + vec4(final_col_0 * radiance_0 * max(dot(ts_N, ts_L_0), 0.0), 0.0);
    out_col = out_col + vec4(final_col_1 * radiance_1 * max(dot(ts_N, ts_L_1), 0.0), 0.0);
    out_col = out_col + vec4(final_col_2 * radiance_2 * max(dot(ts_N, ts_L_2), 0.0), 0.0);
    out_col = max(out_col, vec4(emissive_col, 1.0));
    out_col = out_col * col_0;
}
