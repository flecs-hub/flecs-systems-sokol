#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#ifndef __APPLE__
#define SOKOL_IMPL
#endif

#define SOKOL_NO_ENTRY
#include "private_api.h"

/* Application wrapper */

typedef struct {
    ecs_world_t *world;
    ecs_app_desc_t *desc;
} sokol_app_ctx_t;

static
void sokol_frame_action(sokol_app_ctx_t *ctx) {
    ecs_app_run_frame(ctx->world, ctx->desc);
}

static
sokol_app_ctx_t sokol_app_ctx;

void DeltaTime(ecs_iter_t *it) {
    printf("delta time: %f\n", it->delta_time);
}

static
int sokol_run_action(
    ecs_world_t *world,
    ecs_app_desc_t *desc)
{
    // ECS_SYSTEM(world, DeltaTime, EcsOnUpdate, 0);
    
    sokol_app_ctx = (sokol_app_ctx_t){
        .world = world,
        .desc = desc
    };

    /* Find canvas instance for width, height & title */
    ecs_iter_t it = ecs_term_iter(world, &(ecs_term_t) {
        .id = ecs_id(EcsCanvas)
    });

    int width = 800, height = 600;
    const char *title = "Flecs App";

    if (ecs_term_next(&it)) {
        EcsCanvas *canvas = ecs_term(&it, EcsCanvas, 1);
        width = canvas->width;
        height = canvas->height;
        title = canvas->title;
    }

    /* If there is more than one canvas, ignore */
    while (ecs_term_next(&it)) { }

    ecs_trace("sokol: starting app '%s'", title);

    /* Run app */
    sapp_run(&(sapp_desc) {
        .frame_userdata_cb = (void(*)(void*))sokol_frame_action,
        .user_data = &sokol_app_ctx,
        .window_title = title,
        .width = width,
        .height = height,
        .sample_count = 2,
#ifndef __EMSCRIPTEN__        
        .high_dpi = true,
#endif
        .gl_force_gles2 = false
    });

    return 0;
}

void FlecsSystemsSokolImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokol);

    ecs_set_name_prefix(world, "Sokol");
    
    ECS_IMPORT(world, FlecsComponentsGui);

    ECS_IMPORT(world, FlecsSystemsSokolMaterials);
    ECS_IMPORT(world, FlecsSystemsSokolRenderer);
    ECS_IMPORT(world, FlecsSystemsSokolGeometry);

    ecs_app_set_run_action(sokol_run_action);
}
