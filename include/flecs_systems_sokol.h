#ifndef FLECS_SYSTEMS_SOKOL_H
#define FLECS_SYSTEMS_SOKOL_H

/* This generated file contains includes for project dependencies */
#include <flecs-systems-sokol/bake_config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FlecsSystemsSokol {
    /*
     * ECS_DECLARE_ENTITY(MyTag);
     * ECS_DECLARE_COMPONENT(MyComponent);
     */
} FlecsSystemsSokol;

void FlecsSystemsSokolImport(
    ecs_world_t *world,
    int flags);

#define FlecsSystemsSokolImportHandles(handles) /*\
    * ECS_IMPORT_ENTITY(handles, MyTag);\
    * ECS_IMPORT_COMPONENT(handles, MyComponent); 
    */

#ifdef __cplusplus
}
#endif

#endif
