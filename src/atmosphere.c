#include "private_api.h"

typedef struct atmos_fs_uniforms_t {
    mat4 inv_mat_vp;
    vec3 eye_pos;
    vec3 light_pos;
    vec3 night_color;
    float aspect;
    float offset;
} atmos_fs_uniforms_t;

typedef struct atmos_param_fs_uniforms_t {
    float intensity;
    float planet_radius;
    float atmosphere_radius;
    float mie_coef;
    float rayleigh_scale_height;
    float mie_scale_height;
    float mie_scatter_dir;
    vec3 rayleigh_coef;
} atmos_param_fs_uniforms_t;

static const char *atmosphere_f =
    SOKOL_SHADER_HEADER
    "#include \"etc/sokol/shaders/atmosphere_frag.glsl\"\n";

static
void update_atmosphere_pass(sokol_offscreen_pass_t *pass)
{
    pass->pass_action = sokol_clear_action((ecs_rgb_t){1, 1, 1}, false, false),
    pass->color_target = sokol_target_rgba16f(
        "Atmos color target", 256, 256, 1, 1);
    pass->pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = pass->color_target,
        .label = "atmosphere-pass"
    });
}

sokol_offscreen_pass_t sokol_init_atmos_pass(void)
{
    sokol_offscreen_pass_t result = {0};
    update_atmosphere_pass(&result);

    char *fs = sokol_shader_from_str(atmosphere_f);
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source = sokol_vs_passthrough(),
        .fs = {
            .source = fs,
            .uniform_blocks = {
                [0] = {
                    .size = sizeof(atmos_fs_uniforms_t),
                    .uniforms = {
                        [0] = { .name="u_mat_v", .type=SG_UNIFORMTYPE_MAT4 },
                        [1] = { .name="u_eye_pos", .type=SG_UNIFORMTYPE_FLOAT3 },
                        [2] = { .name="u_light_pos", .type=SG_UNIFORMTYPE_FLOAT3 },
                        [3] = { .name="u_night_color", .type=SG_UNIFORMTYPE_FLOAT3 },
                        [4] = { .name="u_aspect", .type=SG_UNIFORMTYPE_FLOAT },
                        [5] = { .name="u_offset", .type=SG_UNIFORMTYPE_FLOAT },
                        [6] = { .name="padding", .type=SG_UNIFORMTYPE_FLOAT },
                    }
                },
                [1] = {
                    .size = sizeof(atmos_param_fs_uniforms_t),
                    .uniforms = {
                        [0] = { .name="intensity", .type=SG_UNIFORMTYPE_FLOAT },
                        [1] = { .name="planet_radius", .type=SG_UNIFORMTYPE_FLOAT },
                        [2] = { .name="atmosphere_radius", .type=SG_UNIFORMTYPE_FLOAT },
                        [3] = { .name="mie_coef", .type=SG_UNIFORMTYPE_FLOAT },
                        [4] = { .name="rayleigh_scale_height", .type=SG_UNIFORMTYPE_FLOAT },
                        [5] = { .name="mie_scale_height", .type=SG_UNIFORMTYPE_FLOAT },
                        [6] = { .name="mie_scatter_dir", .type=SG_UNIFORMTYPE_FLOAT },
                        [7] = { .name="rayleigh_coef", .type=SG_UNIFORMTYPE_FLOAT3 },
                    }
                }
            }
        }
    });
    ecs_os_free(fs);

    /* Create pipeline that mimics the normal pipeline, but without the material
     * normals and color, and with front culling instead of back culling */
    result.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                /* Static geometry */
                [0] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT2 }
            }
        },
        .colors = {{
            .pixel_format = SG_PIXELFORMAT_RGBA16F
        }},
        .cull_mode = SG_CULLMODE_FRONT,
        .sample_count = 1
    });

    return result;
}

void sokol_run_atmos_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state) 
{
    if (!state->atmosphere) {
        ecs_err("atmosphere pass called without atmosphere parameters");
        return;
    }

    atmos_fs_uniforms_t fs_u;
    glm_mat4_copy(state->uniforms.inv_mat_v, fs_u.inv_mat_vp);
    glm_vec3_copy(state->uniforms.eye_pos, fs_u.eye_pos);
    glm_vec3_copy(state->uniforms.sun_direction, fs_u.light_pos);
    glm_vec3_copy((float*)&state->atmosphere->night_color, fs_u.night_color);
    // fs_u.night_color[0] = 0.001 / 8.0;
    // fs_u.night_color[1] = 0.008 / 8.0;
    // fs_u.night_color[2] = 0.016 / 8.0;
    fs_u.aspect = state->uniforms.aspect;
    fs_u.offset = 0.05;

    atmos_param_fs_uniforms_t fs_p_u;
    fs_p_u.intensity = state->atmosphere->intensity;
    fs_p_u.planet_radius = state->atmosphere->planet_radius;
    fs_p_u.atmosphere_radius = state->atmosphere->atmosphere_radius;
    fs_p_u.mie_coef = state->atmosphere->mie_coef;
    fs_p_u.rayleigh_scale_height = state->atmosphere->rayleigh_scale_height;
    fs_p_u.mie_scale_height = state->atmosphere->mie_scale_height;
    fs_p_u.mie_scatter_dir = state->atmosphere->mie_scatter_dir;
    glm_vec3_copy((float*)state->atmosphere->rayleigh_coef, fs_p_u.rayleigh_coef);

    sg_begin_pass(pass->pass, &pass->pass_action);
    sg_apply_pipeline(pass->pip);
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &(sg_range){&fs_u, sizeof(atmos_fs_uniforms_t)});
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 1, &(sg_range){&fs_p_u, sizeof(atmos_param_fs_uniforms_t)});

    sg_bindings bind = { .vertex_buffers = { state->resources->quad } };
    sg_apply_bindings(&bind);
    sg_draw(0, 6, 1);

    sg_end_pass();
}
