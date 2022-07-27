#include "../../private_api.h"

ECS_COMPONENT_DECLARE(SokolRenderer);

/* Static geometry/texture resources */
static
sokol_resources_t init_resources(void) {
    return (sokol_resources_t){
        .quad = sokol_buffer_quad(),

        .rect = sokol_buffer_rectangle(),
        .rect_indices = sokol_buffer_rectangle_indices(),
        .rect_normals = sokol_buffer_rectangle_normals(),

        .box = sokol_buffer_box(),
        .box_indices = sokol_buffer_box_indices(),
        .box_normals = sokol_buffer_box_normals(),

        .noise_texture = sokol_noise_texture(16, 16)
    };
}

/* Light matrix (used for shadow map calculation) */
static
void init_light_mat_vp(
    sokol_render_state_t *state)
{
    mat4 mat_p;
    mat4 mat_v;
    vec3 lookat = {0.0, 0.0, 0.0};
    vec3 up = {0, 1, 0};

    glm_ortho(-1000, 1000, -1000, 1000, 1.0, 50, mat_p);

    vec4 dir = {
        state->light->direction[0],
        state->light->direction[1],
        state->light->direction[2]
    };
    glm_vec4_scale(dir, 1.0, dir);
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

/* Compute uniform values for camera & light that are used across shaders */
static
void init_global_uniforms(
    sokol_render_state_t *state)
{
    /* Default camera parameters */
    state->uniforms.near_ = SOKOL_DEFAULT_DEPTH_NEAR;
    state->uniforms.far_ = SOKOL_DEFAULT_DEPTH_FAR;
    state->uniforms.fov = 30;
    state->uniforms.ortho = false;

    /* If camera is set, get values */
    if (state->camera) {
        EcsCamera cam = *state->camera;

        if (cam.fov) {
            state->uniforms.fov = cam.fov;
        }

        if (cam.near_ || cam.far_) {
            state->uniforms.near_ = cam.near_;
            state->uniforms.far_ = cam.far_;
        }

        if (cam.up[0] || !cam.up[1] || !cam.up[2]) {
            glm_vec3_copy(cam.up, state->uniforms.eye_up);
        }

        state->uniforms.ortho = cam.ortho;

        glm_vec3_copy(cam.position, state->uniforms.eye_pos);
        glm_vec3_copy(cam.lookat, state->uniforms.eye_lookat);
    }

    /* Orthographic/perspective projection matrix */
    if (state->uniforms.ortho) {
        glm_ortho_default(state->uniforms.aspect, state->uniforms.mat_p);
    } else {
        glm_perspective(
            state->uniforms.fov, 
            state->uniforms.aspect, 
            state->uniforms.near_, 
            state->uniforms.far_, 
            state->uniforms.mat_p);
    }
    glm_mat4_inv(state->uniforms.mat_p, state->uniforms.inv_mat_p);

    /* View + view * projection matrix */
    glm_lookat(state->uniforms.eye_pos, state->uniforms.eye_lookat, state->uniforms.eye_up, state->uniforms.mat_v);
    glm_mat4_mul(state->uniforms.mat_p, state->uniforms.mat_v, state->uniforms.mat_vp);

    /* Light parameters */
    if (state->light) {
        EcsDirectionalLight l = *state->light;
        glm_vec3_copy(l.direction, state->uniforms.light_direction);
        glm_vec3_copy(l.color, state->uniforms.light_color);
    } else {
        glm_vec3_zero(state->uniforms.light_direction);
        glm_vec3_zero(state->uniforms.light_color);
    }
    glm_vec3_copy((float*)&state->ambient_light, state->uniforms.light_ambient);

    /* Shadow map size */
    state->uniforms.shadow_map_size = SOKOL_SHADOW_MAP_SIZE;
}

/* Render */
static
void SokolRender(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolRenderer *r = ecs_field(it, SokolRenderer, 1);
    SokolQuery *q_buffers = ecs_field(it, SokolQuery, 2);
    sokol_render_state_t state = {0};
    sokol_fx_resources_t *fx = r->fx;

    if (it->count > 1) {
        ecs_err("sokol: multiple canvas instances unsupported");
    }

    const ecs_world_info_t *stats = ecs_get_world_info(world);

    /* Initialize renderer state */
    state.uniforms.dt = it->delta_time;
    state.uniforms.t = stats->world_time_total;
    state.width = sapp_width();
    state.height = sapp_height();
    state.uniforms.aspect = (float)state.width / (float)state.height;
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

    /* Ssao */
    sg_image ssao = sokol_fx_run(&fx->ssao, 2, (sg_image[]){ 
        hdr, r->depth_pass.color_target }, 
            &state, 0);

    /* Fog */
    sg_image scene_with_fog = sokol_fx_run(&fx->fog, 2, (sg_image[]){ 
        ssao, r->depth_pass.color_target },
            &state, 0);

    /* HDR */
    sokol_fx_run(&fx->hdr, 1, (sg_image[]){ scene_with_fog },
        &state, &r->screen_pass);

    // sokol_run_screen_pass(&r->screen_pass, &r->resources, &state, ssao);
}

static
void SokolCommit(ecs_iter_t *it) {
    sg_commit();
}

/* Initialize renderer & resources */
static
void SokolInitRenderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    EcsCanvas *canvas = ecs_field(it, EcsCanvas, 1);

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
        canvas->background_color, w, h, 1, &depth_pass);

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

    ecs_set_pair(world, SokolRendererInst, SokolQuery, ecs_id(SokolGeometry), {
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
        [out] !flecs.systems.sokol.Renderer($));

    /* Configure no_staging for SokolInitRenderer as it needs direct access to
     * the world for creating queries */
    ecs_system(world, {
        .entity = SokolInitRenderer,
        .no_staging = true
    });

    /* System that cleans up renderer */
    ECS_OBSERVER(world, SokolFiniRenderer, EcsUnSet, 
        flecs.systems.sokol.Renderer);

    /* System that orchestrates the render tasks */
    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        flecs.systems.sokol.Renderer,
        (sokol.Query, Geometry));

    /* System that calls sg_commit */
    ECS_SYSTEM(world, SokolCommit, EcsOnStore, 0);
}
