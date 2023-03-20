#include "../private_api.h"

static
const char *shd_fog_header = 
    "#include \"etc/sokol/shaders/fx_fog_header.glsl\"\n";

static
const char *shd_fog =
    "#include \"etc/sokol/shaders/fx_fog_main.glsl\"\n";

#define FOG_INPUT_HDR SOKOL_FX_INPUT(0)
#define FOG_INPUT_DEPTH SOKOL_FX_INPUT(1)
#define FOG_INPUT_ATMOS SOKOL_FX_INPUT(2)

SokolFx sokol_init_fog(
    int width, int height)
{
    ecs_trace("sokol: initialize fog effect");
    ecs_log_push();

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
        .inputs = { "hdr", "depth", "atmos" },
        .params = { "u_density", "u_r", "u_g", "u_b", "u_horizon" },
        .steps = {
            [0] = {
                .inputs = { {FOG_INPUT_HDR}, {FOG_INPUT_DEPTH}, {FOG_INPUT_ATMOS} },
                .params = { 1.2, 0.0, 0.0, 0.0 }
            }
        }
    });

    ecs_log_pop();

    return fx;
}

void sokol_fog_set_params(
    SokolFx *fx,
    float density,
    float r,
    float g,
    float b,
    float uv_horizon)
{
    float *params = fx->pass[0].steps[0].params;
    params[0] = density;
    params[1] = r;
    params[2] = g;
    params[3] = b;
    params[4] = uv_horizon;
}
