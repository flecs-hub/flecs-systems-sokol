#ifndef SOKOL_MODULES_GEOMETRY_H
#define SOKOL_MODULES_GEOMETRY_H

#include "../../types.h"
#include "../renderer/renderer.h"
#include "../materials/materials.h"

typedef void (*sokol_geometry_action_t)(
    mat4 *transforms,
    void *data,
    int32_t count,
    bool self);

#define SOKOL_GEOMETRY_PAGE_SIZE (65536)

typedef struct sokol_geometry_page_t {
    /* Buffers with instanced data */
    ecs_rgb_t *colors;
    mat4 *transforms;
    SokolMaterial *materials;

    /* Number of instances in page */
    int32_t count;

    /* Next page */
    struct sokol_geometry_page_t *next;
} sokol_geometry_page_t;

struct sokol_geometry_buffer_t;

/* A group stores geometry data for a (world_cell, prefab) combination. Groups
 * are only updated when their data has changed. If an entity is not an instance
 * of a prefab, or does not have a world cell, 0 is used as placeholder. */
typedef struct sokol_geometry_group_t {
    /* Pages with instanced data */
    sokol_geometry_page_t *first_page;
    sokol_geometry_page_t *last_page;
    sokol_geometry_page_t *first_no_data;

    /* Prev/next group for buffers */
    struct sokol_geometry_group_t *prev, *next;

    /* Backref to buffer that group is part of */
    struct sokol_geometry_buffer_t *buffer;

    /* ref<DrawDistance> to determine if group contents are visible */
    ecs_ref_t draw_distance;

    /* Number of instances in the group */
    int32_t count;

    /* Last match count. Used to detect if tables were removed from group */
    int32_t match_count;

    /* Is group visible */
    bool visible;

    /* Group id */
    uint64_t id;
} sokol_geometry_group_t;

/* Buffers are maintained per world cell. Multiple groups can be stored in one
 * set of buffers if a cell contains entities of different kinds of prefabs. */
typedef struct sokol_geometry_buffer_t {
    /* Buffer id */
    ecs_entity_t id;

    /* ref<CellCoord> to find location and size of cell */
    ecs_ref_t cell_coord;

    /* Linked list with groups for buffers */
    sokol_geometry_group_t *groups;

    /* Linked list with all buffers for geometry */
    struct sokol_geometry_buffer_t *prev, *next;

    /* Did any of the group data change */
    bool changed;
} sokol_geometry_buffer_t;

typedef struct sokol_geometry_buffers_t {
    sokol_geometry_buffer_t *first;
    ecs_map_t index; /* map<world_cell, sokol_geometry_buffer_t*> */

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

    /* Allocator */
    ecs_allocator_t allocator;
} sokol_geometry_buffers_t;

typedef struct SokolGeometry {
    /* GPU buffers with static geometry data */
    sg_buffer vertices;
    sg_buffer normals;
    sg_buffer indices;

    /* Number of indices */
    int32_t index_count;

    /* Buffers with instanced data (one set per world cell) */
    sokol_geometry_buffers_t *solid;
    sokol_geometry_buffers_t *emissive;

    /* Function that copies geometry-specific data to GPU buffer */
    sokol_geometry_action_t populate;

    /* Temporary storage for group ids to process */
    ecs_vec_t group_ids;
} SokolGeometry;

typedef struct SokolGeometryQuery {
    ecs_entity_t component;
    ecs_query_t *parent_query;
    ecs_query_t *solid;
    ecs_query_t *emissive;
} SokolGeometryQuery;

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
