#define SOKOL_IMPL
#include "private.h"

static
sg_pipeline init_screen_pipeline() {
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source = sokol_vs_passthrough(),
        .fs = {
            .source =
                "#version 330\n"
                "uniform sampler2D tex;\n"
                "out vec4 frag_color;\n"
                "in vec2 uv;\n"
                "void main() {\n"
                "  frag_color = texture(tex, uv);\n"
                "}\n"
                ,
            .images[0] = {
                .name = "screen",
                .type = SG_IMAGETYPE_2D
            }
        }
    });

    /* create a pipeline object (default render state is fine) */
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {         
            .attrs = {
                /* Static geometry (position, uv) */
                [0] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT2 }
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = false
        }
    });
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
sokol_screen_pass_t init_screen_pass(void) {
    return (sokol_screen_pass_t){
        .pass_action = sokol_clear_action((ecs_rgb_t){0, 0, 0}),
        .pip = init_screen_pipeline()
    };
}

static
void SokolSetRenderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    Sdl2Window *window = ecs_column(it, Sdl2Window, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    ecs_entity_t ecs_entity(SokolRenderer) = ecs_column_entity(it, 3);
    ecs_entity_t ecs_entity(SokolGeometry) = ecs_column_entity(it, 4);
    ecs_entity_t ecs_entity(SokolMaterial) = ecs_column_entity(it, 5);

    for (int32_t i = 0; i < it->count; i ++) {
        int w, h;
        SDL_Window *sdl_window = window->window;
        SDL_GLContext ctx = SDL_GL_CreateContext(sdl_window);
        SDL_GL_GetDrawableSize(sdl_window, &w, &h);

        sg_setup(&(sg_desc) {0});
        assert(sg_isvalid());
        ecs_trace_1("sokol initialized");

        sokol_resources_t resources = init_resources();
        
        ecs_set(world, it->entities[i], SokolRenderer, {
            .resources = resources,
            .sdl_window = sdl_window,
            .gl_context = ctx,
            .shadow_pass = sokol_init_shadow_pass(SOKOL_SHADOW_MAP_SIZE),
            .scene_pass = sokol_init_scene_pass(canvas[i].background_color, w, h),
            .screen_pass = init_screen_pass(),
            .fx_bloom = sokol_init_bloom(w, h)
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
    ecs_entity_t ecs_entity(SokolMaterial) = ecs_column_entity(it, 1);

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
void init_light_vp(
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
    SokolRenderer *r = ecs_column(it, SokolRenderer, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    EcsQuery *q_buffers = ecs_column(it, EcsQuery, 3);
    EcsQuery *q_mats = ecs_column(it, EcsQuery, 4);
    ecs_entity_t ecs_entity(EcsCamera) = ecs_column_entity(it, 5);
    ecs_entity_t ecs_entity(EcsDirectionalLight) = ecs_column_entity(it, 6);

    ecs_query_t *q_geometry = q_buffers->query;
    sokol_vs_materials_t mat_u = {};

    /* Update material buffer when changed */
    bool set_mat = false;
    if (ecs_query_changed(q_mats->query)) {
        init_materials(q_mats->query, &mat_u);
        set_mat = true;
    }

    /* Loop each canvas */
    int32_t i;
    for (i = 0; i < it->count; i ++) {
        sokol_render_state_t state = {};
        SDL_GL_GetDrawableSize(r[i].sdl_window, &state.width, &state.height);
        state.aspect = (float)state.width / (float)state.height;
        state.q_scene = q_geometry;
        state.ambient_light = canvas[i].ambient_light;
        state.shadow_map = r[i].shadow_pass.color_target;

        ecs_entity_t camera = canvas[i].camera;
        if (camera) {
            state.camera = ecs_get(world, camera, EcsCamera);
        }

        ecs_entity_t light = canvas[i].directional_light;
        if (light) {
            state.light = ecs_get(world, light, EcsDirectionalLight);
            init_light_vp(&state);
            
            /* Render shadow map only when we have a light */
            sokol_run_shadow_pass(&r[i].shadow_pass, &state);
        }

        sokol_run_scene_pass(&r[i].scene_pass, &state, set_mat ? &mat_u : NULL);

        /* Apply bloom effect */
        sg_image target = sokol_effect_run(
            &r->resources, &r->fx_bloom, r->scene_pass.color_target);

        /* Render resulting offscreen texture to screen */
        sg_begin_default_pass(&r->screen_pass.pass_action, state.width, state.height);
        sg_apply_pipeline(r->screen_pass.pip);

        sg_bindings bind = {
            .vertex_buffers = { 
                [0] = r->resources.quad 
            },
            .fs_images[0] = target
        };

        sg_apply_bindings(&bind);
        sg_draw(0, 6, 1);
        sg_end_pass();
        
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
