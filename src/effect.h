#ifndef SOKOL_EFFECT_H
#define SOKOL_EFFECT_H

typedef struct sokol_fx_pass_input_t {
    const char *name;
    int id;
} sokol_fx_pass_input_t;

typedef struct sokol_fx_pass_param_t {
    const char *name;
    float value;
} sokol_fx_pass_param_t;

typedef struct sokol_fx_pass_desc_t {
    const char *shader;
    int32_t width;
    int32_t height;
    sokol_fx_pass_input_t inputs[SOKOL_MAX_EFFECT_INPUTS];
    sokol_fx_pass_param_t params[SOKOL_MAX_EFFECT_PARAMS];
} sokol_fx_pass_desc_t;

typedef struct sokol_fx_pass_t {
    sokol_offscreen_pass_t pass;
    int32_t input_count;
    int32_t param_count;
    int32_t inputs[SOKOL_MAX_EFFECT_INPUTS];
    float params[SOKOL_MAX_EFFECT_PARAMS];
    int32_t width;
    int32_t height;
} sokol_fx_pass_t;

typedef struct SokolEffect {
    sokol_fx_pass_t pass[SOKOL_MAX_EFFECT_PASS];
    int32_t pass_count;
} SokolEffect;

void sokol_effect_set_param(
    SokolEffect *effect,
    const char *param,
    float value);

sg_image sokol_effect_run(
    sokol_resources_t *res,
    SokolEffect *effect,
    sg_image input);

int sokol_effect_add_pass(
    SokolEffect *fx, 
    sokol_fx_pass_desc_t pass);

#endif
