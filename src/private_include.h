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

typedef struct SokolCanvas {
    SDL_Window* sdl_window;
    SDL_GLContext gl_context;
	sg_pass_action pass_action;
    sg_pipeline pip;

    sg_image offscreen_tex;
    sg_image offscreen_depth_tex;
    sg_pass offscreen_pass;
    sg_pass_action tex_pass_action;
    sg_buffer offscreen_quad;

    SokolEffect fx_bloom;

    sg_pipeline tex_pip;
} SokolCanvas;

#include "buffer.h"
#include "effect.h"

SokolEffect sokol_init_bloom(
    int width, int height);
