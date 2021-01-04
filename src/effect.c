#include "private.h"

static
char* build_fs_shader(
    sokol_fx_pass_desc_t pass)
{
    ecs_strbuf_t fs_shader = ECS_STRBUF_INIT;

    ecs_strbuf_appendstr(&fs_shader, 
    "#version 330\n"
    "out vec4 frag_color;\n"
    "in vec2 uv;\n");

    sokol_fx_pass_input_t *input;
    int i = 0;
    while (true) {
        input = &pass.inputs[i];
        if (!input->name) {
            break;
        }

        ecs_strbuf_append(&fs_shader, "uniform sampler2D %s;\n", input->name);
        i ++;
    }

    if (pass.shader_header) {
        ecs_strbuf_appendstr(&fs_shader, pass.shader_header);
    }
    
    ecs_strbuf_appendstr(&fs_shader, "void main() {\n");
    ecs_strbuf_append(&fs_shader, pass.shader);
    ecs_strbuf_append(&fs_shader, "}\n");

    return ecs_strbuf_get(&fs_shader);
}

int sokol_effect_add_pass(
    SokolEffect *fx, 
    sokol_fx_pass_desc_t pass_desc)
{
    sokol_fx_pass_t *pass = &fx->pass[fx->pass_count ++];

    char *fs_shader = build_fs_shader(pass_desc);

    /* Create FX shader */
    sg_shader_desc shd_desc = {
        .vs.source = sokol_vs_passthrough(),
        .fs.source = fs_shader
    };

    sokol_fx_pass_input_t *input;
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

    pass->pass.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {         
            .attrs = {
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

    pass->pass.color_target = sokol_target_rgba8(pass_desc.width, pass_desc.height);
    pass->pass.depth_target = sokol_target_depth(pass_desc.width, pass_desc.height);

    pass->pass.pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = pass->pass.color_target,
        .depth_stencil_attachment.image = pass->pass.depth_target,
        .label = "fx-pass"
    }); 

    return fx->pass_count - 1;
}

static
void effect_pass_draw(
    sokol_resources_t *res,
    SokolEffect *effect,
    sokol_fx_pass_t *fx_pass)
{
    sg_begin_pass(fx_pass->pass.pass, &fx_pass->pass.pass_action);
    sg_apply_pipeline(fx_pass->pass.pip);
    
    sg_bindings bind = {
        .vertex_buffers = { 
            [0] = res->quad 
        }
    };

    int i;
    for (i = 0; i < fx_pass->input_count; i ++) {
        int input = fx_pass->inputs[i];
        bind.fs_images[i] = effect->pass[input].pass.color_target;
    }

    sg_apply_bindings(&bind);

    sg_draw(0, 6, 1);
    sg_end_pass();    
}

SokolEffect sokol_effect_init(
    int32_t input_count)
{
    SokolEffect fx = {};
    fx.input_count = input_count;
    fx.pass_count = input_count;
    return fx;
}

sg_image sokol_effect_run(
    sokol_resources_t *res,
    SokolEffect *effect,
    int32_t input_count,
    sg_image inputs[])
{
    assert(input_count == effect->input_count);

    /* Initialize inputs */
    int i;
    for (i = 0; i < input_count; i ++) {
        effect->pass[i].pass.color_target = inputs[i];
    }

    /* Run passes */
    for (; i < effect->pass_count; i ++) {
        effect_pass_draw(res, effect, &effect->pass[i]);
    }

    return effect->pass[effect->pass_count - 1].pass.color_target;
}
