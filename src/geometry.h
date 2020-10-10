#ifndef SOKOL_GEOMETRY_H
#define SOKOL_GEOMETRY_H

typedef enum sokol_buffer_draw_kind_t {
    SokolDrawSolid = 1,
    SokolDrawEmissive = 2,
    SokolDrawTransparent = 4,
} sokol_buffer_draw_kind_t;

typedef void (*sokol_geometry_action_t)(
    ecs_iter_t *qit, 
    int32_t offset, 
    mat4 *transforms);

typedef struct sokol_instances_t {
    /* Instanced GPU buffers */
    sg_buffer color_buffer;
    sg_buffer transform_buffer;
    sg_buffer material_buffer;

    /* Application-cached buffers */
    ecs_rgba_t *colors;
    mat4 *transforms;
    uint32_t *materials;

    /* Number of instances */
    int32_t instance_count;

    /* Max number of instances allowed in current buffer */
    int32_t instance_max;
 } sokol_instances_t;

typedef struct SokolGeometry {
    /* Static GPU buffers */
    sg_buffer vertex_buffer;
    sg_buffer normal_buffer;
    sg_buffer index_buffer;

    /* Number of indices */
    int32_t index_count;

    /* Instanced data per category */
    sokol_instances_t solid;
    sokol_instances_t emissive;
    sokol_instances_t transparent;

    /* Function that copies geometry-specific data to GPU buffer */
    sokol_geometry_action_t populate;
} SokolGeometry;

typedef struct SokolGeometryQuery {
    ecs_entity_t component;
    ecs_query_t *parent_query;
    ecs_query_t *solid;
    ecs_query_t *emissive;
    ecs_query_t *transparent;
} SokolGeometryQuery;

/* Initialize static resources for geometry rendering */
void sokol_init_geometry(
    ecs_world_t *world,
    sokol_resources_t *resources);

/* Module implementation */

typedef struct FlecsSystemsSokolGeometry {
    int dummy;
} FlecsSystemsSokolGeometry;

void FlecsSystemsSokolGeometryImport(
    ecs_world_t *world);

#define FlecsSystemsSokolGeometryImportHandles(handles)

#endif
