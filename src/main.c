#define SOKOL_IMPL
#include "private.h"

static
sokol_resources_t init_resources(void) {
    return (sokol_resources_t){
        .quad = sokol_buffer_quad(),

        .rect = sokol_buffer_rectangle(),
        .rect_indices = sokol_buffer_rectangle_indices(),
        .rect_normals = sokol_buffer_rectangle_normals(),

        .box = sokol_buffer_box(),
        .box_indices = sokol_buffer_box_indices(),
        .box_normals = sokol_buffer_box_normals()
    };
}

static
void SokolSetRenderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    Sdl2Window *window = ecs_column(it, Sdl2Window, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    ecs_entity_t ecs_typeid(SokolRenderer) = ecs_column_entity(it, 3);
    ecs_entity_t ecs_typeid(SokolGeometry) = ecs_column_entity(it, 4);
    ecs_entity_t ecs_typeid(SokolMaterial) = ecs_column_entity(it, 5);

    for (int32_t i = 0; i < it->count; i ++) {
        int w, h;
        SDL_Window *sdl_window = window->window;
        SDL_GLContext ctx = SDL_GL_CreateContext(sdl_window);
        SDL_GL_GetDrawableSize(sdl_window, &w, &h);

        sg_setup(&(sg_desc) {0});
        assert(sg_isvalid());
        ecs_trace_1("sokol initialized");

        sokol_resources_t resources = init_resources();
        sokol_offscreen_pass_t depth_pass = sokol_init_depth_pass(w, h);

        ecs_set(world, it->entities[i], SokolRenderer, {
            .resources = resources,
            .sdl_window = sdl_window,
            .gl_context = ctx,
            .depth_pass = depth_pass,
            .shadow_pass = sokol_init_shadow_pass(SOKOL_SHADOW_MAP_SIZE),
            .scene_pass = sokol_init_scene_pass(canvas[i].background_color, depth_pass.depth_target, w, h),
            .screen_pass = sokol_init_screen_pass(),
            .fx_bloom = sokol_init_bloom(w * 2, h * 2)
        });

        ecs_trace_1("sokol canvas initialized");

        ecs_set_trait(world, it->entities[i], SokolGeometry, EcsQuery, {
            ecs_query_new(world, "[in] flecs.systems.sokol.Geometry")
        });

        ecs_set_trait(world, it->entities[i], SokolMaterial, EcsQuery, {
            ecs_query_new(world, 
                "[in] flecs.systems.sokol.Material,"
                "[in] ?flecs.components.graphics.Specular,"
                "[in] ?flecs.components.graphics.Emissive,"
                "     ?Prefab")
        });

        sokol_init_geometry(world, &resources);

        ecs_trace_1("sokol buffer support initialized");
    }
}

static
void SokolUnsetRenderer(ecs_iter_t *it) {
    SokolRenderer *canvas = ecs_column(it, SokolRenderer, 1);

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        sg_shutdown();
        SDL_GL_DeleteContext(canvas[i].gl_context);
    }
}

static
void SokolRegisterMaterial(ecs_iter_t *it) {
    ecs_entity_t ecs_typeid(SokolMaterial) = ecs_column_entity(it, 1);

    static uint16_t next_material = 1; /* Material 0 is the default material */

    int i;
    for (i = 0; i < it->count; i ++) {
        ecs_set(it->world, it->entities[i], SokolMaterial, {
            next_material ++
        });
    }

    ecs_assert(next_material < SOKOL_MAX_MATERIALS, ECS_INVALID_PARAMETER, NULL);
}

static
void init_materials(
    ecs_query_t *mat_q, 
    sokol_vs_materials_t *mat_u)
{
    ecs_iter_t qit = ecs_query_iter(mat_q);
    while (ecs_query_next(&qit)) {
        SokolMaterial *mat = ecs_column(&qit, SokolMaterial, 1);
        EcsSpecular *spec = ecs_column(&qit, EcsSpecular, 2);
        EcsEmissive *em = ecs_column(&qit, EcsEmissive, 3);

        int i;
        if (spec) {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].specular_power = spec[i].specular_power;
                mat_u->array[id].shininess = spec[i].shininess;
            }
        } else {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].specular_power = 0;
                mat_u->array[id].shininess = 0;
            }            
        }

        if (em) {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].emissive = em[i].value;
            }
        } else {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].emissive = 0;
            }
        }
    }    
}

#define SHADOW_FAR (5)

