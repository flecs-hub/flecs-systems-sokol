#include "private_api.h"

typedef struct depth_vs_uniforms_t {
    mat4 mat_vp;
} depth_vs_uniforms_t;

typedef struct depth_fs_uniforms_t {
    vec3 eye_pos;    
} depth_fs_uniforms_t;

const char* sokol_vs_depth(void) 
{
    return  SOKOL_SHADER_HEADER
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
    return  SOKOL_SHADER_HEADER
            "uniform vec3 u_eye_pos;\n"
            "in vec3 position;\n"
            "out vec4 frag_color;\n"

            "vec4 encodeDepth(float v) {\n"
            "    vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;\n"
            "    enc = fract(enc);\n"
            "    enc -= enc.yzww * vec4(1.0/255.0,1.0/255.0,1.0/255.0,0.0);\n"
            "    return enc;\n"
            "}\n"

            "void main() {\n"
            "  float depth = length(position);\n"
            "  frag_color = encodeDepth(depth);\n"
            "}\n";
}

sg_pipeline init_depth_pipeline(void) {
    ecs_trace("sokol: initialize depth pipieline");

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
                    [0] = { .name="u_eye_pos", .type=SG_UNIFORMTYPE_FLOAT3 }
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
            .pixel_format = SG_PIXELFORMAT_RGBA16F
        }},
        .cull_mode = SG_CULLMODE_BACK
    });
}

sokol_offscreen_pass_t sokol_init_depth_pass(
    int32_t w, 
    int32_t h) 
{
    sg_image color_target = sokol_target_rgba16f(w, h);
    sg_image depth_target = sokol_target_depth(w, h);
    ecs_rgb_t background_color = {0};

    return (sokol_offscreen_pass_t){
        .pass_action = sokol_clear_action(background_color, true, true),
        .pass = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0].image = color_target,
            .depth_stencil_attachment.image = depth_target
        }),
        .pip = init_depth_pipeline(),
        .color_target = color_target,
        .depth_target = depth_target,
    };   
}

static
void depth_draw_instances(
    SokolGeometry *geometry,
    sokol_instances_t *instances)
{
    if (!instances->instance_count) {
        return;
    }

    sg_bindings bind = {
        .vertex_buffers = {
            [0] = geometry->vertex_buffer,
            [1] = instances->transform_buffer
        },
        .index_buffer = geometry->index_buffer
    };

    sg_apply_bindings(&bind);
    sg_draw(0, geometry->index_count, instances->instance_count);
}

void sokol_run_depth_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state)
{
    depth_vs_uniforms_t vs_u;
    glm_mat4_copy(state->uniforms.mat_vp, vs_u.mat_vp);
    
    depth_fs_uniforms_t fs_u;
    glm_vec3_copy(state->uniforms.eye_pos, fs_u.eye_pos);

    /* Render to offscreen texture so screen-space effects can be applied */
    sg_begin_pass(pass->pass, &pass->pass_action);
    sg_apply_pipeline(pass->pip);

    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){&vs_u, sizeof(depth_vs_uniforms_t)});
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &(sg_range){&fs_u, sizeof(depth_fs_uniforms_t)});

    /* Loop geometry, render scene */
    ecs_iter_t qit = ecs_query_iter(state->world, state->q_scene);
    while (ecs_query_next(&qit)) {
        SokolGeometry *geometry = ecs_term(&qit, SokolGeometry, 1);

        int b;
        for (b = 0; b < qit.count; b ++) {
            depth_draw_instances(&geometry[b], &geometry[b].solid);
            depth_draw_instances(&geometry[b], &geometry[b].emissive);
        }
    }
    sg_end_pass();
}
