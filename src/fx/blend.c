#include "../private_api.h"



#define BLEND_INPUT_1 SOKOL_FX_INPUT(0)
#define BLEND_INPUT_2 SOKOL_FX_INPUT(1)

SokolFx sokol_init_blend(
    int width, int height)
{
    ecs_trace("sokol: initialize fog effect");

    SokolFx fx = {0};
    fx.name = "Blend";


    return fx;
}
