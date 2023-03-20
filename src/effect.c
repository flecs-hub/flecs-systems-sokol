#include "private_api.h"

typedef struct fx_uniforms_t {
    float t;
    float dt;
    float aspect;
    float near_;
    float far_;
    float target_size[2];
} fx_uniforms_t;

typedef struct fx_mat_uniforms_t {
    mat4 mat_p;
    mat4 inv_mat_p;
} fx_mat_uniforms_t;

static
char* fx_build_shader(
    sokol_fx_pass_desc_t *pass)
{
    ecs_strbuf_t shad = ECS_STRBUF_INIT;

    /* Add fx header */
    ecs_strbuf_appendstr(&shad, 
        SOKOL_SHADER_HEADER
        "#include \"etc/sokol/shaders/constants.glsl\"\n"
        "#define FX\n"
        "out vec4 frag_color;\n"
        "in vec2 uv;\n"
        "uniform float u_t;\n"
        "uniform float u_dt;\n"
        "uniform float u_aspect;\n"
        "uniform float u_near;\n"
        "uniform float u_far;\n"
        "uniform vec2 u_target_size;\n"
        "uniform mat4 u_mat_p;\n"
        "uniform mat4 u_inv_mat_p;\n"
        "uniform sampler2D u_noise;\n");

    /* Add inputs */
    for (int32_t i = 0; i < SOKOL_MAX_FX_INPUTS; i ++) {
        const char *input = pass->inputs[i];
        if (!input) {
            break;
        }

        ecs_strbuf_append(&shad, "uniform sampler2D %s;\n", input);
    }

    /* Add uniform params */
    for (int32_t i = 0; i < SOKOL_MAX_FX_INPUTS; i ++) {
        const char *param = pass->params[i];
        if (!param) {
            break;
        }

        ecs_strbuf_append(&shad, "uniform float %s;\n", param);
    }

    /* Add shader header */
    if (pass->shader_header) {
        ecs_strbuf_appendstr(&shad, pass->shader_header);
    }

    /* Shader main */
    ecs_strbuf_appendstr(&shad, "void main() {\n");
    ecs_strbuf_append(&shad, pass->shader);
    ecs_strbuf_append(&shad, "}\n");

    char *shader = ecs_strbuf_get(&shad);
    char *shader_pp = sokol_shader_from_str(shader);
    ecs_os_free(shader);
    return shader_pp;
}

