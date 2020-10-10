#ifndef SOKOL_EFFECT_H
#define SOKOL_EFFECT_H

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

sg_image sokol_effect_run(
    sokol_resources_t *res,
    SokolEffect *effect,
    sg_image input);

int sokol_effect_add_pass(
    SokolEffect *fx, 
    sokol_pass_desc_t pass);

#endif
