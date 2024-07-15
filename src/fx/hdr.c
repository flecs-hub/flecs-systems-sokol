#include "../private_api.h"

// ITU BT.709: vec3(0.2126, 0.7152, 0.0722)
// ITU BT.601: vec3(0.299, 0.587, 0.114)

static
const char *shd_threshold =
    "#include \"etc/sokol/shaders/fx_hdr_threshold.glsl\"\n";

const char *shd_blur_hdr =
    "const float gauss[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n";

const char *shd_blur =
    "#include \"etc/sokol/shaders/fx_hdr_blur.glsl\"\n";

static
const char *shd_hdr =
    "#include \"etc/sokol/shaders/fx_hdr.glsl\"\n";

#define FX_INPUT_HDR (0)
#define FX_INPUT_LUM (1)

SokolFx sokol_init_hdr(
    int width, int height)
{
    ecs_trace("sokol: initialize hdr effect");
    ecs_log_push();

    SokolFx fx = {0};
    fx.name = "Hdr";
    fx.width = width;
    fx.height = height;

    int threshold = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "threshold",
        .outputs = {{1024}},
        .shader = shd_threshold,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "hdr" },
        .params = { "threshold", "mipmap" },
        .steps = {
            [0] = {
                .name = "glow",
                .params = { 0.1, 1.0 },
                .inputs = { {FX_INPUT_HDR} }
            }
        }
    });

    int blur = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "blur",
        .outputs = {{512}},
        .shader_header = shd_blur_hdr,
        .shader = shd_blur,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "tex" },
        .params = { "horizontal" },
        .steps = {
            /* glow */
            [0] = { 
                .name = "halo hblur",
                .inputs = { {SOKOL_FX_PASS(threshold), 0} },
                .params = { 1.0 },
                .loop_count = 5
            },
            [1] = { 
                .name = "halo vblur",
                .inputs = { { -1 } },
                .params = { 0.0 }
            }
        }
    });

    sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "hdr",
        .outputs = {{ .global_size = true }},
        .shader = shd_hdr,
        .inputs = { "hdr", "bloom" },
        .params = { "exposure", "gamma" },
        .steps = {
            [0] = {
                .inputs = { {SOKOL_FX_INPUT(0)}, {SOKOL_FX_PASS(blur), 0} },
                .params = { 1.0, 2.2 }
            }
        }
    });

    ecs_log_pop();

    return fx;
}
