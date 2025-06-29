#include "../../private_api.h"
#include "math.h"

ECS_COMPONENT_DECLARE(SokolRenderer);

/* Static geometry/texture resources */
static
sokol_resources_t sokol_init_resources(void) {
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

static
void vec4_print(vec4 v) {
    printf("%.2f, %.2f %.2f %.2f\n",
        v[0], v[1], v[2], v[3]);
}

typedef struct {
    // far plane
    vec3 c_far;  // center
    vec3 tl_far; // top left
    vec3 tr_far; // top right
    vec3 bl_far; // bottom left
    vec3 br_far; // bottom right

    // near plane
    vec3 c_near;  // center
    vec3 tl_near; // top left
    vec3 tr_near; // top right
    vec3 bl_near; // bottom left
    vec3 br_near; // bottom right

    // frustum center
    vec3 c_frustum;
} sokol_frustum_t;

static
void vec3_copy4(vec3 src, float v4, vec4 dst) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = v4;
}

/* Light matrix (used for shadow map calculation) */
static
void sokol_init_light_mat_vp(
    sokol_render_state_t *state)
{
    vec3 orig = {0, 0, 0};
    sokol_global_uniforms_t *u = &state->uniforms;

    vec3 d;
    glm_vec3_copy(u->eye_dir, d);

    // Cross product of direction & up
    vec3 right; glm_vec3_cross(d, u->eye_up, right);

    float ar = state->uniforms.aspect;
    float fov = state->uniforms.fov;
    float near_ = state->uniforms.shadow_near;
    float far_ = state->uniforms.shadow_far;

    float Hnear = 2 * tan(fov / 2) * near_;
    float Wnear = Hnear * ar;
    float Hfar = 2 * tan(fov / 2) * far_;
    float Wfar = Hfar * ar;
    vec3 camera_pos; glm_vec3_copy(state->uniforms.eye_pos, camera_pos);
    camera_pos[0] *= -1;
    
    sokol_frustum_t f_cam;

    // Points on far plane
    glm_vec3_scale(d, far_, f_cam.c_far);
    glm_vec3_add(camera_pos, f_cam.c_far, f_cam.c_far);

    vec3 u_hfar; glm_vec3_scale(u->eye_up, Hfar / 2, u_hfar);
    vec3 r_wfar; glm_vec3_scale(right, Wfar / 2, r_wfar);

    glm_vec3_add(f_cam.c_far, u_hfar, f_cam.tl_far);
    glm_vec3_sub(f_cam.tl_far, r_wfar, f_cam.tl_far);

    glm_vec3_add(f_cam.c_far, u_hfar, f_cam.tr_far);
    glm_vec3_add(f_cam.tr_far, r_wfar, f_cam.tr_far);

    glm_vec3_sub(f_cam.c_far, u_hfar, f_cam.bl_far);
    glm_vec3_sub(f_cam.bl_far, r_wfar, f_cam.bl_far);

    glm_vec3_sub(f_cam.c_far, u_hfar, f_cam.br_far);
    glm_vec3_add(f_cam.br_far, r_wfar, f_cam.br_far);

    // // Points on near plane
    glm_vec3_scale(d, near_, f_cam.c_near);
    glm_vec3_add(camera_pos, f_cam.c_near, f_cam.c_near);

    vec3 u_hnear; glm_vec3_scale(u->eye_up, Hnear / 2, u_hnear);
    vec3 r_wnear; glm_vec3_scale(right, Wnear / 2, r_wnear);

    glm_vec3_add(f_cam.c_near, u_hnear, f_cam.tl_near);
    glm_vec3_sub(f_cam.tl_near, r_wnear, f_cam.tl_near);

    glm_vec3_add(f_cam.c_near, u_hnear, f_cam.tr_near);
    glm_vec3_add(f_cam.tr_near, r_wnear, f_cam.tr_near);

    glm_vec3_sub(f_cam.c_near, u_hnear, f_cam.bl_near);
    glm_vec3_sub(f_cam.bl_near, r_wnear, f_cam.bl_near);

    glm_vec3_sub(f_cam.c_near, u_hnear, f_cam.br_near);
    glm_vec3_add(f_cam.br_near, r_wnear, f_cam.br_near);

    // // Light "position"
    vec3 light_pos_norm;
    glm_vec3_scale(u->sun_direction, -1, light_pos_norm);
    glm_vec3_normalize(light_pos_norm);

    // Light view matrix
    mat4 mat_light_view;
    glm_lookat(light_pos_norm, orig, u->eye_up, mat_light_view);

    // // Frustum to light view
    vec4 f_light[8];
    vec3_copy4(f_cam.br_near, 1.0, f_light[0]);
    vec3_copy4(f_cam.tr_near, 1.0, f_light[1]);
    vec3_copy4(f_cam.bl_near, 1.0, f_light[2]);
    vec3_copy4(f_cam.tl_near, 1.0, f_light[3]);

    vec3_copy4(f_cam.br_far, 1.0, f_light[4]);
    vec3_copy4(f_cam.tr_far, 1.0, f_light[5]);
    vec3_copy4(f_cam.bl_far, 1.0, f_light[6]);
    vec3_copy4(f_cam.tl_far, 1.0, f_light[7]);

    for (int i = 0; i < 8; i ++) {
        glm_mat4_mulv(mat_light_view, f_light[i], f_light[i]);
    }

    // Find min/max points for defining the orthographic light matrix
    vec3 min = { INFINITY, INFINITY, INFINITY };
    vec3 max = { -INFINITY, -INFINITY, -INFINITY };
    for (int i = 0; i < 8; i ++) {
        for (int c = 0; c < 3; c ++) {
            min[c] = glm_min(min[c], f_light[i][c]);
        }
        for (int c = 0; c < 3; c ++) {
            max[c] = glm_max(max[c], f_light[i][c]);
        }
    }

    // Get parameters for orthographic matrix
    float l = min[0];
    float r = max[0];
    float b = min[1];
    float t = max[1];
    float n = -max[2];
    float f = -min[2];

    // Discretize coordinates
    const float step_size = 16;
    l = floor(l / step_size) * step_size;
    r = ceil(r / step_size) * step_size;
    b = floor(b / step_size) * step_size;
    t = ceil(t / step_size) * step_size;
    n = floor(n / step_size) * step_size;
    f = ceil(f / step_size) * step_size;

    mat4 mat_light_ortho, mat_light_view_proj, mat_light_proj = {
         { 0.5f, 0.0f, 0.0f, 0 },
         { 0.0f, 0.5f, 0.0f, 0 },
         { 0.0f, 0.0f, 0.5f, 0 },
         { 0.5f, 0.5f, 0.5f, 1 }
    };

    glm_ortho(l, r, b, t, n, f, mat_light_ortho);
    glm_mat4_mul(mat_light_proj, mat_light_ortho, mat_light_proj);
    glm_mat4_mul(mat_light_proj, mat_light_view, mat_light_view_proj);

    glm_mat4_copy(mat_light_view, u->light_mat_v);
    glm_mat4_copy(mat_light_view_proj, u->light_mat_vp);
}

