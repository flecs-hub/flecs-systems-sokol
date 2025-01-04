#include "geometry.h"

ECS_COMPONENT_DECLARE(SokolGeometry);
ECS_COMPONENT_DECLARE(SokolGeometryQuery);

ECS_DECLARE(SokolRectangleGeometry);
ECS_DECLARE(SokolBoxGeometry);

static
void sokol_geometry_buffers_init(ecs_allocator_t *a, sokol_geometry_buffers_t *result) {
    ecs_vec_init_t(a, &result->transforms_data, mat4, 0);
    ecs_vec_init_t(a, &result->colors_data, ecs_rgb_t, 0);
    ecs_vec_init_t(a, &result->materials_data, SokolMaterial, 0);
}

static
void sokol_geometry_buffers_fini(ecs_allocator_t *a, sokol_geometry_buffers_t* result) {
    if (result->colors.id) {
        sg_destroy_buffer(result->colors);
    }
    if (result->transforms.id) {
        sg_destroy_buffer(result->transforms);
    }
    if (result->materials.id) {
        sg_destroy_buffer(result->materials);
    }

    ecs_vec_fini_t(a, &result->transforms_data, mat4);
    ecs_vec_fini_t(a, &result->colors_data, ecs_rgb_t);
    ecs_vec_fini_t(a, &result->materials_data, SokolMaterial);
}

static
void sokol_free_geometry(SokolGeometry *ptr) {
    sokol_geometry_buffers_fini(ptr->allocator, &ptr->solid);
    if (ptr->allocator) {
        flecs_allocator_fini(ptr->allocator);
        ecs_os_free(ptr->allocator);
        ptr->allocator = NULL;
    }
}

ECS_CTOR(SokolGeometry, ptr, {
    ecs_os_memset_t(ptr, 0, SokolGeometry);
    ptr->allocator = ecs_os_malloc_t(ecs_allocator_t);
    
    flecs_allocator_init(ptr->allocator);
    flecs_allocator_fini(ptr->allocator);
    flecs_allocator_init(ptr->allocator);


    sokol_geometry_buffers_init(ptr->allocator, &ptr->solid);
})

ECS_MOVE(SokolGeometry, dst, src, {
    sokol_free_geometry(dst);
    ecs_os_memcpy_t(dst, src, SokolGeometry);
    ecs_os_memset_t(src, 0, SokolGeometry);
})

ECS_DTOR(SokolGeometry, ptr, {
    sokol_free_geometry(ptr);
})

// To ensure rectangles are of the right size, use the Rectangle component to
// apply a scaling factor to the transform matrix that is sent to the GPU.
static
void sokol_populate_rectangle(
    mat4 *transforms, 
    EcsRectangle *data, 
    int32_t count,
    bool self) 
{
    int i;
    if (self) {
        for (i = 0; i < count; i ++) {
            vec3 scale = {data[i].width, data[i].height, 1.0};
            glm_scale(transforms[i], scale);
        }
    } else {
        vec3 scale = {data->width, data->height, 1.0};
        for (i = 0; i < count; i ++) {
            glm_scale(transforms[i], scale);
        }
    }
}

// To ensure boxes are of the right size, use the Box component to apply
// a scaling factor to the transform matrix that is sent to the GPU.
static
void sokol_populate_box(
    mat4 *transforms, 
    EcsBox *data, 
    int32_t count,
    bool self)
     
{    
    int i;
    if (self) {
        for (i = 0; i < count; i ++) {
            vec3 scale = {data[i].width, data[i].height, data[i].depth};
            glm_scale(transforms[i], scale);
        }
    } else {
        vec3 scale = {data->width, data->height, data->depth};
        for (i = 0; i < count; i ++) {
            glm_scale(transforms[i], scale);
        }
    }
}

// Init static rectangle geometry data (vertices, indices)
static
void sokol_init_rectangle(
    ecs_world_t *world,
    sokol_resources_t *resources) 
{
    SokolGeometry *g = ecs_get_mut(
        world, ecs_id(SokolRectangleGeometry), SokolGeometry);
    ecs_assert(g != NULL, ECS_INTERNAL_ERROR, NULL);

    g->vertices = resources->rect;
    g->normals = resources->rect_normals;
    g->indices = resources->rect_indices;
    g->index_count = sokol_rectangle_index_count();
    g->populate = (sokol_geometry_action_t)sokol_populate_rectangle;
}

// Init static box geometry data (vertices, indices)
static
void sokol_init_box(
    ecs_world_t *world,
    sokol_resources_t *resources) 
{
    if (SokolBoxGeometry) {
        SokolGeometry *g = ecs_get_mut(
            world, ecs_id(SokolBoxGeometry), SokolGeometry);
        ecs_assert(g != NULL, ECS_INTERNAL_ERROR, NULL);

        g->vertices = resources->box;
        g->normals = resources->box_normals;
        g->indices = resources->box_indices;
        g->index_count = sokol_box_index_count();
        g->populate = (sokol_geometry_action_t)sokol_populate_box;
    }
}

