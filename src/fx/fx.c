#include "../private_api.h"

sokol_fx_resources_t* sokol_init_fx(
    int w, int h)
{
    sokol_fx_resources_t *result = ecs_os_calloc_t(sokol_fx_resources_t);
    result->hdr = sokol_init_hdr(w, h);
    return result;
}
