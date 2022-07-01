#include "../private_api.h"

static
const char *shd_fog_header = 
    SOKOL_SHADER_FUNC_RGBA_TO_FLOAT
    SOKOL_SHADER_FUNC_RGBA_TO_DEPTH
    SOKOL_SHADER_FUNC_RGBA_TO_DEPTH_LOG
    ;

static
const char *shd_fog =
    "#define LOG2 1.442695\n"
    "vec4 c = texture(hdr, uv);\n"
    "float d = (rgba_to_depth(texture(depth, uv)) / u_far);\n"
    "vec4 fog_color = vec4(0.3, 0.6, 0.9, 1.0);\n"
    "float intensity = 1.0 - exp2(-(d * d) * u_density * u_density * LOG2);\n"
    "intensity *= 1.1;\n"
    "frag_color = mix(c, fog_color, intensity);\n"
    ;

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
        .params = { "u_density" },
        .steps = {
            [0] = {
                .inputs = { {FOG_INPUT_HDR}, {FOG_INPUT_DEPTH} },
                .params = { 1.5}
            }
        }
    });

    return fx;
}
