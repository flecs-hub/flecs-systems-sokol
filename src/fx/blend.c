#include "../private_api.h"

#include "../private_api.h"

static
const char *shd_blend_tex_mult_header = 
    SOKOL_SHADER_FUNC_RGBA_TO_FLOAT
    ;

static
const char *shd_blend_tex_mult =
    "frag_color = texture(t_a, uv) + texture(t_b, uv);\n"
    ;

#define SSAO_INPUT_SCENE SOKOL_FX_INPUT(0)
#define SSAO_INPUT_DEPTH SOKOL_FX_INPUT(1)

SokolFx sokol_init_blend(
    int width, int height)
{
    ecs_trace("sokol: initialize blend effect");
    ecs_log_push();

    SokolFx fx = {0};
    fx.name = "Blend";
    fx.width = width;
    fx.height = height;

    // Multiply ambient occlusion with the scene
    sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "blend",
        .outputs = {{ .global_size = true }},
        .shader_header = shd_blend_tex_mult_header,
        .shader = shd_blend_tex_mult,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "t_a", "t_b" },
        .steps = {
            [0] = {
                .inputs = { {SOKOL_FX_INPUT(0)}, {SOKOL_FX_INPUT(1)} }
            }
        }
    });

    ecs_log_pop();

    return fx;
}