void sokol_init_geometry(
    ecs_world_t *world,
    sokol_resources_t *resources) 
{
    sokol_init_rectangle(world, resources);
    sokol_init_box(world, resources);
}

static
void sokol_populate_buffers(
    SokolGeometry *geometry,
    sokol_geometry_buffers_t *buffers,
    ecs_query_t *query)
{
    const ecs_world_t *world = ecs_get_world(query);
    ecs_allocator_t *a = geometry->allocator;

    int32_t i, old_size = ecs_vec_size(&buffers->colors_data);

    ecs_vec_clear(&buffers->transforms_data);
    ecs_vec_clear(&buffers->colors_data);
    ecs_vec_clear(&buffers->materials_data);

    ecs_iter_t qit = ecs_query_iter(world, query);
    while (ecs_query_next(&qit)) {
        EcsTransform3 *transforms = ecs_field(&qit, EcsTransform3, 0);
        EcsRgb *colors = ecs_field(&qit, EcsRgb, 1);
        EcsEmissive *emissive = ecs_field(&qit, EcsEmissive, 2);
        EcsSpecular *specular = ecs_field(&qit, EcsSpecular, 3);
        void *geometry_data = ecs_field_w_size(&qit, 0, 4);
        bool geometry_self = ecs_field_is_self(&qit, 4);

        int32_t cur = ecs_vec_count(&buffers->colors_data);
        int32_t count = qit.count;

        ecs_vec_grow_t(a, &buffers->transforms_data, mat4, count);
        ecs_vec_grow_t(a, &buffers->colors_data, ecs_rgb_t, count);
        ecs_vec_grow_t(a, &buffers->materials_data, SokolMaterial, count);

        // Copy transform data
        ecs_os_memcpy_n( ecs_vec_get_t(&buffers->transforms_data, mat4, cur), 
            transforms, mat4, count);

        // Copy color data
        if (ecs_field_is_self(&qit, 1)) {
            ecs_os_memcpy_n( ecs_vec_get_t(&buffers->colors_data, ecs_rgb_t, cur), 
                colors, ecs_rgb_t, count);
        } else {
            for (i = 0; i < count; i ++) {
                *ecs_vec_get_t(&buffers->colors_data, ecs_rgb_t, cur + i) = colors[0];
            }
        }

        if (emissive || specular) {
            SokolMaterial *m = ecs_vec_get_t(
                &buffers->materials_data, SokolMaterial, cur);
            if (emissive) {
                if (ecs_field_is_self(&qit, 2)) {
                    for (i = 0; i < count; i ++) {
                        m[i].emissive = emissive[i].value;
                    }
                } else {
                    for (i = 0; i < count; i ++) {
                        m[i].emissive = emissive->value;
                    }
                }
            } else {
                for (i = 0; i < count; i ++) {
                    m[i].emissive = 0;
                }
            }

            if (specular) {
                if (ecs_field_is_self(&qit, 3)) {
                    for (i = 0; i < count; i ++) {
                        m[i].specular_power = specular[i].specular_power;
                        m[i].shininess = specular[i].shininess;
                    }
                } else {
                    for (i = 0; i < count; i ++) {
                        m[i].specular_power = specular->specular_power;
                        m[i].shininess = specular->shininess;
                    }
                }
            } else {
                for (i = 0; i < count; i ++) {
                    m[i].specular_power = 0;
                    m[i].shininess = 0;
                }
            }
        } else {
            ecs_os_memset_n(
                ecs_vec_get_t(&buffers->materials_data, SokolMaterial, cur), 
                    0, SokolMaterial, count);
        }

        // Apply geometry-specific scaling to transform matrix
        geometry->populate(ecs_vec_get_t(&buffers->transforms_data, mat4, cur), 
            geometry_data, count, geometry_self);
    }

    // Populate sokol buffers
    {
        ecs_size_t new_size = ecs_vec_size(&buffers->colors_data);
        if (new_size != old_size) {
            if (old_size) {
                sg_destroy_buffer(buffers->colors);
                sg_destroy_buffer(buffers->transforms);
                sg_destroy_buffer(buffers->materials);
            }

            buffers->colors = sg_make_buffer(&(sg_buffer_desc){
                .size = new_size * sizeof(ecs_rgb_t), .usage = SG_USAGE_STREAM });
            buffers->transforms = sg_make_buffer(&(sg_buffer_desc){
                .size = new_size * sizeof(EcsTransform3), .usage = SG_USAGE_STREAM });
            buffers->materials = sg_make_buffer(&(sg_buffer_desc){
                .size = new_size * sizeof(SokolMaterial), .usage = SG_USAGE_STREAM });
        }

        buffers->instance_count = ecs_vec_count(&buffers->colors_data);

        if (buffers->instance_count > 0) {
            sg_update_buffer(buffers->colors, &(sg_range) {
                ecs_vec_first_t(&buffers->colors_data, ecs_rgb_t), 
                    buffers->instance_count * sizeof(ecs_rgb_t) } );
            sg_update_buffer(buffers->transforms, &(sg_range) {
                ecs_vec_first_t(&buffers->transforms_data, mat4), 
                    buffers->instance_count * sizeof(mat4) } );
            sg_update_buffer(buffers->materials, &(sg_range) {
                ecs_vec_first_t(&buffers->materials_data, SokolMaterial), 
                    buffers->instance_count * sizeof(SokolMaterial) } );
        }
    }
}

