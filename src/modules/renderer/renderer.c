#include "../../private_api.h"

ECS_COMPONENT_DECLARE(SokolRenderer);

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
    glm_mat4_mul(mat_p, mat_v, state->uniforms.light_mat_vp);
}

static
void init_global_uniforms(
    sokol_render_state_t *state)
{
    vec3 eye = {0, 0, -2.0};
    vec3 center = {0.0, 0.0, 0.0};
    vec3 up = {0.0, 1.0, 0.0};

    mat4 mat_p;
    mat4 mat_v;

    /* Compute perspective & lookat matrix */
    if (state->camera) {
        EcsCamera cam = *state->camera;
        if (!cam.fov) {
            cam.fov = 30;
        }

        if (!cam.near && !cam.far) {
            cam.near = 1.5;
            cam.far = 2000;
        }

        if (!cam.up[0] && !cam.up[1] && !cam.up[2]) {
            cam.up[1] = 1.0;
        }

        if (!cam.ortho) {
            glm_perspective(cam.fov, state->aspect, cam.near, cam.far, mat_p);
        } else {
            glm_ortho_default(state->aspect, mat_p);
        }

        glm_lookat(cam.position, cam.lookat, cam.up, mat_v);
        glm_vec3_copy(cam.position, state->uniforms.eye_pos);
    } else {
        glm_perspective(30, state->aspect, 0.5, 100.0, mat_p);
        glm_lookat(eye, center, up, mat_v);
        glm_vec3_copy(eye, state->uniforms.eye_pos);
    }

    /* Compute view/projection matrix */
    glm_mat4_mul(mat_p, mat_v, state->uniforms.mat_vp);

    /* Get light parameters */
    if (state->light) {
        EcsDirectionalLight l = *state->light;
        glm_vec3_copy(l.direction, state->uniforms.light_direction);
        glm_vec3_copy(l.color, state->uniforms.light_color);
    } else {
        glm_vec3_zero(state->uniforms.light_direction);
        glm_vec3_zero(state->uniforms.light_color);
    }

    glm_vec3_copy((float*)&state->ambient_light, state->uniforms.light_ambient);

    state->uniforms.shadow_map_size = SOKOL_SHADOW_MAP_SIZE;
}

/* Render */
static
void SokolRender(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolRenderer *r = ecs_term(it, SokolRenderer, 1);
    EcsQuery *q_buffers = ecs_term(it, EcsQuery, 2);
    sokol_render_state_t state = {0};
    sokol_fx_resources_t *fx = r->fx;

    if (it->count > 1) {
        ecs_err("sokol: multiple canvas instances unsupported");
    }

    /* Initialize renderer state */
    state.delta_time = it->delta_time;
    state.width = sapp_width();
    state.height = sapp_height();
    state.aspect = (float)state.width / (float)state.height;
    state.world = world;
    state.q_scene = q_buffers->query;
    state.shadow_map = r->shadow_pass.color_target;
    state.resources = &r->resources;

    /* Load active camera & light data from canvas */
    const EcsCanvas *canvas = ecs_get(world, r->canvas, EcsCanvas);
    if (canvas->camera) {
        state.camera = ecs_get(world, canvas->camera, EcsCamera);
    }

    state.ambient_light = canvas->ambient_light;

    if (canvas->directional_light) {
        state.light = ecs_get(world, canvas->directional_light, 
            EcsDirectionalLight);
        init_light_mat_vp(&state);
        sokol_run_shadow_pass(&r->shadow_pass, &state);  
    } else {
        /* Set default ambient light if nothing is configured */
        if (!state.ambient_light.r && !state.ambient_light.g && 
            !state.ambient_light.b) 
        {
            state.ambient_light = (EcsRgb){1.0, 1.0, 1.0};
        }
    }

    /* Compute uniforms that are shared between passes */
    init_global_uniforms(&state);

    /* Depth prepass for more efficient drawing */
    sokol_run_depth_pass(&r->depth_pass, &state);

    /* Render scene */
    sokol_run_scene_pass(&r->scene_pass, &state);
    sg_image hdr = r->scene_pass.color_target;

    /* Fog */
    sg_image fog = sokol_fx_run(&fx->fog, 2, (sg_image[]){ 
        hdr, r->depth_pass.color_target },
            &state, 0);

    /* HDR */
    sokol_fx_run(&fx->hdr, 1, (sg_image[]){ fog },
        &state, &r->screen_pass);
}

