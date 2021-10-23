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
    Sdl2Window *window = ecs_term(it, Sdl2Window, 1);
    EcsCanvas *canvas = ecs_term(it, EcsCanvas, 2);

    for (int32_t i = 0; i < it->count; i ++) {
        ecs_trace("#[bold]initializing sokol renderer");
        ecs_log_push();

        int w, h;
        SDL_Window *sdl_window = window->window;
        SDL_GLContext ctx = SDL_GL_CreateContext(sdl_window);
        SDL_GL_GetDrawableSize(sdl_window, &w, &h);

        sg_setup(&(sg_desc) {0});
        assert(sg_isvalid());
        ecs_trace("library initialized");

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
            .fx_bloom = sokol_init_bloom(w * 2, h * 2),
            .fx_fog = sokol_init_fog(w, h)
        });
        ecs_trace("canvas initialized");

        ecs_set_pair(world, it->entities[i], EcsQuery, ecs_id(SokolGeometry), {
            ecs_query_new(world, "[in] flecs.systems.sokol.Geometry")
        });

        ecs_set_pair(world, it->entities[i], EcsQuery, ecs_id(SokolMaterial), {
            ecs_query_new(world, 
                "[in] flecs.systems.sokol.Material,"
                "[in] ?flecs.components.graphics.Specular,"
                "[in] ?flecs.components.graphics.Emissive,"
                "     ?Prefab")
        });

        sokol_init_geometry(world, &resources);
        ecs_trace("static geometry initialized");

        ecs_log_pop();
    }
}

static
void SokolUnsetRenderer(ecs_iter_t *it) {
    SokolRenderer *canvas = ecs_term(it, SokolRenderer, 1);

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        sg_shutdown();
        SDL_GL_DeleteContext(canvas[i].gl_context);
    }
}

static
void SokolRegisterMaterial(ecs_iter_t *it) {
    ecs_entity_t ecs_id(SokolMaterial) = ecs_term_id(it, 1);

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
    const ecs_world_t *world = ecs_get_world(mat_q);
    ecs_iter_t qit = ecs_query_iter(world, mat_q);
    while (ecs_query_next(&qit)) {
        SokolMaterial *mat = ecs_term(&qit, SokolMaterial, 1);
        EcsSpecular *spec = ecs_term(&qit, EcsSpecular, 2);
        EcsEmissive *em = ecs_term(&qit, EcsEmissive, 3);

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
    mat4 mat_v;
    vec3 lookat = {0.0, 0.0, 0.0};
    vec3 up = {0, 1, 0};

    glm_ortho(-7, 7, -7, 7, -10, 30, mat_p);

    vec4 dir = {
        state->light->direction[0],
        state->light->direction[1],
        state->light->direction[2]
    };
    glm_vec4_scale(dir, 50, dir);
    glm_lookat(dir, lookat, up, mat_v);

    mat4 light_proj = {
         { 0.5f, 0.0f, 0.0f, 0 },
         { 0.0f, 0.5f, 0.0f, 0 },
         { 0.0f, 0.0f, 0.5f, 0 },
         { 0.5f, 0.5f, 0.5f, 1 }
    };
    
    glm_mat4_mul(mat_p, light_proj, mat_p);
    glm_mat4_mul(mat_p, mat_v, state->light_mat_vp);
}

static
void SokolRender(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolRenderer *r = ecs_term(it, SokolRenderer, 1);
    EcsCanvas *canvas = ecs_term(it, EcsCanvas, 2);
    EcsQuery *q_buffers = ecs_term(it, EcsQuery, 3);
    EcsQuery *q_mats = ecs_term(it, EcsQuery, 4);
    sokol_render_state_t state = {};
    sokol_vs_materials_t mat_u = {};

    bool mat_set = false;
    if (ecs_query_changed(q_mats->query)) {
        init_materials(q_mats->query, &mat_u);
        mat_set = true;
    }

    state.delta_time = it->delta_time;

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        SDL_GL_GetDrawableSize(r[i].sdl_window, &state.width, &state.height);
        state.aspect = (float)state.width / (float)state.height;
        state.world = world;
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
            sokol_run_shadow_pass(&r[i].shadow_pass, &state);   
        } else if (!state.ambient_light.r && !state.ambient_light.g && 
                   !state.ambient_light.b) 
        {
            state.ambient_light = (EcsRgb){1.0, 1.0, 1.0};
        }

        sokol_run_depth_pass(&r[i].depth_pass, &state);
        sokol_run_scene_pass(&r[i].scene_pass, &state, mat_set ? &mat_u : NULL);

        sg_image target = sokol_effect_run(
            &r->resources, &r->fx_bloom, 1, (sg_image[]){r->scene_pass.color_target});

        target = sokol_effect_run(
            &r->resources, &r->fx_fog, 2, (sg_image[]){target, r->depth_pass.color_target});

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

    ECS_COMPONENT_DEFINE(world, SokolRenderer);
    ECS_COMPONENT_DEFINE(world, SokolMaterial);

    ECS_IMPORT(world, FlecsSystemsSokolGeometry);

    ECS_OBSERVER(world, SokolSetRenderer, EcsOnSet,
        flecs.systems.sdl2.window.Window,
        flecs.components.gui.Canvas);

    ECS_OBSERVER(world, SokolUnsetRenderer, EcsUnSet, 
        Renderer);

    ECS_SYSTEM(world, SokolRegisterMaterial, EcsPostLoad,
        [out] !flecs.systems.sokol.Material,
        [in]   flecs.components.graphics.Specular || 
               flecs.components.graphics.Emissive,
               ?Prefab);

    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        Renderer, 
        flecs.components.gui.Canvas, 
        (flecs.core.Query, Geometry),
        (flecs.core.Query, Material));
}
