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

#include "resources.h"
#include "geometry.h"
#include "effect.h"

typedef struct sokol_offscreen_pass_t {
    sg_pass_action pass_action;
    sg_pass pass;
    sg_pipeline pip;
    sg_image depth_target;
    sg_image color_target;
} sokol_offscreen_pass_t;

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

sokol_offscreen_pass_t sokol_init_shadow_pass(
    int size);    

void sokol_run_shadow_pass(
    ecs_query_t *buffers,
    sokol_offscreen_pass_t *pass,
    const EcsDirectionalLight *light_data,
    mat4 light_vp);
