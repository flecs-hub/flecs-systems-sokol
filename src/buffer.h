
typedef struct SokolBuffer {
    /* GPU buffers */
    sg_buffer vertex_buffer;        /* Geometry (static) */
    sg_buffer index_buffer;         /* Indices (static) */
    sg_buffer color_buffer;         /* Color (per instance) */
    sg_buffer transform_buffer;     /* Transform (per instance) */

    /* Application-cached buffers */
    ecs_rgba_t *colors;
    mat4 *transforms;

    /* Number of instances */
    int32_t instance_count;

    /* Number of indices */
    int32_t index_count;
} SokolBuffer;

void sokol_init_buffers(
    ecs_world_t *world);

typedef struct FlecsSystemsSokolBuffer {
    int dummy;
} FlecsSystemsSokolBuffer;

void FlecsSystemsSokolBufferImport(
    ecs_world_t *world);

#define FlecsSystemsSokolBufferImportHandles(handles)
