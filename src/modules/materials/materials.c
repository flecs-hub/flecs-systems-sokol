#include "materials.h"

void SokolInitMaterials(ecs_iter_t *it) {
    const EcsQuery *q = ecs_term(it, EcsQuery, 1);
    SokolMaterials *materials = ecs_term(it, SokolMaterials, 2);

    materials->changed = true;
    materials->array[0].specular_power = 0.0;
    materials->array[0].shininess = 1.0;
    materials->array[0].emissive = 0.0;

    if (!ecs_query_changed(q->query)) {
        materials->changed = false;
        return;
    }

    ecs_iter_t qit = ecs_query_iter(it->world, q->query);
    while (ecs_query_next(&qit)) {
        SokolMaterialId *mat = ecs_term(&qit, SokolMaterialId, 1);
        EcsSpecular *spec = ecs_term(&qit, EcsSpecular, 2);
        EcsEmissive *em = ecs_term(&qit, EcsEmissive, 3);

        int i;
        if (spec) {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                materials->array[id].specular_power = spec[i].specular_power;
                materials->array[id].shininess = spec[i].shininess;
            }
        } else {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                materials->array[id].specular_power = 0;
                materials->array[id].shininess = 1.0;
            }            
        }

        if (em) {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                materials->array[id].emissive = em[i].value;
            }
        } else {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                materials->array[id].emissive = 0;
            }
        }
    }    
}

static
void SokolRegisterMaterial(ecs_iter_t *it) {
    static uint16_t next_material = 1; /* 0 is the default material */

    int i;
    for (i = 0; i < it->count; i ++) {
        ecs_set(it->world, it->entities[i], SokolMaterialId, {
            next_material ++ /* Assign material id */
        });
    }

    ecs_assert(next_material < SOKOL_MAX_MATERIALS, 
        ECS_INVALID_PARAMETER, NULL);
}

void FlecsSystemsSokolMaterialsImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokolMaterials);

    /* Store components in parent sokol scope */
    ecs_entity_t parent = ecs_lookup_fullpath(world, "flecs.systems.sokol");
    ecs_entity_t module = ecs_set_scope(world, parent);
    ecs_set_name_prefix(world, "Sokol");

    ECS_COMPONENT_DEFINE(world, SokolMaterialId);
    ECS_COMPONENT_DEFINE(world, SokolMaterials);

    /* Register systems in module scope */
    ecs_set_scope(world, module);

    /* Query that finds all entities with material properties */
    const char *material_query = 
        "[in] flecs.systems.sokol.MaterialId(self),"
        "[in] ?flecs.components.graphics.Specular(self),"
        "[in] ?flecs.components.graphics.Emissive(self),"
        "     ?Prefab";

    /* System that initializes material array that's sent to vertex shader */
    ECS_SYSTEM(world, SokolInitMaterials, EcsOnLoad,
        [in]   Query(InitMaterials, SokolMaterials),
        [out]  SokolMaterials);

    /* Set material query for InitMaterials system */
    ecs_set_pair(world, SokolInitMaterials, EcsQuery, ecs_id(SokolMaterials), {
        ecs_query_new(world, material_query)
    });

    /* Assigns material id to entities with material properties */
    ECS_SYSTEM(world, SokolRegisterMaterial, EcsPostLoad,
        [out] !flecs.systems.sokol.MaterialId,
        [in]   flecs.components.graphics.Specular(self) || 
               flecs.components.graphics.Emissive(self),
               ?Prefab);
}
