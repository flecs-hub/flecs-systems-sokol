#include "private_api.h"

static
sg_pipeline init_screen_pipeline(void) {
    ecs_trace("sokol: initialize screen pipeline");

    /* create an instancing shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source = sokol_vs_passthrough(),
        .fs = {
            .source =
                SOKOL_SHADER_HEADER
                "uniform sampler2D screen;\n"
                "out vec4 frag_color;\n"
                "in vec2 uv;\n"
                "void main() {\n"
                "  frag_color = texture(screen, uv);\n"
                "}\n"
                ,
            .images[0] = {
                .name = "screen",
                .image_type = SG_IMAGETYPE_2D
            }
        }
    });

    /* create a pipeline object (default render state is fine) */
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {         
            .attrs = {
                /* Static geometry (position, uv) */
                [0] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT2 }
            }
        }
    });
}

sokol_screen_pass_t sokol_init_screen_pass(void) {
    return (sokol_screen_pass_t) {
        .pip = init_screen_pipeline(),
        .pass_action = sokol_clear_action((ecs_rgb_t){0, 0, 0}, false, false)
    };
}

void sokol_run_screen_pass(
    sokol_screen_pass_t *pass,
    sokol_resources_t *resources,
    sokol_render_state_t *state,
    sg_image img)
{
    sg_begin_default_pass(&pass->pass_action, state->width, state->height);
    sg_apply_pipeline(pass->pip);

    sg_bindings bind = {
        .vertex_buffers = { 
            [0] = resources->quad 
        },
        .fs_images[0] = img
    };

    sg_apply_bindings(&bind);

    sg_draw(0, 6, 1);
    sg_end_pass();
}