static
void sokol_world_to_screen(
    vec3 pos,
    vec3 out,
    sokol_render_state_t *state)
{
    vec4 pos4;
    glm_vec3_copy(pos, pos4);
    pos4[3] = 0.0;

    glm_mat4_mulv(state->uniforms.mat_vp, pos4, pos4);
    pos4[0] /= pos4[3];
    pos4[1] /= pos4[3];

    pos4[0] = 0.5 * (pos4[0] + 1.0);
    pos4[1] = 0.5 * (pos4[1] + 1.0);
    glm_vec3_copy(pos4, out);
}

/* Compute uniform values for camera & light that are used across shaders */
static
void sokol_init_global_uniforms(
    sokol_render_state_t *state)
{
    sokol_global_uniforms_t *u = &state->uniforms;

    /* Default camera parameters */
    u->near_ = SOKOL_DEFAULT_DEPTH_NEAR;
    u->far_ = SOKOL_DEFAULT_DEPTH_FAR;
    u->fov = 30;
    u->ortho = false;

    /* If camera is set, get values */
    if (state->camera) {
        EcsCamera cam = *state->camera;

        if (cam.fov) {
            u->fov = cam.fov;
        }

        if (cam.near_ || cam.far_) {
            u->near_ = cam.near_;
            u->far_ = cam.far_;
        }

        if (cam.up[0] || !cam.up[1] || !cam.up[2]) {
            glm_vec3_copy(cam.up, u->eye_up);
        }

        state->uniforms.ortho = cam.ortho;

        glm_vec3_copy(cam.position, u->eye_pos);
        u->eye_pos[0] = -u->eye_pos[0];
        glm_vec3_copy(cam.lookat, u->eye_lookat);
        u->eye_lookat[0] = -u->eye_lookat[0];
    }

    /* Orthographic/perspective projection matrix */
    if (u->ortho) {
        glm_ortho_default(u->aspect, u->mat_p);
    } else {
        glm_perspective(u->fov, u->aspect, u->near_, u->far_, u->mat_p);
    }

    glm_mat4_inv(u->mat_p, u->inv_mat_p);

    /* View + view * projection matrix */
    glm_lookat(u->eye_pos, u->eye_lookat, u->eye_up, u->mat_v);

    /* Flip x axis so -1 is on left and 1 is on right */
    vec3 flip_x = {-1, 1, 1};
    glm_scale(u->mat_v, flip_x);

    glm_mat4_mul(u->mat_p, u->mat_v, u->mat_vp);
    glm_mat4_inv(u->mat_v, u->inv_mat_v);

    /* Light parameters */
    if (state->light) {
        EcsDirectionalLight l = *state->light;
        glm_vec3_copy(l.direction, u->sun_direction);
        glm_vec3_copy(l.color, u->sun_color);
        u->sun_intensity = l.intensity;
    } else {
        glm_vec3_zero(u->sun_direction);
        glm_vec3_zero(u->sun_color);
        u->sun_intensity = 1.0;
    }
    
    glm_vec3_copy((float*)&state->ambient_light, u->light_ambient);
    glm_vec3_copy((float*)&state->ambient_light_ground, u->light_ambient_ground);
    u->light_ambient_ground_falloff = state->ambient_light_ground_falloff;
    u->light_ambient_ground_offset = state->ambient_light_ground_offset;
    u->light_ambient_ground_intensity = state->ambient_light_ground_intensity;

    // Direction vector
    vec3 d; glm_vec3_sub(u->eye_lookat, u->eye_pos, d);
    d[0] *= -1;
    glm_vec3_normalize(d);
    glm_vec3_copy(d, state->uniforms.eye_dir);

    /* Shadow parameters */
    u->shadow_map_size = SOKOL_SHADOW_MAP_SIZE;
    u->shadow_near = -(8 + u->eye_pos[1]);

    /* Calculate light position in screen space */
    vec3 sun_pos;
    sun_pos[0] = -u->sun_direction[0];
    sun_pos[1] = -u->sun_direction[1];
    sun_pos[2] = -u->sun_direction[2];
    sokol_world_to_screen(sun_pos, u->sun_screen_pos, state);

    /* Calculate the horizon position in screen space */
    vec3 lookat;
    glm_vec3_copy(u->eye_lookat, lookat);
    lookat[1] = 0;
    sokol_world_to_screen(lookat, u->eye_horizon, state);
}

