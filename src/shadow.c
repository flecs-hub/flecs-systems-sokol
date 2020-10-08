#include "private_include.h"

typedef struct vs_uniforms_t {
    mat4 mat_vp;
} vs_uniforms_t;

static const char *shd_v = 
    "#version 330\n"
    "uniform mat4 u_mat_vp;\n"
    "layout(location=0) in vec4 v_position;\n"
    "layout(location=1) in mat4 i_mat_m;\n"
    "out vec2 proj_zw;\n"
    "void main() {\n"
    "  gl_Position = u_mat_vp * i_mat_m * v_position;\n"
    "  proj_zw = gl_Position.zw;\n"
    "}\n";

static const char *shd_f =
    "#version 330\n"
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

sokol_shadow_pass_t sokol_init_shadow_pass(
    int size)
{
    sokol_shadow_pass_t result = {0};

    result.pass_action  = (sg_pass_action) {
        .colors[0] = { 
            .action = SG_ACTION_CLEAR, 
            .val = { 1.0f, 1.0f, 1.0f, 1.0f}
        }
    };

    result.depth_tex = sokol_init_render_depth_target(size, size);
    result.color_tex = sokol_init_render_target_8(size, size);

    result.pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = result.color_tex,
        .depth_stencil_attachment.image = result.depth_tex,
        .label = "shadow-map-pass"
    });

    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.uniform_blocks = {
            [0] = {
                .size = sizeof(vs_uniforms_t),
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
        .blend = {
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true
        },
        .rasterizer.cull_mode = SG_CULLMODE_FRONT
    });

    return result;
}

static
void init_uniforms(
    const EcsDirectionalLight *light,
    vs_uniforms_t *vs_out)
{
    mat4 mat_p;
    mat4 mat_v;
    vec3 lookat = {0.0, 0.0, 0.0};
    vec3 up = {0, 1, 0};

    glm_ortho(-7, 7, -7, 7, -10, 30, mat_p);

    vec4 dir = {
        light->direction[0],
        light->direction[1],
        light->direction[2]
    };
    glm_vec4_scale(dir, 50, dir);
    glm_lookat(dir, lookat, up, mat_v);

    mat4 light_proj = {
         { 0.5f, 0.0f, 0.0f, 0 },
         { 0.0f, 0.5f, 0.0f, 0 },
         { 0.0f, 0.0f, 0.5f, 0 },
         { 0.5f, 0.5f, 0.5f, 1 }
    };
    
    glm_mat4_mul(mat_p, light_proj, mat_p);
    glm_mat4_mul(mat_p, mat_v, vs_out->mat_vp);
}

void sokol_run_shadow_pass(
    ecs_query_t *buffers,
    sokol_shadow_pass_t *pass,
    const EcsDirectionalLight *light_data,
    mat4 light_vp) 
{
    /* Render to offscreen texture so screen-space effects can be applied */
    sg_begin_pass(pass->pass, &pass->pass_action);
    sg_apply_pipeline(pass->pip);

    vs_uniforms_t vs_u;
    init_uniforms(light_data, &vs_u);

    /* Loop buffers, render scene */
    ecs_iter_t qit = ecs_query_iter(buffers);
    while (ecs_query_next(&qit)) {
        SokolBuffer *buffer = ecs_column(&qit, SokolBuffer, 1);
        
        int b;
        for (b = 0; b < qit.count; b ++) {
            if (!buffer[b].instance_count) {
                continue;
            }
            sg_bindings bind = {
                .vertex_buffers = {
                    [0] = buffer[b].vertex_buffer,
                    [1] = buffer[b].transform_buffer
                },
                .index_buffer = buffer[b].index_buffer
            };

            sg_apply_bindings(&bind);
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_u, sizeof(vs_uniforms_t));
            sg_draw(0, buffer[b].index_count, buffer[b].instance_count);
        }
    }
    sg_end_pass();

    glm_mat4_copy(vs_u.mat_vp, light_vp);
}