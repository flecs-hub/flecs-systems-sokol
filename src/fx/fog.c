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
    "vec4 fog_color = vec4(u_r, u_g, u_b, 1.0);\n"
    "float intensity = 1.0 - exp2(-(d * d) * u_density * u_density * LOG2);\n"
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
    fx.width = width;
    fx.height = height;

    sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "fog",
        .outputs = {{ .global_size = true }},
        .shader_header = shd_fog_header,
        .shader = shd_fog,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .mipmap_count = 2,
        .inputs = { "hdr", "depth" },
        .params = { "u_density", "u_r", "u_g", "u_b" },
        .steps = {
            [0] = {
                .inputs = { {FOG_INPUT_HDR}, {FOG_INPUT_DEPTH} },
                .params = { 1.2, 0.0, 0.0, 0.0 }
            }
        }
    });

    return fx;
}

void sokol_fog_set_params(
    SokolFx *fx,
    float density,
    float r,
    float g,
    float b)
{
    float *params = fx->pass[0].steps[0].params;
    params[0] = density;
    params[1] = r;
    params[2] = g;
    params[3] = b;
}
