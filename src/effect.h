#ifndef SOKOL_EFFECT_H
#define SOKOL_EFFECT_H

typedef struct sokol_screen_pass_t sokol_screen_pass_t;

/* Fx descriptors */

typedef struct sokol_fx_input_t {
    int8_t pass;
    int8_t index;
} sokol_fx_input_t;

typedef struct sokol_fx_output_desc_t {
    int16_t width;
    int16_t height;
    bool global_size;
    float factor;
} sokol_fx_output_desc_t;

typedef struct sokol_fx_step_t {
    const char *name;
    int8_t output;
    int8_t loop_count;
    sokol_fx_input_t inputs[SOKOL_MAX_FX_INPUTS];
    float params[SOKOL_MAX_FX_PARAMS];
} sokol_fx_step_t; 

typedef struct sokol_fx_pass_desc_t {
    const char *name;
    const char *shader_header;
    const char *shader;
    sokol_fx_output_desc_t outputs[SOKOL_MAX_FX_OUTPUTS];
    int8_t sample_count;
    int8_t mipmap_count;
    sg_pixel_format color_format;
    const char *inputs[SOKOL_MAX_FX_INPUTS];
    const char *params[SOKOL_MAX_FX_INPUTS];
    sokol_fx_step_t steps[SOKOL_MAX_FX_STEPS];
} sokol_fx_pass_desc_t;


/* Fx implementation */

typedef struct sokol_fx_output_t {
    int16_t width;
    int16_t height;    
    sg_image out[2];
    sg_pass pass[2];
    int8_t step_count;
    int8_t toggle;
    bool global_size;
    float factor;
} sokol_fx_output_t;

typedef struct sokol_fx_pass_t {
    const char *name;
    int8_t loop_count;

    int8_t param_count;
    int8_t input_count;
    
    int8_t sample_count;
    int8_t mipmap_count;
    int8_t output_count;
    sg_pixel_format color_format;
    sokol_fx_output_t outputs[SOKOL_MAX_FX_OUTPUTS];

    int8_t step_count;
    sokol_fx_step_t steps[SOKOL_MAX_FX_STEPS];

    sg_pipeline pip;
} sokol_fx_pass_t;

typedef struct SokolFx {
    const char *name;
    sokol_fx_pass_t pass[SOKOL_MAX_FX_PASS];
    int8_t input_count;
    int8_t pass_count;
    int16_t width;
    int16_t height;
} SokolFx;

typedef struct sokol_fx_resources_t {
    SokolFx hdr;
    SokolFx fog;
    SokolFx ssao;
    SokolFx blend;
} sokol_fx_resources_t;

/* Map input index to effect input */
#define SOKOL_FX_INPUT(index) (index)

/* Map input index to local pass target */ 
#define SOKOL_FX_PASS(index) (index + SOKOL_MAX_FX_INPUTS)

#define SOKOL_FX_IS_PASS(index) (index >= SOKOL_MAX_FX_INPUTS)

void sokol_effect_set_param(
    SokolFx *effect,
    const char *param,
    float value);

sg_image sokol_fx_run(
    SokolFx *effect,
    int32_t input_count,
    sg_image inputs[],
    sokol_render_state_t *state,
    sokol_screen_pass_t *screen_pass);

int sokol_fx_add_pass(
    SokolFx *fx, 
    sokol_fx_pass_desc_t *pass);

void sokol_fx_update_size(
    SokolFx *fx, 
    int32_t width,
    int32_t height);

#endif
