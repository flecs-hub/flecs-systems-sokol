#include "private_api.h"

typedef struct shadow_vs_uniforms_t {
    mat4 mat_vp;
} shadow_vs_uniforms_t;

static const char *shd_v = 
    SOKOL_SHADER_HEADER
    "uniform mat4 u_mat_vp;\n"
    "layout(location=0) in vec3 v_position;\n"
    "layout(location=1) in mat4 i_mat_m;\n"
    "out vec2 proj_zw;\n"
    "void main() {\n"
    "  gl_Position = u_mat_vp * i_mat_m * vec4(v_position, 1.0);\n"
    "  proj_zw = gl_Position.zw;\n"
    "}\n";

static const char *shd_f =
    SOKOL_SHADER_HEADER
    "in vec2 proj_zw;\n"
    "out vec4 frag_color;\n"

    "vec4 encodeDepth(float v) {\n"
    "    vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;\n"
    "    enc = fract(enc);\n"
    "    enc -= enc.yzww * vec4(1.0/255.0,1.0/255.0,1.0/255.0,0.0);\n"
    "    return enc;\n"
    "}\n"

    "void main() {\n"
    "  float depth = proj_zw.x / proj_zw.y;\n"
    "  frag_color = encodeDepth(depth);\n"
    "}\n";

sokol_offscreen_pass_t sokol_init_shadow_pass(
    int size)
{
    ecs_trace("sokol: initialize shadow pipeline");

    sokol_offscreen_pass_t result = {0};

    result.pass_action  = (sg_pass_action) {
        .colors[0] = { 
            .action = SG_ACTION_CLEAR, 
            .value = { 1.0f, 1.0f, 1.0f, 1.0f}
        }
    };

    result.depth_target = sokol_target_depth(size, size, 1);
    result.color_target = sokol_target_rgba8("Shadow map", size, size, 1);

    result.pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = result.color_target,
        .depth_stencil_attachment.image = result.depth_target,
        .label = "shadow-map-pass"
    });

    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.uniform_blocks = {
            [0] = {
                .size = sizeof(shadow_vs_uniforms_t),
                .uniforms = {
                    [0] = { .name="u_mat_vp", .type=SG_UNIFORMTYPE_MAT4 },
                },
            }
        },
        .vs.source = shd_v,
        .fs.source = shd_f
    });

    /* Create pipeline that mimics the normal pipeline, but without the material
     * normals and color, and with front culling instead of back culling */
    result.pip = sg_make_pipeline(&(sg_pipeline_desc){
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
        .cull_mode = SG_CULLMODE_FRONT
    });

    return result;
}

static
void shadow_draw_instances(
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

void sokol_run_shadow_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state) 
{
    /* Render to offscreen texture so screen-space effects can be applied */
    sg_begin_pass(pass->pass, &pass->pass_action);
    sg_apply_pipeline(pass->pip);

    shadow_vs_uniforms_t vs_u;
    glm_mat4_copy(state->uniforms.light_mat_vp, vs_u.mat_vp);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){ 
        &vs_u, sizeof(shadow_vs_uniforms_t) 
    });

    /* Loop buffers, render scene */
    ecs_iter_t qit = ecs_query_iter(state->world, state->q_scene);
    while (ecs_query_next(&qit)) {
        SokolGeometry *geometry = ecs_field(&qit, SokolGeometry, 0);
        
        int b;
        for (b = 0; b < qit.count; b ++) {
            shadow_draw_instances(&geometry[b], &geometry[b].solid);
        }
    }

    sg_end_pass();
}
