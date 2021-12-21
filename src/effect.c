#include "private_api.h"

typedef struct fx_uniforms_t {
    float aspect;
} fx_uniforms_t;

static
char* fx_build_shader(
    sokol_fx_pass_desc_t *pass)
{
    ecs_strbuf_t shad = ECS_STRBUF_INIT;

    /* Add fx header */
    ecs_strbuf_appendstr(&shad, 
        SOKOL_SHADER_HEADER
        "out vec4 frag_color;\n"
        "in vec2 uv;\n"
        "uniform float u_aspect;\n");

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

    return ecs_strbuf_get(&shad);
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
                        [0] = { .name="u_aspect", .type=SG_UNIFORMTYPE_FLOAT }
                    }
                },
                [1] = prog_ub
            }
        }
    };

    /* Add inputs to program */
    for (int32_t i = 0; i < SOKOL_MAX_FX_INPUTS; i ++) {
        const char *input = pass_desc->inputs[i];
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
            pass_desc->outputs[step->output].width == 0)
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
        if (!output->width) {
            break;
        }
        if (!output->height) {
            output->height = output->width;
        }

        pass->outputs[i].out[0] = sokol_target(output->width, 
            output->height, pass->sample_count, pass->mipmap_count, color_format);
        pass->outputs[i].pass[0] = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0].image = pass->outputs[i].out[0]
        });

        pass->outputs[i].width = output->width;
        pass->outputs[i].height = output->height;

        int32_t step_count = 0;
        for (int s = 0; s < pass->step_count; s ++) {
            sokol_fx_step_t *step = &pass->steps[s];
            if (step->output == i) {
                step_count ++;
            }
        }

        /* If multiple steps use output, create intermediate buffer */
        if (step_count > 1) {
            pass->outputs[i].out[1] = sokol_target(output->width, 
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
    sokol_resources_t *res,
    SokolFx *fx,
    sg_image *inputs,
    sokol_fx_pass_t *pass,
    sokol_screen_pass_t *screen_pass,
    int32_t width,
    int32_t height)
{
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

        fx_uniforms_t fs_u = {
            .aspect = (float)width / (float)height
        };

        sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &(sg_range){
            &fs_u, sizeof(fx_uniforms_t)
        });

        if (pass->param_count) {
            sg_apply_uniforms(SG_SHADERSTAGE_FS, 1, &(sg_range){
                step->params, pass->param_count * sizeof(float)
            });
        }

        sg_bindings bind = { .vertex_buffers = { res->quad } };
        for (int32_t i = 0; i < pass->input_count; i ++) {
            sokol_fx_input_t input = step->inputs[i];

            if (s != step_cur) {
                input.pass = -1;
            }

            if (input.pass == -1) {
                /* Previous version of current output (for pingponging) */
                sokol_fx_output_t *io = &pass->outputs[step->output];
                bind.fs_images[i] = io->out[!io->toggle];
            } else if (SOKOL_FX_IS_PASS(input.pass)) {
                /* Pass level input */
                bind.fs_images[i] = fx->pass[input.pass - SOKOL_MAX_FX_INPUTS]
                    .outputs[input.index].out[0];
            } else {
                // printf("bind %d @ %s.%s to input %d\n", i, pass->name, step->name, input.pass);
                /* Effect level input */
                bind.fs_images[i] = inputs[input.pass];
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
    sokol_resources_t *res = state->resources;
    int32_t i = 0;

    /* Run passes */
    if (!screen_pass) {
        for (; i < fx->pass_count; i ++) {
            fx_draw(res, fx, inputs, &fx->pass[i], NULL, width, height);
        }
    } else {
        for (; i < fx->pass_count - 1; i ++) {
            fx_draw(res, fx, inputs, &fx->pass[i], NULL, width, height);
        }
        fx_draw(res, fx, inputs, &fx->pass[i], screen_pass, width, height);
    }

    return fx->pass[fx->pass_count - 1].outputs[0].out[0];
}
