/** @file Internal materials module.
 * 
 * Assigns material ids to new materials. Materials are uploaded to the vertex
 * shader as a uniform array with SOKOL_MAX_MATERIALS elements. The data that is
 * uploaded to the vertex shader includes this material id, which is then used
 * by the vertex shader to pass the correct material properties to the fragment
 * shader.
 */

#ifndef SOKOL_MODULES_MATERIALS_H
#define SOKOL_MODULES_MATERIALS_H

#include "../../types.h"

#define SOKOL_MAX_MATERIALS (255)

/* Component with material id, assigned to any entity that has material data */
typedef struct {
    uint32_t material_id;
} SokolMaterialId;

/* Element with material parameters */
typedef struct {
    float specular_power;
    float shininess;
    float emissive;
} SokolMaterial;

/* Array with material data. Is added to same entity as Renderer. */
typedef struct {
    bool changed;
    SokolMaterial array[SOKOL_MAX_MATERIALS];
} SokolMaterials;

extern ECS_COMPONENT_DECLARE(SokolMaterialId);
extern ECS_COMPONENT_DECLARE(SokolMaterials);

void FlecsSystemsSokolMaterialsImport(
    ecs_world_t *world);

#endif
