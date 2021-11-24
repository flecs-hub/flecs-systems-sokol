/** @file Internal renderer module.
 */

#ifndef SOKOL_MODULES_RENDERER_H
#define SOKOL_MODULES_RENDERER_H

#include "../../types.h"

typedef struct SokolRenderer {
    sokol_resources_t resources;

    sokol_offscreen_pass_t shadow_pass;
    sokol_offscreen_pass_t depth_pass;    
    sokol_offscreen_pass_t scene_pass;
    sokol_screen_pass_t screen_pass;

    SokolEffect fx_bloom;
    SokolEffect fx_fog;

    ecs_entity_t canvas;
} SokolRenderer;

ECS_COMPONENT_DECLARE(SokolRenderer);

#define SokolRendererInst (ecs_id(SokolRenderer))

void FlecsSystemsSokolRendererImport(
    ecs_world_t *world);

#endif