static
int sokol_qsort_light_compare(const void *ptr1, const void *ptr2) {
    const sokol_light_t *l1 = ptr1;
    const sokol_light_t *l2 = ptr2;

    float d1 = pow(l1->position[0], 2.0) + 
               pow(l1->position[2], 2.0);
    float d2 = pow(l2->position[0], 2.0) + 
               pow(l2->position[2], 2.0);

    return (d1 > d2) - (d1 < d2);
}

/* Collect lights */
static
void sokol_gather_lights(ecs_world_t *world, SokolRenderer *r, sokol_render_state_t *state) {
    ecs_vec_clear(&r->lights);

    vec3 eye_pos;
    glm_vec3_copy(state->uniforms.eye_pos, eye_pos);

    eye_pos[0] *= -1;

    ecs_iter_t it = ecs_query_iter(world, r->lights_query);
    while (ecs_query_next(&it)) {
        EcsPointLight *l = ecs_field(&it, EcsPointLight, 0);
        EcsTransform3 *m = ecs_field(&it, EcsTransform3, 1);

        for (int i = 0; i < it.count; i ++) {
            sokol_light_t *light = ecs_vec_append_t(
                NULL, &r->lights, sokol_light_t);
            glm_vec3_copy(l[i].color, light->color);

            light->color[0] *= l[i].intensity;
            light->color[1] *= l[i].intensity;
            light->color[2] *= l[i].intensity;

            vec3 pos = {
                m[i].value[3][0],
                m[i].value[3][1],
                m[i].value[3][2]
            };

            light->position[0] = pos[0];
            light->position[1] = pos[1];
            light->position[2] = pos[2];

            light->distance = l[i].distance;

            glm_vec3_sub(light->position, eye_pos, light->position);
        }
    }

    sokol_light_t *lights = ecs_vec_first(&r->lights);
    qsort(lights, ecs_vec_count(&r->lights), sizeof(sokol_light_t),
        sokol_qsort_light_compare);

    int32_t lights_count = ecs_vec_count(&r->lights);
    if (lights_count > SOKOL_MAX_LIGHTS) {
        lights_count = SOKOL_MAX_LIGHTS;
    }

    ecs_vec_set_count_t(NULL, &r->lights, sokol_light_t, lights_count);

    state->lights = r->lights;
}

