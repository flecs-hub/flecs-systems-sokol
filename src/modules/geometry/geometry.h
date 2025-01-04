#ifndef SOKOL_MODULES_GEOMETRY_H
#define SOKOL_MODULES_GEOMETRY_H

#include "../../types.h"
#include "../renderer/renderer.h"

typedef void (*sokol_geometry_action_t)(
    mat4 *transforms,
    void *data,
    int32_t count,
    bool self);

typedef struct sokol_geometry_buffers_t {
    /* Temporary buffers for storing data gathered from ECS. This allows data to
     * be copied in one call to the graphics API. */
    ecs_vec_t colors_data;
    ecs_vec_t transforms_data;
    ecs_vec_t materials_data;

    /* Sokol buffers with instanced data */
    sg_buffer colors;
    sg_buffer transforms;
    sg_buffer materials;

    /* Number of instances */
    int32_t instance_count;
} sokol_geometry_buffers_t;

typedef struct SokolGeometry {
    /* GPU buffers with static geometry data */
    sg_buffer vertices;
    sg_buffer normals;
    sg_buffer indices;

    /* Number of indices */
    int32_t index_count;

    /* Buffers with instanced data */
    sokol_geometry_buffers_t solid;
    sokol_geometry_buffers_t emissive;

    /* Function that copies geometry-specific data to GPU buffer */
    sokol_geometry_action_t populate;

    /* Allocator */
    ecs_allocator_t *allocator;
} SokolGeometry;

typedef struct SokolGeometryQuery {
    ecs_entity_t component;
    ecs_query_t *parent_query;
    ecs_query_t *solid;
} SokolGeometryQuery;

/* Element with material parameters */
typedef struct {
    float specular_power;
    float shininess;
    float emissive;
} SokolMaterial;

extern ECS_COMPONENT_DECLARE(SokolGeometry);
extern ECS_COMPONENT_DECLARE(SokolGeometryQuery);

/* Initialize static resources for geometry rendering */
void sokol_init_geometry(
    ecs_world_t *world,
    sokol_resources_t *resources);

/* Module import function */
void FlecsSystemsSokolGeometryImport(
    ecs_world_t *world);

#endif
