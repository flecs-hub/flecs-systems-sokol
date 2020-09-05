#include "private_include.h"

ECS_CTOR(SokolBuffer, ptr, {
    ptr->vertex_buffer = (sg_buffer){ 0 };
    ptr->index_buffer = (sg_buffer){ 0 };
    ptr->color_buffer = (sg_buffer){ 0 };
    ptr->transform_buffer = (sg_buffer){ 0 };

    ptr->colors = NULL;
    ptr->transforms = NULL;

    ptr->instance_count = 0;
    ptr->index_count = 0;
});

ECS_DTOR(SokolBuffer, ptr, {
    ecs_os_free(ptr->colors);
    ecs_os_free(ptr->transforms);
});

static
void init_rect_buffers(
    ecs_world_t *world) 
{
    ecs_entity_t rect_buf = ecs_lookup_fullpath(
        world, "flecs.systems.sokol.RectangleBuffer");
    ecs_assert(rect_buf != 0, ECS_INTERNAL_ERROR, NULL);

    ecs_entity_t sokol_buffer = ecs_lookup_fullpath(
        world, "flecs.systems.sokol.Buffer");
    ecs_assert(sokol_buffer != 0, ECS_INTERNAL_ERROR, NULL);

    SokolBuffer *b = ecs_get_mut_w_entity(world, rect_buf, sokol_buffer, NULL);
    ecs_assert(b != NULL, ECS_INTERNAL_ERROR, NULL);

    vec3 vertices[] = {
        {-0.5, -0.5, 0.0},
        { 0.5, -0.5, 0.0},
        { 0.5,  0.5, 0.0},
        {-0.5,  0.5, 0.0}
    };

    uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    b->vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .usage = SG_USAGE_IMMUTABLE
    });

    b->index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(indices),
        .content = indices,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE
    });

    b->index_count = 6;
}

static
void init_box_buffers(
    ecs_world_t *world) 
{
    ecs_entity_t box_buf = ecs_lookup_fullpath(
        world, "flecs.systems.sokol.BoxBuffer");
    ecs_assert(box_buf != 0, ECS_INTERNAL_ERROR, NULL);

    ecs_entity_t sokol_buffer = ecs_lookup_fullpath(
        world, "flecs.systems.sokol.Buffer");
    ecs_assert(sokol_buffer != 0, ECS_INTERNAL_ERROR, NULL);

    SokolBuffer *b = ecs_get_mut_w_entity(world, box_buf, sokol_buffer, NULL);
    ecs_assert(b != NULL, ECS_INTERNAL_ERROR, NULL);

    vec3 vertices[] = {
        {-0.5, -0.5, -0.5},
        { 0.5, -0.5, -0.5},

        { 0.5,  0.5, -0.5},
        {-0.5,  0.5, -0.5},

        { 0.5, -0.5, 0.5},
        { 0.5,  0.5, 0.5},

        {-0.5, -0.5, 0.5},
        {-0.5,  0.5, 0.5},
    };

    uint16_t indices[] = {
        /* Front */
        0, 1, 2,    0, 2, 3,

        /* Right */
        1, 4, 5,   1, 5, 2,

        /* Left */  
        3, 7, 6,    3, 6, 0,

        /* Back */
        4, 6, 7,    4, 7, 5,

        /* Top */
        6, 4, 1,    6, 1, 0,

        /* Bottom */
        5, 7, 3,    5, 3, 2
    };

    b->vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .usage = SG_USAGE_IMMUTABLE
    });

    b->index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(indices),
        .content = indices,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE
    });

    b->index_count = 36;
}

void sokol_init_buffers(
    ecs_world_t *world) 
{
    init_rect_buffers(world);
    init_box_buffers(world);
}

