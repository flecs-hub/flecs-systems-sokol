#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#ifndef __APPLE__
#define SOKOL_IMPL
#endif

#define SOKOL_NO_ENTRY
#include "sokol/sokol.h"
#include "private_api.h"

/* Application wrapper */

typedef struct {
    ecs_world_t *world;
    ecs_app_desc_t *desc;
} app_ctx_t;

static
void sokol_frame_action(app_ctx_t *ctx) {
    ecs_app_run_frame(ctx->world, ctx->desc);
}

static
int sokol_run_action(
    ecs_world_t *world,
    ecs_app_desc_t *desc)
{
    app_ctx_t ctx = {
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
        .user_data = &ctx,
        .sample_count = 2,
        .gl_force_gles2 = false,
        .window_title = title,
        .width = width,
        .height = height,
        .high_dpi = true,
        .icon.sokol_default = true
    });

    ecs_trace("sokol: app '%s' finished", title);

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