int sokol_fx_add_pass(
    SokolFx *fx, 
    sokol_fx_pass_desc_t *pass_desc)
{
    ecs_assert(fx->pass_count < SOKOL_MAX_FX_PASS, ECS_INVALID_PARAMETER, NULL);

    sokol_fx_pass_t *pass = &fx->pass[fx->pass_count ++];
    pass->name = pass_desc->name;
    pass->mipmap_count = pass_desc->mipmap_count;
    pass->sample_count = pass_desc->sample_count;
    pass->loop_count = 1;
    sg_pixel_format color_format = pass_desc->color_format;
    pass->color_format = color_format;

    if (!color_format) {
        color_format = SG_PIXELFORMAT_RGBA8;
    }

    const char *vs_prog = sokol_vs_passthrough();
    char *fs_prog = fx_build_shader(pass_desc);

    /* Populate list of shader specific uniforms */
    sg_shader_uniform_block_desc prog_ub = { 0 };

    for (int32_t i = 0; i < SOKOL_MAX_FX_PARAMS; i ++) {
        const char *param = pass_desc->params[i];
        if (!param) {
            break;
        }
        prog_ub.size += sizeof(float);
        prog_ub.uniforms[i] = (sg_shader_uniform_desc){
            .name = param, 
            .type = SG_UNIFORMTYPE_FLOAT 
        };
        pass->param_count ++;
    }

    /* Shader program */
    sg_shader_desc prog = {
        .vs = {
            .source = vs_prog,
        },
        .fs = {
            .source = fs_prog,
            .uniform_blocks = {
                [0] = {
                    .size = sizeof(fx_uniforms_t),
                    .uniforms = {
                        [0] = { .name="u_t", .type=SG_UNIFORMTYPE_FLOAT },
                        [1] = { .name="u_dt", .type=SG_UNIFORMTYPE_FLOAT },
                        [2] = { .name="u_aspect", .type=SG_UNIFORMTYPE_FLOAT },
                        [3] = { .name="u_near", .type=SG_UNIFORMTYPE_FLOAT },
                        [4] = { .name="u_far", .type=SG_UNIFORMTYPE_FLOAT },
                        [5] = { .name="u_target_size", .type=SG_UNIFORMTYPE_FLOAT2 }
                    }
                },
                [1] = {
                    .size = sizeof(fx_mat_uniforms_t),
                    .uniforms = {
                        [0] = { .name = "u_mat_p", .type = SG_UNIFORMTYPE_MAT4 },
                        [1] = { .name = "u_inv_mat_p", .type = SG_UNIFORMTYPE_MAT4 }
                    }
                },
                [2] = prog_ub
            }
        }
    };

    /* Add noise texture by default */
    prog.fs.images[0].name = "u_noise";
    prog.fs.images[0].image_type = SG_IMAGETYPE_2D;

    /* Add inputs to program */
    for (int32_t i = 1; i < SOKOL_MAX_FX_INPUTS; i ++) {
        const char *input = pass_desc->inputs[i - 1];
        if (!input) {
            break;
        }

        prog.fs.images[i].name = input;
        prog.fs.images[i].image_type = SG_IMAGETYPE_2D;
        pass->input_count ++;
    }

    /* Create pipeline */
    ecs_trace("sokol: build pipeline for fx pass '%s.%s'", 
        fx->name, pass->name);

    pass->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(&prog),
        .layout = {         
            .attrs = {
                [0] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT2 }
            }
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_NONE
        },
        .colors = {{
            .pixel_format = color_format
        }},
        .sample_count = pass->sample_count
    });

    /* Count steps */
    ecs_os_memcpy_n(pass->steps, pass_desc->steps, sokol_fx_step_t, SOKOL_MAX_FX_STEPS);

    for (int8_t i = 0; i < SOKOL_MAX_FX_STEPS; i ++) {
        sokol_fx_step_t *step = &pass->steps[i];
        if (i && !step->name) {
            /* first step is allowed to be nameless */
            break;
        }

        pass->step_count ++;
        if (step->loop_count > pass->loop_count) {
            pass->loop_count = step->loop_count;
        }

        if (!step->loop_count) {
            if (!i) {
                step->loop_count = 1;
            } else {
                step->loop_count = step[-1].loop_count;
            }
        }

        if (!step->output && i) {
            step->output = step[-1].output;
        }

        if (step->output >= SOKOL_MAX_FX_OUTPUTS || 
            ((pass_desc->outputs[step->output].width == 0) &&
                !pass_desc->outputs[step->output].global_size))
        {
            ecs_fatal("sokol: fx step '%s.%s.%s' has an invalid output",
                fx->name, pass->name, step->name);
            ecs_abort(ECS_INVALID_OPERATION, 0);
        }

        /* Look for effect-level inputs */
        for (int8_t j = 0; j < SOKOL_MAX_FX_INPUTS; j ++) {
            int32_t pass = pass_desc->steps[i].inputs[j].pass;
            if (!SOKOL_FX_IS_PASS(pass)) {
                /* effect-level input */
                if (pass >= fx->input_count) {
                    fx->input_count = pass + 1;
                }
            }
        }
    }

    /* Add outputs */
    for (int32_t i = 0; i < SOKOL_MAX_FX_OUTPUTS; i ++) {
        sokol_fx_output_desc_t *output = &pass_desc->outputs[i];
        if (output->global_size) {
            output->width = fx->width;
            output->height = fx->height;
            ecs_assert(output->width != 0, ECS_INVALID_PARAMETER, NULL);
            ecs_assert(output->height != 0, ECS_INVALID_PARAMETER, NULL);
        }
        if (!output->width) {
            break;
        }
        if (!output->height) {
            output->height = output->width;
        }

        pass->outputs[i].out[0] = sokol_target(fx->name, output->width, 
            output->height, pass->sample_count, pass->mipmap_count, 
            color_format);
        pass->outputs[i].pass[0] = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0].image = pass->outputs[i].out[0]
        });

        pass->outputs[i].width = output->width;
        pass->outputs[i].height = output->height;
        pass->outputs[i].global_size = output->global_size;
        pass->outputs[i].factor = output->factor;

        int32_t step_count = 0;
        for (int s = 0; s < pass->step_count; s ++) {
            sokol_fx_step_t *step = &pass->steps[s];
            if (step->output == i) {
                step_count ++;
            }
        }

        /* If multiple steps use output, create intermediate buffer */
        if (step_count > 1) {
            pass->outputs[i].out[1] = sokol_target(fx->name, output->width, 
                output->height, pass->sample_count, pass->mipmap_count, color_format);
            pass->outputs[i].pass[1] = sg_make_pass(&(sg_pass_desc){
                .color_attachments[0].image = pass->outputs[i].out[1]
            });
        }

        pass->outputs[i].step_count = step_count;
        pass->output_count ++;
    }

    ecs_os_free(fs_prog);

    return fx->pass_count - 1;
}