static
void SokolCommit(ecs_iter_t *it) {
    sg_commit();
}

/* Initialize renderer & resources */
static
void SokolInitRenderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    EcsCanvas *canvas = ecs_term(it, EcsCanvas, 1);

    if (it->count > 1) {
        ecs_err("sokol: multiple canvas instances unsupported");
    }

    ecs_trace("#[bold]sokol: initializing renderer");
    ecs_log_push();

    int w = sapp_width();
    int h = sapp_height();

    sg_setup(&(sg_desc) {
        .context.depth_format = SG_PIXELFORMAT_NONE
    });

    assert(sg_isvalid());
    ecs_trace("sokol: library initialized");

    sokol_resources_t resources = init_resources();

    sokol_offscreen_pass_t depth_pass;
    sokol_offscreen_pass_t scene_pass = sokol_init_scene_pass(
        canvas->background_color, w, h, 2, &depth_pass);

    ecs_set(world, SokolRendererInst, SokolRenderer, {
        .canvas = it->entities[0],
        .resources = resources,
        .depth_pass = depth_pass,
        .shadow_pass = sokol_init_shadow_pass(SOKOL_SHADOW_MAP_SIZE),
        .scene_pass = scene_pass,
        .screen_pass = sokol_init_screen_pass(),
        .fx = sokol_init_fx(w, h)
    });

    ecs_trace("sokol: canvas initialized");

    ecs_set(world, SokolRendererInst, SokolMaterials, { true });

    ecs_set_pair(world, SokolRendererInst, EcsQuery, ecs_id(SokolGeometry), {
        ecs_query_new(world, "[in] flecs.systems.sokol.Geometry")
    });

    sokol_init_geometry(world, &resources);
    ecs_trace("sokol: static geometry resources initialized");

    ecs_log_pop();
}

/* Cleanup renderer */
static
void SokolFiniRenderer(ecs_iter_t *it) {
    ecs_trace("sokol: shutting down");
    sg_shutdown();
}

void FlecsSystemsSokolRendererImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokolRenderer);
    ECS_IMPORT(world, FlecsSystemsSokolMaterials);
    ECS_IMPORT(world, FlecsSystemsSokolGeometry);

    /* Create components in parent scope */
    ecs_entity_t parent = ecs_lookup_fullpath(world, "flecs.systems.sokol");
    ecs_entity_t module = ecs_set_scope(world, parent);
    ecs_set_name_prefix(world, "Sokol");

    ECS_COMPONENT_DEFINE(world, SokolRenderer);

    /* Register systems in module scope */
    ecs_set_scope(world, module);

    /* System that initializes renderer */
    ECS_SYSTEM(world, SokolInitRenderer, EcsOnLoad,
        flecs.components.gui.Canvas, 
        [out] !$flecs.systems.sokol.Renderer);

    /* Configure no_staging for SokolInitRenderer as it needs direct access to
     * the world for creating queries */
    ecs_system_init(world, &(ecs_system_desc_t) {
        .entity.entity = SokolInitRenderer,
        .no_staging = true
    });

    /* System that cleans up renderer */
    ECS_OBSERVER(world, SokolFiniRenderer, EcsUnSet, 
        flecs.systems.sokol.Renderer);

    /* System that orchestrates the render tasks */
    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        flecs.systems.sokol.Renderer,
        (flecs.core.Query, Geometry));

    /* System that calls sg_commit */
    ECS_SYSTEM(world, SokolCommit, EcsOnStore, 0);
}
