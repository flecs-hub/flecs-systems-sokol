#include "private_api.h"

typedef struct depth_vs_uniforms_t {
    mat4 mat_vp;
} depth_vs_uniforms_t;

typedef struct depth_fs_uniforms_t {
    vec3 eye_pos;
    float near_;
    float far_;
    float depth_c;
    float inv_log_far;
} depth_fs_uniforms_t;

const char* sokol_vs_depth(void) 
{
    return SOKOL_SHADER_HEADER
        "uniform mat4 u_mat_vp;\n"
        "uniform vec3 u_eye_pos;\n"
        "layout(location=0) in vec4 v_position;\n"
        "layout(location=1) in mat4 i_mat_m;\n"
        "out vec3 position;\n"
        "void main() {\n"
        "  gl_Position = u_mat_vp * i_mat_m * v_position;\n"
        "  position = gl_Position.xyz;\n"
        "}\n";
}

const char* sokol_fs_depth(void) 
{
    return SOKOL_SHADER_HEADER
        "uniform vec3 u_eye_pos;\n"
        "uniform float u_near;\n"
        "uniform float u_far;\n"
        "uniform float u_depth_c;\n"
        "uniform float u_inv_log_far;\n"
        "in vec3 position;\n"
        "out vec4 frag_color;\n"

        SOKOL_SHADER_FUNC_FLOAT_TO_RGBA

#ifdef SOKOL_LOG_DEPTH
        "vec4 depth_to_rgba(vec3 pos) {\n"
        "  vec3 pos_norm = pos / u_far;\n"
        "  float d = length(pos_norm) * u_far;\n"
        "  d = clamp(d, 0.0, u_far * 0.98);\n"
        "  d = log(u_depth_c * d + 1.0) / log(u_depth_c * u_far + 1.0);\n"
        "  return float_to_rgba(d);\n"
        "}\n"
#else
        "vec4 depth_to_rgba(vec3 pos) {\n"
        "  vec3 pos_norm = pos / u_far;\n"
        "  float d = length(pos_norm);\n"
        "  d = clamp(d, 0.0, 1.0);\n"
        "  return float_to_rgba(d);\n"
        "}\n"
#endif

        "void main() {\n"
        "  frag_color = depth_to_rgba(position);\n"
        "}\n";
}

sg_pipeline init_depth_pipeline(int32_t sample_count) {
    ecs_trace("sokol: initialize depth pipeline");

    /* create an instancing shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.uniform_blocks = {
            [0] = {
                .size = sizeof(depth_vs_uniforms_t),
                .uniforms = {
                    [0] = { .name="u_mat_vp", .type=SG_UNIFORMTYPE_MAT4 }
                },
            }
        },
        .fs.uniform_blocks = { 
            [0] = {
                .size = sizeof(depth_fs_uniforms_t),
                .uniforms = {
                    [0] = { .name="u_eye_pos", .type=SG_UNIFORMTYPE_FLOAT3 },
                    [1] = { .name="u_near", .type=SG_UNIFORMTYPE_FLOAT },
                    [2] = { .name="u_far", .type=SG_UNIFORMTYPE_FLOAT },
                    [3] = { .name="u_depth_c", .type = SG_UNIFORMTYPE_FLOAT },
                    [4] = { .name="u_inv_log_far", .type = SG_UNIFORMTYPE_FLOAT }
                },
            }            
        },
        .vs.source = sokol_vs_depth(),
        .fs.source = sokol_fs_depth()
    });

    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers = {
                [1] = { .stride = 64, .step_func=SG_VERTEXSTEP_PER_INSTANCE }
            },

            .attrs = {
                /* Static geometry */
                [0] = { .buffer_index=0, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },
         
                /* Matrix (per instance) */
                [1] = { .buffer_index=1, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT4 },
                [2] = { .buffer_index=1, .offset=16, .format=SG_VERTEXFORMAT_FLOAT4 },
                [3] = { .buffer_index=1, .offset=32, .format=SG_VERTEXFORMAT_FLOAT4 },
                [4] = { .buffer_index=1, .offset=48, .format=SG_VERTEXFORMAT_FLOAT4 }
            }
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        },
        .colors = {{
            .pixel_format = SG_PIXELFORMAT_RGBA8
        }},
        .cull_mode = SG_CULLMODE_BACK,
        .sample_count = sample_count
    });
}

sokol_offscreen_pass_t sokol_init_depth_pass(
    int32_t w, 
    int32_t h,
    sg_image depth_target,
    int32_t sample_count) 
{
    ecs_trace("sokol: initialize depth pass");

    sg_image color_target = sokol_target_rgba8(
        "Depth color target", w, h, sample_count);
    ecs_rgb_t background_color = {1, 1, 1};

    sokol_offscreen_pass_t result = {
        .pass_action = sokol_clear_action(background_color, true, true),
        .pass = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0].image = color_target,
            .depth_stencil_attachment.image = depth_target
        }),
        .pip = init_depth_pipeline(sample_count),
        .color_target = color_target,
        .depth_target = depth_target
    };

    ecs_trace("sokol: depth initialized");
    return result;
}

void sokol_update_depth_pass(
    sokol_offscreen_pass_t *pass,
    int32_t w,
    int32_t h,
    sg_image depth_target,
    int32_t sample_count)
{
    sg_destroy_image(pass->color_target);
    sg_destroy_pass(pass->pass);

    pass->color_target = sokol_target_rgba8(
        "Depth color target", w, h, sample_count);

    pass->pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = pass->color_target,
        .depth_stencil_attachment.image = depth_target
    });
}

static
void depth_draw_instances(
    SokolGeometry *geometry,
    sokol_geometry_buffers_t *buffers)
{
    if (!buffers->instance_count) {
        return;
    }

    sg_bindings bind = {
        .vertex_buffers = {
            [0] = geometry->vertices,
            [1] = buffers->transforms
        },
        .index_buffer = geometry->indices
    };

    sg_apply_bindings(&bind);
    sg_draw(0, geometry->index_count, buffers->instance_count);
}

void sokol_run_depth_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state)
{
    depth_vs_uniforms_t vs_u;
    glm_mat4_copy(state->uniforms.mat_vp, vs_u.mat_vp);
    
    depth_fs_uniforms_t fs_u;
    glm_vec3_copy(state->uniforms.eye_pos, fs_u.eye_pos);
    fs_u.near_ = state->uniforms.near_;
    fs_u.far_ = state->uniforms.far_;
    fs_u.depth_c = SOKOL_DEPTH_C;
    fs_u.inv_log_far = 1.0 / log(SOKOL_DEPTH_C * fs_u.far_ + 1.0);

    /* Render to offscreen texture so screen-space effects can be applied */
    sg_begin_pass(pass->pass, &pass->pass_action);
    sg_apply_pipeline(pass->pip);

    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){&vs_u, sizeof(depth_vs_uniforms_t)});
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &(sg_range){&fs_u, sizeof(depth_fs_uniforms_t)});

    /* Loop geometry, render scene */
    ecs_iter_t qit = ecs_query_iter(state->world, state->q_scene);
    while (ecs_query_next(&qit)) {
        SokolGeometry *geometry = ecs_field(&qit, SokolGeometry, 0);

        int b;
        for (b = 0; b < qit.count; b ++) {
            depth_draw_instances(&geometry[b], &geometry[b].solid);
            depth_draw_instances(&geometry[b], &geometry[b].emissive);
        }
    }

    sg_end_pass();
}
