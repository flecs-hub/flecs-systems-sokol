#include "private_include.h"

ECS_CTOR(SokolBuffer, ptr, {
    ptr->vertex_buffer = (sg_buffer){ 0 };
    ptr->index_buffer = (sg_buffer){ 0 };
    ptr->color_buffer = (sg_buffer){ 0 };
    ptr->transform_buffer = (sg_buffer){ 0 };
    ptr->material_buffer = (sg_buffer){ 0 };

    ptr->colors = NULL;
    ptr->transforms = NULL;
    ptr->materials = NULL;

    ptr->instance_count = 0;
    ptr->instance_max = 0;
    ptr->index_count = 0;
});

ECS_DTOR(SokolBuffer, ptr, {
    ecs_os_free(ptr->colors);
    ecs_os_free(ptr->transforms);
    ecs_os_free(ptr->materials);
});

static
void compute_flat_normals(
    vec3 *vertices,
    uint16_t *indices,
    int32_t count,
    vec3 *normals_out)
{
    int32_t v;
    for (v = 0; v < count; v += 3) {
        vec3 vec1, vec2, normal;
        glm_vec3_sub(vertices[indices[v + 0]], vertices[indices[v + 1]], vec1);
        glm_vec3_sub(vertices[indices[v + 0]], vertices[indices[v + 2]], vec2);
        glm_vec3_crossn(vec2, vec1, normal);
        
        glm_vec3_copy(normal, normals_out[indices[v + 0]]);
        glm_vec3_copy(normal, normals_out[indices[v + 1]]);
        glm_vec3_copy(normal, normals_out[indices[v + 2]]);
    }
}

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

    vec3 normals[6];
    compute_flat_normals(vertices, indices, 6, normals);

    b->vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .usage = SG_USAGE_IMMUTABLE
    });

    b->normal_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(normals),
        .content = normals,
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
        {-0.5f, -0.5f, -0.5f}, // Back   
        { 0.5f, -0.5f, -0.5f},    
        { 0.5f,  0.5f, -0.5f},    
        {-0.5f,  0.5f, -0.5f},  

        {-0.5f, -0.5f,  0.5f}, // Front  
        { 0.5f, -0.5f,  0.5f},    
        { 0.5f,  0.5f,  0.5f},    
        {-0.5f,  0.5f,  0.5f}, 

        {-0.5f, -0.5f, -0.5f}, // Left   
        {-0.5f,  0.5f, -0.5f},    
        {-0.5f,  0.5f,  0.5f},    
        {-0.5f, -0.5f,  0.5f},    

        { 0.5f, -0.5f, -0.5f}, // Right   
        { 0.5f,  0.5f, -0.5f},    
        { 0.5f,  0.5f,  0.5f},    
        { 0.5f, -0.5f,  0.5f},    

        {-0.5f, -0.5f, -0.5f}, // Bottom   
        {-0.5f, -0.5f,  0.5f},    
        { 0.5f, -0.5f,  0.5f},    
        { 0.5f, -0.5f, -0.5f},    

        {-0.5f,  0.5f, -0.5f}, // Top   
        {-0.5f,  0.5f,  0.5f},    
        { 0.5f,  0.5f,  0.5f},    
        { 0.5f,  0.5f, -0.5f},    
    };

    b->vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .usage = SG_USAGE_IMMUTABLE
    });

    uint16_t indices[] = {
        0,  1,  2,   0,  2,  3,
        6,  5,  4,   7,  6,  4,
        8,  9,  10,  8,  10, 11,
        14, 13, 12,  15, 14, 12,
        16, 17, 18,  16, 18, 19,
        22, 21, 20,  23, 22, 20,
    };

    b->index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(indices),
        .content = indices,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE
    });    

    vec3 normals[24];
    compute_flat_normals(vertices, indices, 36, normals);

    b->normal_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(normals),
        .content = normals,
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
        
        /* Count number of instances to ensure GPU buffers are big enough */
        int32_t count = 0;
        while (ecs_query_next(&qit)) {
            count += qit.count;
        }

        if (!count) {
            b->instance_count = 0;
        } else {
            /* Make sure application buffers are large enough */
            ecs_rgba_t *colors = b->colors;
            mat4 *transforms = b->transforms;
            uint32_t *materials = b->materials;

            int colors_size = count * sizeof(ecs_rgba_t);
            int transforms_size = count * sizeof(EcsTransform3);
            int materials_size = count * sizeof(uint32_t);

            int32_t instance_count = b->instance_count;
            int32_t instance_max = b->instance_max;

            if (instance_count < count) {
                colors = ecs_os_realloc(colors, colors_size);
                transforms = ecs_os_realloc(transforms, transforms_size);
                materials = ecs_os_realloc(materials, materials_size);
            }

            /* Copy data into application buffers */
            int32_t cursor = 0;
            int i;

            ecs_iter_t qit = ecs_query_iter(query);
            while (ecs_query_next(&qit)) {
                EcsTransform3 *t = ecs_column(&qit, EcsTransform3, 1);
                EcsColor *c = ecs_column(&qit, EcsColor, 3);
                SokolMaterial *mat = ecs_column(&qit, SokolMaterial, 4);

                if (ecs_is_owned(&qit, 3)) {
                    memcpy(&colors[cursor], c, qit.count * sizeof(ecs_rgba_t));
                } else {
                    for (i = 0; i < qit.count; i ++) {
                        memcpy(&colors[cursor + i], c, sizeof(ecs_rgba_t));
                    }
                }

                if (mat) {
                    uint16_t material_id = mat->material_id;
                    for (i = 0; i < qit.count; i ++) {
                        materials[cursor + i] = material_id;
                    }
                } else {
                    memset(&materials[cursor], 0, qit.count * sizeof(uint32_t));
                }

                memcpy(&transforms[cursor], t, qit.count * sizeof(mat4));
                action(&qit, cursor, transforms);
                cursor += qit.count;
            }

            b->colors = colors;
            b->transforms = transforms;
            b->materials = materials;
            b->instance_count = count;

            if (count > instance_max) {
                if (instance_max) {
                    /* Make sure buffers are no longer in use (I think- without
                     * this OpenGL complains) */
                    sg_reset_state_cache();
                    
                    sg_destroy_buffer(b->color_buffer);
                    sg_destroy_buffer(b->transform_buffer);
                    sg_destroy_buffer(b->material_buffer);
                }

                while (count > instance_max) {
                    if (!instance_max) {
                        instance_max = 1;
                    }
                    instance_max *= 2;
                }

                b->color_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = instance_max * sizeof(ecs_rgba_t),
                    .usage = SG_USAGE_STREAM
                });

                b->transform_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = instance_max * sizeof(EcsTransform3),
                    .usage = SG_USAGE_STREAM
                });

                b->material_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = instance_max * sizeof(uint32_t),
                    .usage = SG_USAGE_STREAM
                });

                b->instance_max = instance_max;
            }

            sg_update_buffer(b->color_buffer, colors, colors_size);
            sg_update_buffer(b->transform_buffer, transforms, transforms_size);
            sg_update_buffer(b->material_buffer, materials, materials_size);
        }
    }
}

