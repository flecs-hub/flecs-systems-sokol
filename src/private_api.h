
#ifndef FLECS_SYSTEMS_SOKOL_PRIVATE_API
#define FLECS_SYSTEMS_SOKOL_PRIVATE_API

#include "types.h"
#include "modules/renderer/renderer.h"
#include "modules/geometry/geometry.h"

void sokol_run_scene_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state);

char* sokol_shader_from_file(
    const char *filename);

char* sokol_shader_from_str(
    const char *str);

#endif
