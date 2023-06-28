#include "../private_api.h"

static
const char *shd_ssao_header = 
    "#include \"etc/sokol/shaders/fx_ssao_header.glsl\"\n";

static
const char *shd_ssao =
    "#include \"etc/sokol/shaders/fx_ssao_main.glsl\"\n";

static
const char *shd_blend_mult_header = 
    "#include \"etc/sokol/shaders/common.glsl\"\n";

static
const char *shd_blend_mult =
    "#include \"etc/sokol/shaders/fx_ssao_blend.glsl\"\n";

#define SSAO_INPUT_SCENE SOKOL_FX_INPUT(0)
#define SSAO_INPUT_DEPTH SOKOL_FX_INPUT(1)

SokolFx sokol_init_ssao(
    int width, int height)
{
    ecs_trace("sokol: initialize ambient occlusion effect");
    ecs_log_push();

    SokolFx fx = {0};
    fx.name = "AmbientOcclusion";
    fx.width = width;
    fx.height = height;

#ifdef __EMSCRIPTEN__
    float factor = 2;
#else
    float factor = 0.5;
#endif

    // Ambient occlusion shader 
    int32_t ao = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "ssao",
        .outputs = {{ .global_size = true, .factor = factor }},
        .shader_header = shd_ssao_header,
        .shader = shd_ssao,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "t_depth" },
        .steps = {
            [0] = {
                .inputs = { {SSAO_INPUT_DEPTH} }
            }
        }
    });

    // Blur to reduce the noise, so we can keep sample count low
    int blur = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "blur",
        .outputs = {{512}},
        .shader_header = shd_blur_hdr,
        .shader = shd_blur,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "tex" },
        .params = { "horizontal" },
        .steps = {
            [0] = { 
                .name = "ssao hblur",
                .inputs = { {SOKOL_FX_PASS(ao)} },
                .params = { 1.0 },
                .loop_count = 4
            },
            [1] = { 
                .name = "ssao vblur",
                .inputs = { { -1 } },
                .params = { 0.0 }
            }
        }
    });

    // Multiply ambient occlusion with the scene
    sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "blend",
        .outputs = {{ .global_size = true }},
        .shader_header = shd_blend_mult_header,
        .shader = shd_blend_mult,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "t_scene", "t_occlusion" },
        .steps = {
            [0] = {
                .inputs = { {SSAO_INPUT_SCENE}, {SOKOL_FX_PASS(blur)} }
            }
        }
    });

    ecs_log_pop();

    return fx;
}