static
void fx_draw(
    sokol_render_state_t *state,
    SokolFx *fx,
    sg_image *inputs,
    sokol_fx_pass_t *pass,
    sokol_screen_pass_t *screen_pass,
    int32_t width,
    int32_t height)
{
    sokol_resources_t *res = state->resources;

    for (int32_t i = 0; i < pass->output_count; i ++) {
        pass->outputs[i].toggle = (pass->outputs[i].step_count - 1) % 2;
    }

    sg_pass_action load_any = sokol_clear_action((ecs_rgb_t){0}, false, false);
    sg_pass_action load_prev = {
        .colors[0].action = SG_ACTION_LOAD,
        .depth.action = SG_ACTION_LOAD,
        .stencil.action = SG_ACTION_LOAD
    };

    int32_t step_count = pass->step_count;
    int32_t step_last = step_count * pass->loop_count;

    fx_mat_uniforms_t fs_mat_u;
    glm_mat4_copy(state->uniforms.mat_p, fs_mat_u.mat_p);
    glm_mat4_copy(state->uniforms.inv_mat_p, fs_mat_u.inv_mat_p);

    fx_uniforms_t f_u = {
        .t = state->uniforms.t,
        .dt = state->uniforms.dt,
        .aspect = state->uniforms.aspect,
        .near_ = state->uniforms.near_,
        .far_ = state->uniforms.far_,
    };

    for (int32_t s = 0; s < step_last; s ++) {
        int8_t step_cur = s % step_count;
        int8_t loop_cur = s / pass->step_count;
        sokol_fx_step_t *step = &pass->steps[step_cur];

        if (step->loop_count <= loop_cur) {
            continue;
        }

        sokol_fx_output_t *output = &pass->outputs[step->output];
        int8_t toggle = output->toggle;

        bool last = s == (step_last - 1);
        bool first = s == 0;

        if (!last || !screen_pass) {
            if (first) {
                sg_begin_pass(output->pass[toggle], &load_any);
            } else {
                sg_begin_pass(output->pass[toggle], &load_prev);
            }
        } else {
            sg_begin_default_pass(&screen_pass->pass_action, width, height);
        }

        sg_apply_pipeline(pass->pip);

        f_u.target_size[0] = output->width;
        f_u.target_size[1] = output->height;
        sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &(sg_range){
            &f_u, sizeof(fx_uniforms_t) 
        });

        sg_apply_uniforms(SG_SHADERSTAGE_FS, 1, &(sg_range){
            &fs_mat_u, sizeof(fx_mat_uniforms_t)
        });

        if (pass->param_count) {
            sg_apply_uniforms(SG_SHADERSTAGE_FS, 2, &(sg_range){
                step->params, pass->param_count * sizeof(float)
            });
        }

        sg_bindings bind = { .vertex_buffers = { res->quad } };
        bind.fs_images[0] = res->noise_texture;

        for (int32_t i = 0; i < pass->input_count; i ++) {
            sokol_fx_input_t input = step->inputs[i];

            if (s != step_cur) {
                input.pass = -1;
            }

            if (input.pass == -1) {
                /* Previous version of current output (for pingponging) */
                sokol_fx_output_t *io = &pass->outputs[step->output];
                bind.fs_images[i + 1] = io->out[!io->toggle];
            } else if (SOKOL_FX_IS_PASS(input.pass)) {
                /* Pass level input */
                bind.fs_images[i + 1] = fx->pass[input.pass - SOKOL_MAX_FX_INPUTS]
                    .outputs[input.index].out[0];
            } else {
                /* Effect level input */
                bind.fs_images[i + 1] = inputs[input.pass];
            }
        }

        sg_apply_bindings(&bind);

        sg_draw(0, 6, 1);

        sg_end_pass();

        output->toggle = !toggle;
    }
}

sg_image sokol_fx_run(
    SokolFx *fx,
    int32_t input_count,
    sg_image inputs[],
    sokol_render_state_t *state,
    sokol_screen_pass_t *screen_pass)
{
    ecs_assert(input_count == fx->input_count, ECS_INVALID_OPERATION, fx->name);

    int32_t width = state->width;
    int32_t height = state->height;
    int32_t i = 0;

    /* Run passes */
    if (!screen_pass) {
        for (; i < fx->pass_count; i ++) {
            fx_draw(state, fx, inputs, &fx->pass[i], NULL, width, height);
        }
    } else {
        for (; i < fx->pass_count - 1; i ++) {
            fx_draw(state, fx, inputs, &fx->pass[i], NULL, width, height);
        }
        fx_draw(state, fx, inputs, &fx->pass[i], screen_pass, width, height);
    }

    return fx->pass[fx->pass_count - 1].outputs[0].out[0];
}

void sokol_fx_update_size(
    SokolFx *fx, 
    int32_t width,
    int32_t height)
{
    for (int p = 0; p < fx->pass_count; p ++) {
        sokol_fx_pass_t *pass = &fx->pass[p];
        for (int o = 0; o < pass->output_count; o ++) {
            sokol_fx_output_t *output = &pass->outputs[o];
            if (!output->global_size) {
                continue;
            }

            output->width = width;
            output->height = height;
            if (output->factor) {
                output->width *= output->factor;
                output->height *= output->factor;
            }

            sg_destroy_pass(output->pass[0]);
            sg_destroy_image(output->out[0]);
            output->out[0] = sokol_target(fx->name, output->width, 
                output->height, pass->sample_count, pass->mipmap_count, 
                pass->color_format);
            output->pass[0] = sg_make_pass(&(sg_pass_desc){
                .color_attachments[0].image = output->out[0]});

            if (output->step_count > 1) {
                sg_destroy_pass(output->pass[1]);
                sg_destroy_image(output->out[1]);
                output->out[1] = sokol_target(fx->name, output->width, 
                    output->height, pass->sample_count, pass->mipmap_count, 
                    pass->color_format);
                output->pass[1] = sg_make_pass(&(sg_pass_desc){
                    .color_attachments[0].image = output->out[1]});
            }
        }
    }
}
