#include "private.h"

ECS_CTOR(SokolGeometry, ptr, {
    *ptr = (SokolGeometry) {};
});

static 
void free_buffer(sokol_instances_t *b) {
    ecs_os_free(b->colors);
    ecs_os_free(b->transforms);
    ecs_os_free(b->materials);
}

ECS_DTOR(SokolGeometry, ptr, {
    free_buffer(&ptr->solid);
    free_buffer(&ptr->emissive);
    free_buffer(&ptr->transparent);
});

static
void populate_rectangle(ecs_iter_t *qit, int32_t offset, mat4 *transforms) {
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
void populate_box(ecs_iter_t *qit, int32_t offset, mat4 *transforms) {
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
void init_rect(
    ecs_world_t *world,
    ecs_entity_t geometry,
    sokol_resources_t *resources) 
{
    ecs_entity_t rect = ecs_lookup_fullpath(
        world, "flecs.systems.sokol.RectangleGeometry");
    ecs_assert(rect != 0, ECS_INTERNAL_ERROR, NULL);

    SokolGeometry *g = ecs_get_mut_w_entity(world, rect, geometry, NULL);
    ecs_assert(g != NULL, ECS_INTERNAL_ERROR, NULL);

    g->vertex_buffer = resources->rect;
    g->normal_buffer = resources->rect_normals;
    g->index_buffer = resources->rect_indices;
    g->index_count = sokol_rectangle_index_count();
    g->populate = populate_rectangle;
}

static
void init_box(
    ecs_world_t *world,
    ecs_entity_t geometry,
    sokol_resources_t *resources) 
{
    ecs_entity_t box = ecs_lookup_fullpath(
        world, "flecs.systems.sokol.BoxGeometry");
    ecs_assert(box != 0, ECS_INTERNAL_ERROR, NULL);

    SokolGeometry *g = ecs_get_mut_w_entity(world, box, geometry, NULL);
    ecs_assert(g != NULL, ECS_INTERNAL_ERROR, NULL);

    g->vertex_buffer = resources->box;
    g->normal_buffer = resources->box_normals;
    g->index_buffer = resources->box_indices;
    g->index_count = sokol_box_index_count();
    g->populate = populate_box;
}

void sokol_init_geometry(
    ecs_world_t *world,
    sokol_resources_t *resources) 
{
    ecs_entity_t geometry = ecs_lookup_fullpath(
        world, "flecs.systems.sokol.Geometry");
    ecs_assert(geometry != 0, ECS_INTERNAL_ERROR, NULL);
    
    init_rect(world, geometry, resources);
    init_box(world, geometry, resources);
}

static
void populate_buffer(
    SokolGeometry *geometry,
    sokol_instances_t *instances,
    ecs_query_t *query)
{
    if (ecs_query_changed(query)) {
        ecs_iter_t qit = ecs_query_iter(query);

        /* Count number of instances to ensure GPU buffers are big enough */
        int32_t count = 0;
        while (ecs_query_next(&qit)) {
            count += qit.count;
        }

        if (!count) {
            instances->instance_count = 0;
        } else {
            /* Make sure application buffers are large enough */
            ecs_rgba_t *colors = instances->colors;
            mat4 *transforms = instances->transforms;
            uint32_t *materials = instances->materials;

            int colors_size = count * sizeof(ecs_rgba_t);
            int transforms_size = count * sizeof(EcsTransform3);
            int materials_size = count * sizeof(uint32_t);

            int32_t instance_count = instances->instance_count;
            int32_t instance_max = instances->instance_max;

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
                SokolMaterial *mat = ecs_column(&qit, SokolMaterial, 3);
                EcsRgb *c = ecs_column(&qit, EcsRgb, 4);

                if (ecs_is_owned(&qit, 4)) {
                    for (i = 0; i < qit.count; i ++) {
                        colors[cursor + i].r = c[i].value.r;
                        colors[cursor + i].g = c[i].value.g;
                        colors[cursor + i].b = c[i].value.b;
                        colors[cursor + i].a = 0;
                    }
                } else {
                    for (i = 0; i < qit.count; i ++) {
                        colors[cursor + i].r = c->value.r;
                        colors[cursor + i].g = c->value.g;
                        colors[cursor + i].b = c->value.b;
                        colors[cursor + i].a = 0;
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
                geometry->populate(&qit, cursor, transforms);
                cursor += qit.count;
            }

            instances->colors = colors;
            instances->transforms = transforms;
            instances->materials = materials;
            instances->instance_count = count;

            if (count > instance_max) {
                if (instance_max) {                    
                    sg_destroy_buffer(instances->color_buffer);
                    sg_destroy_buffer(instances->transform_buffer);
                    sg_destroy_buffer(instances->material_buffer);
                }

                while (count > instance_max) {
                    if (!instance_max) {
                        instance_max = 1;
                    }
                    instance_max *= 2;
                }

                instances->color_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = instance_max * sizeof(ecs_rgba_t),
                    .usage = SG_USAGE_STREAM
                });

                instances->transform_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = instance_max * sizeof(EcsTransform3),
                    .usage = SG_USAGE_STREAM
                });

                instances->material_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = instance_max * sizeof(uint32_t),
                    .usage = SG_USAGE_STREAM
                });

                instances->instance_max = instance_max;
            }

            sg_update_buffer(instances->color_buffer, colors, colors_size);
            sg_update_buffer(instances->transform_buffer, transforms, transforms_size);
            sg_update_buffer(instances->material_buffer, materials, materials_size);
        }
    }
}

