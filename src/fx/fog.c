#include "../private_api.h"

static
const char *shd_fog_header = SOKOL_DECODE_FLOAT_RGB;

static
const char *shd_fog =
    "#define LOG2 1.442695\n"
    "vec4 c = texture(hdr, uv);\n"
    "float d_sqr = rgb_to_float(texture(depth, uv));\n"
    "vec4 fog_color = vec4(0.3, 0.6, 0.9, 1.0);\n"
    "float intensity = 1.0 - exp2(-d_sqr * density * density * LOG2);\n"
    "intensity = clamp(intensity, 0.0, 1.0);\n"
    "frag_color = mix(c, fog_color, intensity);\n";

#define FOG_INPUT_HDR SOKOL_FX_INPUT(0)
#define FOG_INPUT_DEPTH SOKOL_FX_INPUT(1)

SokolFx sokol_init_fog(
    int width, int height)
{
    ecs_trace("sokol: initialize fog effect");

    SokolFx fx = {0};
    fx.name = "Fog";

    sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "fog",
        .outputs = {{width, height}},
        .shader_header = shd_fog_header,
        .shader = shd_fog,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .mipmap_count = 2,
        .inputs = { "hdr", "depth" },
        .params = { "density" },
        .steps = {
            [0] = {
                .inputs = { {FOG_INPUT_HDR}, {FOG_INPUT_DEPTH} },
                .params = { 1.8 }
            }
        }
    });

    return fx;
}
