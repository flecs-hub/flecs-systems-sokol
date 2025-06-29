#ifndef FLECS_SYSTEMS_SOKOL_TYPES_H
#define FLECS_SYSTEMS_SOKOL_TYPES_H

#include <flecs_systems_sokol.h>
#include "sokol/sokol.h"

#define SOKOL_MAX_FX_STEPS (8)
#define SOKOL_MAX_FX_INPUTS (8)
#define SOKOL_MAX_FX_OUTPUTS (8)
#define SOKOL_MAX_FX_PASS (8)
#define SOKOL_MAX_FX_PARAMS (32)
#define SOKOL_SHADOW_MAP_SIZE (1024 * 8)
#define SOKOL_DEFAULT_DEPTH_NEAR (2.0)
#define SOKOL_DEFAULT_DEPTH_FAR (2500.0)
#define SOKOL_MAX_LIGHTS (32)

typedef struct SokolQuery {
    ecs_query_t *query;
} SokolQuery;

extern ECS_COMPONENT_DECLARE(SokolQuery);

/* Immutable resources used by different components to avoid duplication */
typedef struct sokol_resources_t {
    sg_buffer quad;

    sg_buffer rect;
    sg_buffer rect_indices;
    sg_buffer rect_normals;

    sg_buffer box;
    sg_buffer box_indices;
    sg_buffer box_normals;

    sg_image noise_texture;
    sg_image bg_texture;
} sokol_resources_t;

typedef struct sokol_global_uniforms_t {
    mat4 mat_v;
    mat4 mat_p;
    mat4 mat_vp;
    mat4 inv_mat_p;
    mat4 inv_mat_v;
    
    mat4 light_mat_v;
    mat4 light_mat_vp;

    vec3 light_ambient;
    
    vec4 light_ambient_ground;
    float light_ambient_ground_falloff;
    float light_ambient_ground_offset;
    float light_ambient_ground_intensity;

    vec3 sun_direction;
    vec3 sun_color;
    vec3 night_color;
    vec3 sun_screen_pos;
    float sun_intensity;

    vec3 eye_pos;
    vec3 eye_up;
    vec3 eye_lookat;
    vec3 eye_dir;
    vec3 eye_horizon;

    float t;
    float dt;
    float aspect;
    float near_;
    float far_;
    float fov;
    float shadow_near;
    float shadow_far;
    bool ortho;

    float shadow_map_size;
} sokol_global_uniforms_t;

typedef struct sokol_light_t {
    vec3 color;
    vec3 position;
    float distance;
} sokol_light_t;

/* Data that is collected once per frame and that is shared between passes */
typedef struct sokol_render_state_t {
    ecs_world_t *world;
    ecs_query_t *q_scene;
    
    const EcsDirectionalLight *light;
    const EcsCamera *camera;
    const EcsAtmosphere *atmosphere;

    ecs_rgb_t ambient_light;
    ecs_rgb_t ambient_light_ground;
    float ambient_light_ground_falloff;
    float ambient_light_ground_offset;
    float ambient_light_ground_intensity;
    int32_t width;
    int32_t height;

    sokol_resources_t *resources;
    sokol_global_uniforms_t uniforms;
    sg_image atmos;
    sg_image shadow_map;

    ecs_vec_t lights;
} sokol_render_state_t;

typedef struct sokol_offscreen_pass_t {
    sg_pass_action pass_action;
    sg_pass pass;
    sg_pipeline pip;
    sg_pipeline pip_2;
    sg_image depth_target;
    sg_image color_target;
    int32_t sample_count;
} sokol_offscreen_pass_t;

#include "resources.h"
#include "effect.h"
#include "fx/fx.h"

struct sokol_screen_pass_t {
    sg_pass_action pass_action;
    sg_pipeline pip;
};

/* Shadow pass */
sokol_offscreen_pass_t sokol_init_shadow_pass(
    int size);    

void sokol_run_shadow_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state);

/* Depth pass */
sokol_offscreen_pass_t sokol_init_depth_pass(
    int32_t w, 
    int32_t h,
    sg_image depth_target,
    int32_t sample_count);

void sokol_update_depth_pass(
    sokol_offscreen_pass_t *pass,
    int32_t w,
    int32_t h,
    sg_image depth_target,
    int32_t sample_count);

void sokol_run_depth_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state);

/* Scene pass */
sokol_offscreen_pass_t sokol_init_scene_pass(
    ecs_rgb_t background_color,
    int32_t w, 
    int32_t h,
    int32_t sample_count,
    sokol_offscreen_pass_t *depth_pass_out);

void sokol_update_scene_pass(
    sokol_offscreen_pass_t *pass,
    int32_t w,
    int32_t h,
    sokol_offscreen_pass_t *depth_pass);

/* Screen pass */
sokol_screen_pass_t sokol_init_screen_pass(void);

void sokol_run_screen_pass(
    sokol_screen_pass_t *pass,
    sokol_resources_t *resources,
    sokol_render_state_t *state,
    sg_image target);

/* Atmosphere pass */
sokol_offscreen_pass_t sokol_init_atmos_pass(void);

void sokol_run_atmos_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state);

/* Atmosphere to screen pass */
sokol_offscreen_pass_t sokol_init_atmos_to_scene_pass(void);

void sokol_run_atmos_to_screen_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state);

#endif
