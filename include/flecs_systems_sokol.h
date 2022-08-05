#ifndef FLECS_SYSTEMS_SOKOL_H
#define FLECS_SYSTEMS_SOKOL_H

/* This generated file contains includes for project dependencies */
#include "flecs-systems-sokol/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

FLECS_SYSTEMS_SOKOL_API
void FlecsSystemsSokolImport(
    ecs_world_t *world);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifndef FLECS_NO_CPP

namespace flecs {
namespace systems {

class sokol {
public:
    sokol(flecs::world& ecs) {
        // Load module contents
        FlecsSystemsSokolImport(ecs);

        // Bind C++ types with module contents
        ecs.module<flecs::systems::sokol>();
    }
};

}
}

#endif
#endif

#endif