/* Render */
static
void SokolRender(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolRenderer *r = ecs_field(it, SokolRenderer, 0);
    SokolQuery *q_buffers = ecs_field(it, SokolQuery, 1);
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

    const EcsCanvas *canvas = ecs_get(world, r->canvas, EcsCanvas);
    state.uniforms.shadow_far = canvas->shadow_far;

    /* Resize resources if canvas changed */
    if (state.width != canvas->width || state.height != canvas->height) {
        EcsCanvas *canvas_w = ecs_ensure(world, r->canvas, EcsCanvas);
        canvas_w->width = state.width;
        canvas_w->height = state.height;
        ecs_modified(world, r->canvas, EcsCanvas);

        ecs_dbg_3("sokol: update canvas size to %d, %d", state.width, state.height);
        sokol_update_scene_pass(&r->scene_pass, state.width, state.height,
            &r->depth_pass);
        sokol_update_fx(r->fx, state.width, state.height);
    }

    /* Load active camera & light data from canvas */
    if (canvas->camera) {
        state.camera = ecs_get(world, canvas->camera, EcsCamera);
        r->camera = canvas->camera;

        float camera_y_offset = state.camera->position[1] - state.uniforms.shadow_far / 2;
        if (camera_y_offset < 0) {
            camera_y_offset = 0;
        }

        /* Increase shadow distance with camera height */
        state.uniforms.shadow_far = glm_max(
            state.uniforms.shadow_far, pow(camera_y_offset, 1.4));
        state.uniforms.shadow_far = glm_min(state.uniforms.shadow_far,
            state.camera->far_);
    }

    /* Get atmosphere settings */
    state.atmosphere = ecs_get(world, r->canvas, EcsAtmosphere);

    /* Get ambient light */
    state.ambient_light = canvas->ambient_light;
    state.ambient_light_ground = canvas->ambient_light_ground;
    state.ambient_light_ground_falloff = canvas->ambient_light_ground_falloff;
    state.ambient_light_ground_offset = canvas->ambient_light_ground_offset;
    state.ambient_light_ground_intensity = canvas->ambient_light_ground_intensity;

    if (canvas->directional_light) {
        state.light = ecs_get(world, canvas->directional_light, 
            EcsDirectionalLight);
    } else {
        /* Set default ambient light if nothing is configured */
        if (!state.ambient_light.r && !state.ambient_light.g && 
            !state.ambient_light.b) 
        {
            state.ambient_light = (EcsRgb){1.0, 1.0, 1.0};
        }
    }

    /* Compute uniforms that are shared between passes */
    sokol_init_global_uniforms(&state);

    /* Collect lights for scene */
    sokol_gather_lights(world, r, &state);

    /* Compute shadow parameters and run shadow pass */
    if (canvas->directional_light) {
        sokol_init_light_mat_vp(&state);
        sokol_run_shadow_pass(&r->shadow_pass, &state);
    }

    /* Depth prepass for more efficient drawing */
    sokol_run_depth_pass(&r->depth_pass, &state);

    /* Render atmosphere */
    if (state.atmosphere) {
        sokol_run_atmos_pass(&r->atmos_pass, &state);
        state.atmos = r->atmos_pass.color_target;
    } else {
        state.atmos = r->resources.bg_texture;
    }

    /* Render scene */
    sokol_run_scene_pass(&r->scene_pass, &state);
    sg_image hdr = r->scene_pass.color_target;

    /* Ssao */
    sg_image ssao = sokol_fx_run(&fx->ssao, 2, (sg_image[]){ 
        hdr, r->depth_pass.color_target }, 
            &state, 0);

    /* Fog */
    const EcsRgb *bg_color = &canvas->background_color;
    sokol_fog_set_params(&fx->fog, canvas->fog_density, 
        bg_color->r, bg_color->g, bg_color->b, state.uniforms.eye_horizon[1]);
    sg_image scene_with_fog = sokol_fx_run(&fx->fog, 3, (sg_image[]){ 
        ssao, r->depth_pass.color_target, state.atmos },
            &state, 0);

    /* HDR */
    sokol_fx_run(&fx->hdr, 1, (sg_image[]){ scene_with_fog },
        &state, &r->screen_pass);

    // sokol_run_screen_pass(&r->screen_pass, r, &state, hdr);
}

