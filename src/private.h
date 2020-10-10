#include <flecs_systems_sokol.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#define SOKOL_GLCORE33
#include <flecs-systems-sokol/sokol_gfx.h>

#define SOKOL_MAX_EFFECT_INPUTS (8)
#define SOKOL_MAX_EFFECT_PASS (8)

#define SOKOL_MAX_MATERIALS (255)
#define SOKOL_SHADOW_MAP_SIZE (4096)

typedef struct sokol_vs_material_t {
    float specular_power;
    float shininess;
    float emissive;
} sokol_vs_material_t;

typedef struct sokol_vs_materials_t {
    sokol_vs_material_t array[SOKOL_MAX_MATERIALS];
} sokol_vs_materials_t;

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

/* Data that is collected once per frame and that is shared between passes */
typedef struct sokol_render_state_t {
    /* Computed by renderer */
    ecs_world_t *world;
    ecs_query_t *q_scene;
    const EcsDirectionalLight *light;
    ecs_rgb_t ambient_light;
    const EcsCamera *camera;
    int32_t width;
    int32_t height;
    float aspect;
    mat4 light_mat_vp;
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
#include "geometry.h"
#include "effect.h"

typedef struct sokol_screen_pass_t {
    sg_pass_action pass_action;
    sg_pipeline pip;
} sokol_screen_pass_t;

typedef struct SokolMaterial {
    uint16_t material_id;
} SokolMaterial;

typedef struct SokolRenderer {
    sokol_resources_t resources;

    SDL_Window* sdl_window;
    SDL_GLContext gl_context;

    sokol_offscreen_pass_t shadow_pass;    
    sokol_offscreen_pass_t scene_pass;
    sokol_screen_pass_t screen_pass;

    SokolEffect fx_bloom;
} SokolRenderer;

SokolEffect sokol_init_bloom(
    int width, 
    int height);

/* Shadow pass */
sokol_offscreen_pass_t sokol_init_shadow_pass(
    int size);    

void sokol_run_shadow_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state);

/* Scene pass */
sokol_offscreen_pass_t sokol_init_scene_pass(
    ecs_rgb_t background_color,
    int32_t w, 
    int32_t h);

void sokol_run_scene_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state,
    sokol_vs_materials_t *mat_u);

/* Screen pass */
sokol_screen_pass_t sokol_init_screen_pass(void);

void sokol_run_screen_pass(
    sokol_screen_pass_t *pass,
    sokol_resources_t *resources,
    sokol_render_state_t *state,
    sg_image target);