static
void init_light_mat_vp(
    sokol_render_state_t *state)
{
    mat4 mat_p;
    vec3 lookat = {0.0, 0.0, 0.0};
    vec3 up = {0, 1, 0};

    vec4 dir = {
        state->light->direction[0],
        state->light->direction[1],
        state->light->direction[2]
    };
    // glm_vec4_scale(dir, 10, dir);
    glm_lookat(dir, lookat, up, state->light_mat_v);

    vec3 cam_pos = {0, 4, 4};
    // cam_pos[0] = cos(state->world_time) * 5;
    // cam_pos[1] = 4;
    // cam_pos[2] = sin(state->world_time) * 5;


        // Compute camera frustrum vertices in light space
        ecs_rect_t near, far;
        sokol_get_near_far_rect(
            state->aspect, 
            state->camera->near, 
            SHADOW_FAR, 
            state->camera->fov, 
            &near, &far);

        vec3 verts[8];
        sokol_get_frustrum_verts(
            state->camera->near,
            SHADOW_FAR,
            near,
            far,
            (float*)cam_pos,
            (float*)state->camera->lookat,
            up,
            verts);

        // printf("World = {%f, %f, %f}\n", verts[0][0], verts[0][1], verts[0][2]);

        int i;
        for (i = 0; i < 8; i ++) {
            glm_mat4_mulv3(state->light_mat_v, verts[i], 1, verts[i]);
        }

        // printf("Light = {%f, %f, %f}\n", verts[0][0], verts[0][1], verts[0][2]);

        vec3 first;
        glm_vec3_copy(verts[0], first);
        float min_x = first[0], max_x = first[0];
        float min_y = first[1], max_y = first[1];
        float min_z = first[2], max_z = first[2];

        for (i = 1; i < 8; i ++) {
            float x = verts[i][0];
            min_x = glm_min(min_x, x);
            max_x = glm_max(max_x, x);

            float y = verts[i][1];
            min_y = glm_min(min_y, y);
            max_y = glm_max(max_y, y);

            float z = verts[i][2];
            min_z = glm_min(min_z, z);
            max_z = glm_max(max_z, z);
        }

        float width = (max_x - min_x);
        float height = (max_y - min_y);
        float length = (max_z - min_z);

        // printf("w = %f, h = %f, l = %f (%f)\n", width, height, length, width * height * length);

        vec3 center = {
            (max_x + min_x) / 2.0,
            (max_y + min_y) / 2.0,
            (max_z + min_z) / 2.0
        };

        mat4 inv_light_mat_v;
        glm_mat4_inv(state->light_mat_v, inv_light_mat_v);
        glm_mat4_mulv3(inv_light_mat_v, center, 1, center);

        // printf("P {%f, %f, %f}\n", cam_pos[0], cam_pos[1], cam_pos[2]);
        // printf("C {%f, %f, %f}\n", center[0], center[1], center[2]);



    glm_translate(state->light_mat_v, center);

    // width = 50;
    // height = 50;
    // length = 20;

    glm_ortho(-width, width, -height, height, -length, length, mat_p);

    mat4 light_proj = {
         { 0.5f, 0.0f, 0.0f, 0 },
         { 0.0f, 0.5f, 0.0f, 0 },
         { 0.0f, 0.0f, 0.5f, 0 },
         { 0.5f, 0.5f, 0.5f, 1 }
    };
    
    glm_mat4_mul(mat_p, light_proj, mat_p);
    glm_mat4_mul(mat_p, state->light_mat_v, state->light_mat_vp);
}

static
void SokolRender(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolRenderer *r = ecs_column(it, SokolRenderer, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    EcsQuery *q_buffers = ecs_column(it, EcsQuery, 3);
    EcsQuery *q_mats = ecs_column(it, EcsQuery, 4);
    ecs_entity_t ecs_typeid(EcsCamera) = ecs_column_entity(it, 5);
    ecs_entity_t ecs_typeid(EcsDirectionalLight) = ecs_column_entity(it, 6);
    sokol_render_state_t state = {};
    sokol_vs_materials_t mat_u = {};

    bool mat_set = false;
    if (ecs_query_changed(q_mats->query)) {
        init_materials(q_mats->query, &mat_u);
        mat_set = true;
    }

    state.delta_time = it->delta_time;
    state.world_time = it->world_time;

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        SDL_GL_GetDrawableSize(r[i].sdl_window, &state.width, &state.height);
        state.aspect = (float)state.width / (float)state.height;
        state.q_scene = q_buffers->query;
        state.ambient_light = canvas[i].ambient_light;
        state.shadow_map = r[i].shadow_pass.color_target;

        ecs_entity_t camera = canvas[i].camera;
        if (camera) {
            state.camera = ecs_get(world, camera, EcsCamera);
        }

        ecs_entity_t light = canvas[i].directional_light;
        if (light) {
            state.light = ecs_get(world, light, EcsDirectionalLight);
            init_light_mat_vp(&state);
            // sokol_run_shadow_pass(&r[i].shadow_pass, &state);   
        }

        sokol_run_depth_pass(&r[i].depth_pass, &state, mat_set ? &mat_u : NULL);

        sokol_run_scene_pass(&r[i].scene_pass, &state, mat_set ? &mat_u : NULL);

        sg_image target = sokol_effect_run(
            &r->resources, &r->fx_bloom, r->scene_pass.color_target);

        sokol_run_screen_pass(&r->screen_pass, &r->resources, &state, target);
        
        sg_commit();
        SDL_GL_SwapWindow(r->sdl_window);
    }
}

void FlecsSystemsSokolImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokol);

    ecs_set_name_prefix(world, "Sokol");
    
    ECS_IMPORT(world, FlecsSystemsSdl2);
    ECS_IMPORT(world, FlecsComponentsGui);

    ECS_COMPONENT(world, SokolRenderer);
    ECS_COMPONENT(world, SokolMaterial);

    ECS_IMPORT(world, FlecsSystemsSokolGeometry);

    ECS_SYSTEM(world, SokolSetRenderer, EcsOnSet,
        flecs.systems.sdl2.window.Window,
        flecs.components.gui.Canvas,
        :Renderer,
        :Geometry,
        :Material);

    ECS_SYSTEM(world, SokolUnsetRenderer, EcsUnSet, 
        Renderer);

    ECS_SYSTEM(world, SokolRegisterMaterial, EcsPostLoad,
        [out] !flecs.systems.sokol.Material,
        [in]   flecs.components.graphics.Specular || 
               flecs.components.graphics.Emissive,
               ?Prefab);

    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        Renderer, 
        flecs.components.gui.Canvas, 
        flecs.system.Query FOR Geometry,
        flecs.system.Query FOR Material,
        :flecs.components.graphics.Camera,
        :flecs.components.graphics.DirectionalLight);      
}
