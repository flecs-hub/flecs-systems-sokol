#define SOKOL_IMPL
#include "private_include.h"

typedef struct uniforms_t {
    mat4 mat_vp;
} uniforms_t;

typedef struct SokolCanvas {
    SDL_Window* sdl_window;
    SDL_GLContext gl_context;
	sg_pass_action pass_action;
    sg_pipeline pip;
} SokolCanvas;

static
sg_pass_action init_pass_action(
    const EcsCanvas *canvas) 
{
    ecs_rgb_t bg_color = canvas->background_color;

    return (sg_pass_action) {
        .colors[0] = {
            .action = SG_ACTION_CLEAR, 
            .val = {
                bg_color.r,
                bg_color.g,
                bg_color.b,
                1.0f 
            }
        } 
    };
}

static
sg_pipeline init_pipeline(void) {
    /* create an instancing shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.uniform_blocks[0] = {
            .size = sizeof(uniforms_t),
            .uniforms = {
                [0] = { .name="mat_vp", .type=SG_UNIFORMTYPE_MAT4 }
            }
        },        
        .vs.source =
            "#version 330\n"
            "uniform mat4 mat_vp;\n"
            "layout(location=0) in vec4 position;\n"
            "layout(location=1) in vec4 color0;\n"
            "layout(location=2) in mat4 mat_m;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "  gl_Position = mat_vp * mat_m * position;\n"
            "  color = color0;\n"
            "}\n",
        .fs.source =
            "#version 330\n"
            "in vec4 color;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "  frag_color = color;\n"
            "}\n"
    });

    /* create a pipeline object (default render state is fine) */
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers = {
                [1] = { .stride = 16, .step_func=SG_VERTEXSTEP_PER_INSTANCE },
                [2] = { .stride = 64, .step_func=SG_VERTEXSTEP_PER_INSTANCE }
            },

            .attrs = {
                /* Static geometry */
                [0] = {.offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },

                /* Color buffer (per instance) */
                [1] = { .buffer_index=1,  .offset=0, .format=SG_VERTEXFORMAT_FLOAT4 },

                /* Matrix (per instance) */
                [2] = { .buffer_index=2,  .offset=0,  .format=SG_VERTEXFORMAT_FLOAT4 },
                [3] = { .buffer_index=2,  .offset=16, .format=SG_VERTEXFORMAT_FLOAT4 },
                [4] = { .buffer_index=2,  .offset=32, .format=SG_VERTEXFORMAT_FLOAT4 },
                [5] = { .buffer_index=2,  .offset=48, .format=SG_VERTEXFORMAT_FLOAT4 }
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true
        },
        .rasterizer.cull_mode = SG_CULLMODE_BACK
    });
}

static
void SokolSetCanvas(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    Sdl2Window *window = ecs_column(it, Sdl2Window, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    ecs_entity_t ecs_entity(SokolCanvas) = ecs_column_entity(it, 3);

    for (int32_t i = 0; i < it->count; i ++) {
        SDL_Window *sdl_window = window->window;

        SDL_GLContext ctx = SDL_GL_CreateContext(sdl_window);

        sg_setup(&(sg_desc) {0});
        assert(sg_isvalid());
        ecs_trace_1("sokol initialized");

        ecs_set(world, it->entities[i], SokolCanvas, {
            .sdl_window = sdl_window,
            .gl_context = ctx,
            .pass_action = init_pass_action(&canvas[i]),
            .pip = init_pipeline()
        });

        ecs_trace_1("sokol canvas initialized");

        ecs_set(world, it->entities[i], EcsQuery, {
            ecs_query_new(world, "[in] flecs.systems.sokol.Buffer")
        });

        sokol_init_buffers(world);

        ecs_trace_1("sokol buffer support initialized");
    }
}

static
void SokolUnsetCanvas(ecs_iter_t *it) {
    SokolCanvas *canvas = ecs_column(it, SokolCanvas, 1);

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        sg_shutdown();
        SDL_GL_DeleteContext(canvas[i].gl_context);
    }
}

static
void SokolRender(ecs_iter_t *it) {
    SokolCanvas *sk_canvas = ecs_column(it, SokolCanvas, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    EcsQuery *q = ecs_column(it, EcsQuery, 3);
    ecs_entity_t ecs_entity(EcsCamera) = ecs_column_entity(it, 4);

    ecs_query_t *buffers = q->query;
    uniforms_t u;
    mat4 mat_p;
    mat4 mat_v;

    /* Default values */
    vec3 eye = {0, -4.0, 0.0};
    vec3 center = {0, 0.0, 5.0};
    vec3 up = {0.0, 1.0, 0.0};

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        int w, h;
        SDL_GL_GetDrawableSize(sk_canvas->sdl_window, &w, &h);
        float aspect = (float)w / (float)h;

        /* Compute perspective & lookat matrix */
        ecs_entity_t camera = canvas->camera;
        if (camera) {
            EcsCamera *cam = ecs_get_mut(it->world, camera, EcsCamera, NULL);
            ecs_assert(cam != NULL, ECS_INTERNAL_ERROR, NULL);
            glm_perspective(cam->fov, aspect, 0.1, 1000.0, mat_p);
            glm_lookat(cam->position, cam->lookat, cam->up, mat_v);
        } else {
            glm_perspective(30, aspect, 0.1, 100.0, mat_p);
            glm_lookat(eye, center, up, mat_v);
        }

        /* Compute view/projection matrix */
        glm_mat4_mul(mat_p, mat_v, u.mat_vp);        

        sg_begin_default_pass(&sk_canvas->pass_action, w, h);
        sg_apply_pipeline(sk_canvas->pip);

        ecs_iter_t qit = ecs_query_iter(buffers);
        while (ecs_query_next(&qit)) {
            SokolBuffer *buffer = ecs_column(&qit, SokolBuffer, 1);
            
            int b;
            for (b = 0; b < qit.count; b ++) {
                if (!buffer[b].instance_count) {
                    continue;
                }
                sg_bindings bind = {
                    .vertex_buffers = {
                        [0] = buffer[b].vertex_buffer,
                        [1] = buffer[b].color_buffer,
                        [2] = buffer[b].transform_buffer
                    },
                    .index_buffer = buffer[b].index_buffer
                };

                sg_apply_bindings(&bind);
                sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &u, sizeof(uniforms_t));

                sg_draw(0, buffer[b].index_count, buffer[b].instance_count);
            }
        }

        sg_end_pass();

        sg_commit();
        SDL_GL_SwapWindow(sk_canvas->sdl_window);
    }
}

void FlecsSystemsSokolImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokol);

    ecs_set_name_prefix(world, "Sokol");
    
    ECS_IMPORT(world, FlecsSystemsSokolBuffer);
    ECS_IMPORT(world, FlecsSystemsSdl2);
    ECS_IMPORT(world, FlecsComponentsGui);

    ECS_COMPONENT(world, SokolCanvas);

    ECS_SYSTEM(world, SokolSetCanvas, EcsOnSet,
        flecs.systems.sdl2.window.Window,
        flecs.components.gui.Canvas,
        :Canvas);

    ECS_SYSTEM(world, SokolUnsetCanvas, EcsUnSet, 
        Canvas);

    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        Canvas, 
        flecs.components.gui.Canvas, 
        flecs.system.Query,
        :flecs.components.gui.Camera);     
}
