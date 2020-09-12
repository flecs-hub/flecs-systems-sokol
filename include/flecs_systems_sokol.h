#ifndef FLECS_SYSTEMS_SOKOL_H
#define FLECS_SYSTEMS_SOKOL_H

/* This generated file contains includes for project dependencies */
#include "flecs-systems-sokol/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
typedef int MyComponent;
*/

typedef struct FlecsSystemsSokol {
    int32_t dummy;
} FlecsSystemsSokol;

void FlecsSystemsSokolImport(
    ecs_world_t *world);

#define FlecsSystemsSokolImportHandles(handles)

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace flecs {
namespace systems {

class sokol : FlecsSystemsSokol {
public:
    sokol(flecs::world& ecs) {
        FlecsSystemsSokolImport(ecs.c_ptr());

        ecs.module<flecs::systems::sokol>();
    }
};

}
}

#endif

#endif