static
void SokolCommit(ecs_iter_t *it) {
    sg_commit();
}

/* Initialize renderer & resources */
static
void SokolInitRenderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    EcsCanvas *canvas = ecs_field(it, EcsCanvas, 0);

    if (it->count > 1) {
        ecs_err("sokol: multiple canvas instances unsupported");
    }

    ecs_trace("#[bold]sokol: initializing renderer");
    ecs_log_push();

    int w = sapp_width();
    int h = sapp_height();

    sg_setup(&(sg_desc) {
        .context.depth_format = SG_PIXELFORMAT_NONE,
        .buffer_pool_size = 16384,
        .logger = { slog_func }
    });

    assert(sg_isvalid());
    ecs_trace("sokol: library initialized");

    sokol_resources_t resources = sokol_init_resources();
    resources.bg_texture = sokol_bg_texture(canvas->background_color, 2, 2);

    sokol_offscreen_pass_t depth_pass;
    sokol_offscreen_pass_t scene_pass = sokol_init_scene_pass(
        canvas->background_color, w, h, 1, &depth_pass);
    sokol_offscreen_pass_t atmos_pass = sokol_init_atmos_pass();

    ecs_vec_t lights; 
    ecs_vec_init_t(NULL, &lights, sokol_light_t, 0);

    ecs_query_t *lights_query = ecs_query(world, {
        .terms = {
            { ecs_id(EcsPointLight) },
            { ecs_id(EcsTransform3) }
        },
        .cache_kind = EcsQueryCacheAuto
    });

    ecs_set(world, SokolRendererInst, SokolRenderer, {
        .canvas = it->entities[0],
        .resources = resources,
        .depth_pass = depth_pass,
        .shadow_pass = sokol_init_shadow_pass(SOKOL_SHADOW_MAP_SIZE),
        .scene_pass = scene_pass,
        .atmos_pass = atmos_pass,
        .screen_pass = sokol_init_screen_pass(),
        .fx = sokol_init_fx(w, h),
        .lights = lights,
        .lights_query = lights_query
    });

    ecs_trace("sokol: canvas initialized");

    ecs_set_pair(world, SokolRendererInst, SokolQuery, ecs_id(SokolGeometry), {
        ecs_query(world, { .expr = "[in] flecs.systems.sokol.Geometry" })
    });

    sokol_init_geometry(world, &resources);
    ecs_trace("sokol: static geometry resources initialized");

    /* Run once */
    ecs_enable(it->world, it->system, false);

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
    ECS_IMPORT(world, FlecsSystemsSokolGeometry);

    /* Create components in parent scope */
    ecs_entity_t parent = ecs_lookup(world, "flecs.systems.sokol");
    ecs_entity_t module = ecs_set_scope(world, parent);
    ecs_set_name_prefix(world, "Sokol");

    ECS_COMPONENT_DEFINE(world, SokolRenderer);

    /* Register systems in module scope */
    ecs_set_scope(world, module);

    /* System that initializes renderer */
    ECS_SYSTEM(world, SokolInitRenderer, EcsOnLoad,
        flecs.components.gui.Canvas, 
        [out] !flecs.systems.sokol.Renderer($));

    /* Configure immediate for SokolInitRenderer as it needs direct access to
     * the world for creating queries */
    ecs_system(world, {
        .entity = SokolInitRenderer,
        .immediate = true
    });

    /* System that cleans up renderer */
    ECS_OBSERVER(world, SokolFiniRenderer, EcsOnRemove, 
        flecs.systems.sokol.Renderer);

    /* System that orchestrates the render tasks */
    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        flecs.systems.sokol.Renderer,
        (sokol.Query, Geometry));

    /* System that calls sg_commit */
    ECS_SYSTEM(world, SokolCommit, EcsOnStore, 0);
}
