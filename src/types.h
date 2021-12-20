#ifndef FLECS_SYSTEMS_SOKOL_TYPES_H
#define FLECS_SYSTEMS_SOKOL_TYPES_H

#include <flecs_systems_sokol.h>
#include "sokol/sokol.h"

#define SOKOL_MAX_FX_STEPS (8)
#define SOKOL_MAX_FX_INPUTS (8)
#define SOKOL_MAX_FX_OUTPUTS (8)
#define SOKOL_MAX_FX_PASS (8)
#define SOKOL_MAX_FX_PARAMS (32)
#define SOKOL_SHADOW_MAP_SIZE (1024)

/* Immutable resources used by different components to avoid duplication */
typedef struct sokol_resources_t {
    sg_buffer quad;

    sg_buffer rect;
    sg_buffer rect_indices;
    sg_buffer rect_normals;

    sg_buffer box;
    sg_buffer box_indices;
    sg_buffer box_normals;
} sokol_resources_t;

typedef struct sokol_global_uniforms_t {
    mat4 mat_vp;
    mat4 light_mat_v;
    mat4 light_mat_vp;

    vec3 light_ambient;
    vec3 light_direction;
    vec3 light_color;
    vec3 eye_pos;
    float shadow_map_size;
} sokol_global_uniforms_t;

/* Data that is collected once per frame and that is shared between passes */
typedef struct sokol_render_state_t {
    ecs_world_t *world;
    ecs_query_t *q_scene;
    
    const EcsDirectionalLight *light;
    const EcsCamera *camera;

    ecs_rgb_t ambient_light;
    int32_t width;
    int32_t height;
    float aspect;
    float delta_time;
    float world_time;

    sokol_resources_t *resources;
    sokol_global_uniforms_t uniforms;
    sg_image shadow_map;
} sokol_render_state_t;

typedef struct sokol_offscreen_pass_t {
    sg_pass_action pass_action;
    sg_pass pass;
    sg_pipeline pip;
    sg_image depth_target;
    sg_image color_target;
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

/* Screen pass */
sokol_screen_pass_t sokol_init_screen_pass(void);

void sokol_run_screen_pass(
    sokol_screen_pass_t *pass,
    sokol_resources_t *resources,
    sokol_render_state_t *state,
    sg_image target);

#endif
