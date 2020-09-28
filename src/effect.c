#include "private_include.h"

static
const char *vs_fx_shader = 
    "#version 330\n"
    "layout(location=0) in vec4 v_position;\n"
    "layout(location=1) in vec2 v_uv;\n"
    "out vec2 uv;\n"
    "void main() {\n"
    "  gl_Position = v_position;\n"
    "  uv = v_uv;\n"
    "}\n";

static
char* build_fs_shader(
    sokol_pass_desc_t pass)
{
    ecs_strbuf_t fs_shader = ECS_STRBUF_INIT;

    ecs_strbuf_appendstr(&fs_shader, 
    "#version 330\n"
    "out vec4 frag_color;\n"
    "in vec2 uv;\n");

    sokol_pass_input_t *input;
    int i = 0;
    while (true) {
        input = &pass.inputs[i];
        if (!input->name) {
            break;
        }

        ecs_strbuf_append(&fs_shader, "uniform sampler2D %s;\n", input->name);
        i ++;
    }
    
    ecs_strbuf_appendstr(&fs_shader, "void main() {\n");
    ecs_strbuf_append(&fs_shader, pass.shader);
    ecs_strbuf_append(&fs_shader, "}\n");

    return ecs_strbuf_get(&fs_shader);
}

int sokol_effect_add_pass(
    SokolEffect *fx, 
    sokol_pass_desc_t pass_desc)
{
    sokol_pass_t *pass = &fx->pass[fx->pass_count ++];

    char *fs_shader = build_fs_shader(pass_desc);

    /* Create FX shader */
    sg_shader_desc shd_desc = {
        .vs.source = vs_fx_shader,
        .fs.source = fs_shader
    };

    sokol_pass_input_t *input;
    int i = 0;
    while (true) {
        input = &pass_desc.inputs[i];
        if (!input->name) {
            break;
        }
        shd_desc.fs.images[i].name = input->name;
        shd_desc.fs.images[i].type = SG_IMAGETYPE_2D;
        pass->inputs[i] = input->id;
        i ++;
    }
    sg_shader shd = sg_make_shader(&shd_desc);
    pass->input_count = i;

    /* Create fx pipeline */
    pass->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {         
            .attrs = {
                /* Static geometry (position, uv) */
                [0] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT2 }
            }
        },
        .blend = {
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH
        },        
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true
        }
    });

    /* Create output texture */ 
    pass->output = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = pass_desc.width,
        .height = pass_desc.height,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,        
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .sample_count = 1,
        .label = "color-image"
    });

    /* Create output depth texture */ 
    pass->depth_output = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = pass_desc.width,
        .height = pass_desc.height,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .sample_count = 1,
        .label = "depth-image"
    });

    pass->pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = pass->output,
        .depth_stencil_attachment.image = pass->depth_output,
        .label = "fx-pass"
    }); 

    return fx->pass_count;
}

static
void effect_pass_draw(
    SokolCanvas *sk_canvas,
    SokolEffect *effect,
    sokol_pass_t *fx_pass,
    sg_image input_0)
{
    sg_begin_pass(fx_pass->pass, &fx_pass->pass_action);
    sg_apply_pipeline(fx_pass->pip);
    
    sg_bindings bind = {
        .vertex_buffers = { 
            [0] = sk_canvas->offscreen_quad 
        }
    };

    int i;
    for (i = 0; i < fx_pass->input_count; i ++) {
        int input = fx_pass->inputs[i];
        if (!input) {
            bind.fs_images[i] = input_0;
        } else {
            bind.fs_images[i] = effect->pass[input - 1].output;
        }
    }

    sg_apply_bindings(&bind);

    sg_draw(0, 6, 1);
    sg_end_pass();    
}

sg_image sokol_effect_run(
    SokolCanvas *sk_canvas,
    SokolEffect *effect,
    sg_image input)
{
    int i;
    for (i = 0; i < effect->pass_count; i ++) {
        effect_pass_draw(sk_canvas, effect, &effect->pass[i], input);
    }

    return effect->pass[effect->pass_count - 1].output;
}
