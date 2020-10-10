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

typedef struct sokol_shadow_pass_t {
    sg_pass_action pass_action;
    sg_pass pass;
    sg_pipeline pip;
    sg_image depth_tex;
    sg_image color_tex;
} sokol_shadow_pass_t;

typedef struct sokol_pass_input_t {
    const char *name;
    int id;
} sokol_pass_input_t;

typedef struct sokol_pass_desc_t {
    const char *shader;
    int32_t width;
    int32_t height;
    sokol_pass_input_t inputs[SOKOL_MAX_EFFECT_INPUTS];
} sokol_pass_desc_t;

typedef struct sokol_pass_t {
    sg_pass pass;
    sg_pass_action pass_action;
    sg_pipeline pip;
    sg_image output;
    sg_image depth_output;
    int32_t input_count;
    int32_t inputs[SOKOL_MAX_EFFECT_INPUTS];
    int32_t width;
    int32_t height;
} sokol_pass_t;

typedef struct SokolEffect {
    sokol_pass_t pass[SOKOL_MAX_EFFECT_PASS];
    int32_t pass_count;
} SokolEffect;

typedef struct SokolMaterial {
    uint16_t material_id;
} SokolMaterial;

typedef struct SokolRenderer {
    SDL_Window* sdl_window;
    SDL_GLContext gl_context;
	sg_pass_action pass_action;
    sg_pipeline pip;

    sg_image offscreen_tex;
    sg_image offscreen_depth_tex;
    sg_pass offscreen_pass;
    sg_pass shadow_map_pass;
    sg_pass_action tex_pass_action;
    sg_buffer offscreen_quad;

    sokol_shadow_pass_t shadow_pass;    

    SokolEffect fx_bloom;

    sg_pipeline tex_pip;
} SokolRenderer;

#include "geometry.h"
#include "effect.h"

SokolEffect sokol_init_bloom(
    int width, 
    int height);

sokol_shadow_pass_t sokol_init_shadow_pass(
    int size);    

void sokol_run_shadow_pass(
    ecs_query_t *buffers,
    sokol_shadow_pass_t *pass,
    const EcsDirectionalLight *light_data,
    mat4 light_vp);

sg_image sokol_init_render_target_8(
    int32_t width, 
    int32_t height);

sg_image sokol_init_render_target_16(
    int32_t width, 
    int32_t height);

sg_image sokol_init_render_depth_target(
    int32_t width, 
    int32_t height);
