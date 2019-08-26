#pragma once

#include <flecs_systems_sokol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*sdl_osx_init_func)(
    const void* mtl_device);

typedef void(*sdl_osx_frame_func)(
    ecs_rows_t *rows);

const void* sdl_osx_start(
    void* cametal_layer, 
    int sample_count);
	
extern 
void sdl_osx_frame(
    ecs_rows_t *rows,
    sdl_osx_frame_func frame_cb);

extern 
const void* sdl_osx_mtk_get_render_pass_descriptor();

extern 
const void* sdl_osx_mtk_get_drawable();

#ifdef __cplusplus
} // extern "C"
#endif