static
void SokolPopulateGeometry(
    ecs_iter_t *it) 
{
    SokolGeometry *g = ecs_column(it, SokolGeometry, 1);
    SokolGeometryQuery *q = ecs_column(it, SokolGeometryQuery, 2);

    int i;
    for (i = 0; i < it->count; i ++) {
        populate_buffer(&g[i], &g[i].solid, q[i].solid);
        populate_buffer(&g[i], &g[i].emissive, q[i].emissive);
        populate_buffer(&g[i], &g[i].transparent, q[i].transparent);
    }
}

static 
void CreateGeometryQueries(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolGeometryQuery *sb = ecs_column(it, SokolGeometryQuery, 1);

    int i;
    for (i = 0; i < it->count; i ++) {
        char *comp_path = ecs_get_fullpath(world, sb[i].component);
        char expr[255], subexpr[512];
        sprintf(expr, 
            "[in] flecs.components.transform.Transform3,"
            "[in] ANY:%s,"
            "[in] ?SHARED:flecs.systems.sokol.Material",
                comp_path);
        ecs_os_free(comp_path);

        sb[i].parent_query = ecs_query_new(world, expr);

        sprintf(subexpr, 
            "%s,"
            "[in] ANY:flecs.components.graphics.Rgb,"
            "[in] !SHARED:flecs.components.graphics.Emissive,"
            "[in] !ANY:flecs.components.graphics.Rgba", 
                expr);
        sb[i].solid = ecs_subquery_new(world, sb[i].parent_query, subexpr);

        sprintf(subexpr, 
            "%s,"
            "[in] ANY:flecs.components.graphics.Rgb,"
            "[in] SHARED:flecs.components.graphics.Emissive,"
            "[in] !ANY:flecs.components.graphics.Rgba", expr);
        sb[i].emissive = ecs_subquery_new(world, sb[i].parent_query, subexpr);

        sprintf(subexpr,
            "%s,"
            "[in] !ANY:flecs.components.graphics.Rgb,"
            "[in] ANY:flecs.components.graphics.Emissive,"
            "[in] ANY:flecs.components.graphics.Rgba", expr);
        sb[i].transparent = ecs_subquery_new(world, sb[i].parent_query, subexpr);
    }
}

void FlecsSystemsSokolGeometryImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokolGeometry);

    /* Store components in parent sokol scope */
    ecs_entity_t scope = ecs_lookup_fullpath(world, "flecs.systems.sokol");
    ecs_set_scope(world, scope);

    ECS_IMPORT(world, FlecsComponentsTransform);
    ECS_IMPORT(world, FlecsComponentsGeometry);
    ECS_IMPORT(world, FlecsSystemsTransform);

    ecs_set_name_prefix(world, "Sokol");

    ECS_COMPONENT(world, SokolGeometry);
    ECS_COMPONENT(world, SokolGeometryQuery);

    ecs_set_component_actions(world, SokolGeometry, {
        .ctor = ecs_ctor(SokolGeometry),
        .dtor = ecs_dtor(SokolGeometry)
    });

    /* Create queries for solid, emissive and transparent */
    ECS_SYSTEM(world, CreateGeometryQueries, EcsOnSet, GeometryQuery);

    /* Support for rectangle primitive */
    ECS_ENTITY(world, SokolRectangleGeometry, Geometry);
        ecs_set(world, SokolRectangleGeometry, SokolGeometryQuery, {
            .component = ecs_typeid(EcsRectangle)
        });

    /* Support for box primitive */
    ECS_ENTITY(world, SokolBoxGeometry, Geometry);
        ecs_set(world, SokolBoxGeometry, SokolGeometryQuery, {
            .component = ecs_typeid(EcsBox)
        });

    /* Create system that manages buffers for rectangles */
    ECS_SYSTEM(world, SokolPopulateGeometry, EcsPostLoad, 
        Geometry, [in] GeometryQuery);       
}