static
void attach_rect(ecs_iter_t *qit, int32_t offset, mat4 *transforms) {
    EcsRectangle *r = ecs_column(qit, EcsRectangle, 2);

    int i;
    if (ecs_is_owned(qit, 2)) {
        for (i = 0; i < qit->count; i ++) {
            vec3 scale = {r[i].width, r[i].height, 1.0};
            glm_scale(transforms[offset + i], scale);
        }
    } else {
        vec3 scale = {r->width, r->height, 1.0};
        for (i = 0; i < qit->count; i ++) {
            glm_scale(transforms[offset + i], scale);
        }
    }
}

static
void attach_box(ecs_iter_t *qit, int32_t offset, mat4 *transforms) {
    EcsBox *b = ecs_column(qit, EcsBox, 2);
    
    int i;
    if (ecs_is_owned(qit, 2)) {
        for (i = 0; i < qit->count; i ++) {
            vec3 scale = {b[i].width, b[i].height, b[i].depth};
            glm_scale(transforms[offset + i], scale);
        }
    } else {
        vec3 scale = {b->width, b->height, b->depth};
        for (i = 0; i < qit->count; i ++) {
            glm_scale(transforms[offset + i], scale);
        }
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
    ECS_IMPORT(world, FlecsSystemsTransform);

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
                "[in]          flecs.components.transform.Transform3,"
                "[in] ANY:     flecs.components.geometry.Rectangle,"
                "[in] ANY:     flecs.components.graphics.Color,"
                "[in] ?SHARED: flecs.systems.sokol.Material")
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
                "[in]          flecs.components.transform.Transform3,"
                "[in] ANY:     flecs.components.geometry.Box,"
                "[in] ANY:     flecs.components.graphics.Color,"
                "[in] ?SHARED: flecs.systems.sokol.Material")
        });

    /* Create system that manages buffers for rectangles */
    ECS_SYSTEM(world, SokolAttachBox, EcsPostLoad, 
        BoxBuffer:flecs.system.Query, 
        BoxBuffer:Buffer);       
}