static
void attach_buffer(
    ecs_iter_t *it, 
    void(*action)(
        ecs_iter_t* q, 
        int32_t offset, 
        mat4 *transforms)) 
{
    EcsQuery *q = ecs_column(it, EcsQuery, 1);

    ecs_query_t *query = q->query;

    if (ecs_query_changed(query)) {
        SokolBuffer *b = ecs_column(it, SokolBuffer, 2);
        ecs_iter_t qit = ecs_query_iter(query);
        
        /* First count number of instances */
        int32_t count = 0;
        while (ecs_query_next(&qit)) {
            count += qit.count;
        }

        if (!count) {
            b->instance_count = 0;
        } else {
            ecs_rgba_t *colors = b->colors;
            mat4 *transforms = b->transforms;
            int32_t instance_count = b->instance_count;
            bool is_new = false;

            if (!colors && !transforms) {
                is_new = true;
            }

            int colors_size = count * sizeof(ecs_rgba_t);
            int transforms_size = count * sizeof(EcsTransform3);

            if (instance_count < count) {
                colors = ecs_os_realloc(colors, colors_size);
                transforms = ecs_os_realloc(transforms, transforms_size);
            }

            int32_t cursor = 0;

            ecs_iter_t qit = ecs_query_iter(query);
            while (ecs_query_next(&qit)) {
                EcsColor *c = ecs_column(&qit, EcsColor, 2);
                EcsTransform3 *t = ecs_column(&qit, EcsTransform3, 3);

                memcpy(&colors[cursor], c, qit.count * sizeof(ecs_rgba_t));
                memcpy(&transforms[cursor], t, qit.count * sizeof(mat4));

                action(&qit, cursor, transforms);

                cursor += qit.count;
            }

            b->colors = colors;
            b->transforms = transforms;
            b->instance_count = count;

            if (is_new) {
                b->color_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = colors_size,
                    .usage = SG_USAGE_STREAM
                });

                b->transform_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = transforms_size,
                    .usage = SG_USAGE_STREAM
                });
            }

            sg_update_buffer(b->color_buffer, colors, colors_size);
            sg_update_buffer(b->transform_buffer, transforms, transforms_size);
        }
    }
}

static
void attach_rect(ecs_iter_t *qit, int32_t offset, mat4 *transforms) {
    EcsRectangle *r = ecs_column(qit, EcsRectangle, 1);

    int i;
    for (i = 0; i < qit->count; i ++) {
        vec3 scale = {r[i].width, r[i].height, 1.0};
        glm_scale(transforms[offset + i], scale);
    }
}

static
void attach_box(ecs_iter_t *qit, int32_t offset, mat4 *transforms) {
    EcsBox *b = ecs_column(qit, EcsBox, 1);
    
    int i;
    for (i = 0; i < qit->count; i ++) {
        vec3 scale = {b[i].width, b[i].height, b[i].depth};
        glm_scale(transforms[offset + i], scale);
    }
}

static
void SokolAttachRect(ecs_iter_t *it) {
    attach_buffer(it, attach_rect);
}

static
void SokolAttachBox(ecs_iter_t *it) {
    attach_buffer(it, attach_box);
}

void FlecsSystemsSokolBufferImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokolBuffer);

    /* Store components in parent sokol scope */
    ecs_entity_t scope = ecs_lookup_fullpath(world, "flecs.systems.sokol");
    ecs_set_scope(world, scope);

    ECS_IMPORT(world, FlecsComponentsTransform);
    ECS_IMPORT(world, FlecsComponentsGeometry);

    ecs_set_name_prefix(world, "Sokol");

    ECS_COMPONENT(world, SokolBuffer);

    ecs_set_component_actions(world, SokolBuffer, {
        .ctor = ecs_ctor(SokolBuffer),
        .dtor = ecs_dtor(SokolBuffer)
    });    

    /* Support for rectangle primitive */

    ECS_ENTITY(world, SokolRectangleBuffer,
        Buffer, flecs.system.Query);

        /* Create query for rectangles */
        ecs_set(world, SokolRectangleBuffer, EcsQuery, {
            ecs_query_new(world, 
                "[in] flecs.components.geometry.Rectangle,"
                "[in] flecs.components.geometry.Color,"
                "[in] flecs.components.transform.Transform3,")
        });

    /* Create system that manages buffers for rectangles */
    ECS_SYSTEM(world, SokolAttachRect, EcsPostLoad, 
        RectangleBuffer:flecs.system.Query, 
        RectangleBuffer:Buffer);

    /* Support for box primitive */

    ECS_ENTITY(world, SokolBoxBuffer,
        Buffer, flecs.system.Query);

        /* Create query for boxes */
        ecs_set(world, SokolBoxBuffer, EcsQuery, {
            ecs_query_new(world, 
                "[in] flecs.components.geometry.Box,"
                "[in] flecs.components.geometry.Color,"
                "[in] flecs.components.transform.Transform3,")
        });        

    /* Create system that manages buffers for rectangles */
    ECS_SYSTEM(world, SokolAttachBox, EcsPostLoad, 
        BoxBuffer:flecs.system.Query, 
        BoxBuffer:Buffer);       
}