// System that matches all geometry kinds & calls the function to update GPU
// buffers with ECS data.
static
void SokolPopulateGeometry(
    ecs_iter_t *it) 
{
    SokolGeometry *g = ecs_field(it, SokolGeometry, 0);
    SokolGeometryQuery *q = ecs_field(it, SokolGeometryQuery, 1);

    int i;
    for (i = 0; i < it->count; i ++) {
        sokol_populate_buffers(&g[i], &g[i].solid, q[i].solid);    
    }
}

static
void CreateGeometryQueries(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolGeometryQuery *gq = ecs_field(it, SokolGeometryQuery, 1);

    int i;
    for (i = 0; i < it->count; i ++) {
        // Geometry query that includes all components that are copied (or used
        // to find data to copy) to GPU buffers.
        ecs_query_desc_t desc = {
            .terms = {{
                .id        = ecs_id(EcsTransform3), 
                .src.id    = EcsSelf,
                .inout     = EcsIn
            }, {
                .id        = ecs_id(EcsRgb),
                .inout     = EcsIn
            }, {
                .id        = ecs_id(EcsEmissive),
                .inout     = EcsInOutNone,
                .oper      = EcsOptional
            }, {
                .id        = ecs_id(EcsSpecular),
                .inout     = EcsInOutNone,
                .oper      = EcsOptional
            }, {
                .id        = gq[i].component, 
                .inout     = EcsIn
            }, {
                .id        = ecs_id(EcsPosition3),
                .src.id    = EcsSelf,
                .inout     = EcsInOutNone,
            }},
            .cache_kind = EcsQueryCacheAuto
        };

        /* Query for solid objects */
        desc.entity = ecs_entity(world, {
            .name = ecs_get_name(world, gq[i].component),
            .parent = ecs_entity(world, {
                .name = "#0.flecs.systems.sokol.geometry_queries.solid"
            })
        });

        gq[i].solid = ecs_query_init(world, &desc);
        if (!gq[i].solid) {
            char *component_str = ecs_id_str(world, gq[i].component);
            ecs_err("sokol: failed to create query for solid %s geometry", 
                component_str);
            ecs_os_free(component_str);
        }
    }
}

void FlecsSystemsSokolGeometryImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokolGeometry);
    ECS_IMPORT(world, FlecsComponentsTransform);
    ECS_IMPORT(world, FlecsComponentsGeometry);
    ECS_IMPORT(world, FlecsSystemsTransform);
    ECS_IMPORT(world, FlecsGame);

    /* Store components in parent sokol scope */
    ecs_entity_t parent = ecs_lookup(world, "flecs.systems.sokol");
    ecs_entity_t module = ecs_set_scope(world, parent);
    ecs_set_name_prefix(world, "Sokol");

    ECS_COMPONENT_DEFINE(world, SokolGeometry);
    ECS_COMPONENT_DEFINE(world, SokolGeometryQuery);

    ecs_set_hooks(world, SokolGeometry, {
        .ctor = ecs_ctor(SokolGeometry),
        .move = ecs_move(SokolGeometry),
        .dtor = ecs_dtor(SokolGeometry)
    });

    ecs_set_scope(world, module);

    /* Create queries for solid objects */
    ECS_OBSERVER(world, CreateGeometryQueries, EcsOnSet, 
        Geometry, GeometryQuery);

    /* Support for rectangle primitive */
    ECS_ENTITY_DEFINE(world, SokolRectangleGeometry, Geometry);
        ecs_set(world, SokolRectangleGeometry, SokolGeometryQuery, {
            .component = ecs_id(EcsRectangle)
        });

    /* Support for box primitive */
    ECS_ENTITY_DEFINE(world, SokolBoxGeometry, Geometry);
        ecs_set(world, SokolBoxGeometry, SokolGeometryQuery, {
            .component = ecs_id(EcsBox)
        });

    /* Create system that manages buffers */
    ECS_SYSTEM(world, SokolPopulateGeometry, EcsPreStore, 
        Geometry, [in] GeometryQuery);
}
