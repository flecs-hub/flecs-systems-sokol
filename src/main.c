#define SOKOL_IMPL
#include "private_include.h"

typedef struct vs_uniforms_t {
    mat4 mat_vp;
} vs_uniforms_t;

typedef struct fs_uniforms_t {
    vec3 light_ambient;
    vec3 light_direction;
    vec3 light_color;
    vec3 eye_pos;
} fs_uniforms_t;

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
            .size = sizeof(vs_uniforms_t),
            .uniforms = {
                [0] = { .name="u_mat_vp", .type=SG_UNIFORMTYPE_MAT4 }
            }
        }, 
        .fs.uniform_blocks[0] = {
            .size = sizeof(fs_uniforms_t),
            .uniforms = {
                [0] = { .name="u_light_ambient", .type=SG_UNIFORMTYPE_FLOAT3 },
                [1] = { .name="u_light_direction", .type=SG_UNIFORMTYPE_FLOAT3 },
                [2] = { .name="u_light_color", .type=SG_UNIFORMTYPE_FLOAT3 },
                [3] = { .name="u_eye_pos", .type=SG_UNIFORMTYPE_FLOAT3 }
            }
        },            
        .vs.source =
            "#version 330\n"
            "uniform mat4 u_mat_vp;\n"
            "layout(location=0) in vec4 v_position;\n"
            "layout(location=1) in vec3 v_normal;\n"
            "layout(location=2) in vec4 i_color;\n"
            "layout(location=3) in mat4 i_mat_m;\n"
            "out vec4 position;\n"
            "out vec3 normal;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "  gl_Position = u_mat_vp * i_mat_m * v_position;\n"
            "  position = (i_mat_m * v_position);\n"
            "  normal = (i_mat_m * vec4(v_normal, 0.0)).xyz;\n"
            "  color = i_color;\n"
            "}\n",
        .fs.source =
            "#version 330\n"
            "uniform vec3 u_light_ambient;\n"
            "uniform vec3 u_light_direction;\n"
            "uniform vec3 u_light_color;\n"
            "uniform vec3 u_eye_pos;\n"
            "in vec4 position;\n"
            "in vec3 normal;\n"
            "in vec4 color;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "  vec3 l = normalize(u_light_direction);\n"
            "  vec3 n = normalize(normal);\n"
            "  float dot_n_l = dot(n, l);\n"
            "  vec4 ambient = vec4(u_light_ambient, 0) * color;\n"
            "  vec4 diffuse = vec4(u_light_color, 0) * color * dot_n_l;\n"
            "  frag_color = ambient + diffuse;\n"
            "}\n"
    });

    /* create a pipeline object (default render state is fine) */
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers = {
                [2] = { .stride = 16, .step_func=SG_VERTEXSTEP_PER_INSTANCE },
                [3] = { .stride = 64, .step_func=SG_VERTEXSTEP_PER_INSTANCE }
            },

            .attrs = {
                /* Static geometry */
                [0] = { .buffer_index=0, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=1, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },

                /* Color buffer (per instance) */
                [2] = { .buffer_index=2, .offset=0, .format=SG_VERTEXFORMAT_FLOAT4 },

                /* Matrix (per instance) */
                [3] = { .buffer_index=3, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT4 },
                [4] = { .buffer_index=3, .offset=16, .format=SG_VERTEXFORMAT_FLOAT4 },
                [5] = { .buffer_index=3, .offset=32, .format=SG_VERTEXFORMAT_FLOAT4 },
                [6] = { .buffer_index=3, .offset=48, .format=SG_VERTEXFORMAT_FLOAT4 }
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
void init_uniforms(
    ecs_world_t *world,
    EcsCanvas *canvas,
    float aspect,
    vs_uniforms_t *vs_out,
    fs_uniforms_t *fs_out,
    ecs_entity_t ecs_entity(EcsCamera),
    ecs_entity_t ecs_entity(EcsDirectionalLight))
{
    vec3 eye = {0, -4.0, 0.0};
    vec3 center = {0, 0.0, 5.0};
    vec3 up = {0.0, 1.0, 0.0};

    mat4 mat_p;
    mat4 mat_v;

    /* Compute perspective & lookat matrix */
    ecs_entity_t camera = canvas->camera;
    if (camera) {
        /* Cast away const since glm doesn't do const */
        EcsCamera *cam = (EcsCamera*)
            ecs_get(world, camera, EcsCamera);
        ecs_assert(cam != NULL, ECS_INVALID_PARAMETER, NULL);

        glm_perspective(cam->fov, aspect, 0.5, 100.0, mat_p);
        glm_lookat(cam->position, cam->lookat, cam->up, mat_v);
    } else {
        glm_perspective(30, aspect, 0.5, 100.0, mat_p);
        glm_lookat(eye, center, up, mat_v);
    }

    /* Compute view/projection matrix */
    glm_mat4_mul(mat_p, mat_v, vs_out->mat_vp);

    /* Get light parameters */
    ecs_entity_t light = canvas->directional_light;
    if (light) {
        /* Cast away const since glm doesn't do const */
        EcsDirectionalLight *l = (EcsDirectionalLight*)
            ecs_get(world, light, EcsDirectionalLight);
        ecs_assert(l != NULL, ECS_INVALID_PARAMETER, NULL);

        glm_vec3_copy(l->direction, fs_out->light_direction);
        glm_vec3_copy(l->color, fs_out->light_color);
    } else {
        glm_vec3_zero(fs_out->light_direction);
        glm_vec3_zero(fs_out->light_color);
    }

    glm_vec3_copy((float*)&canvas->ambient_light, fs_out->light_ambient);
    glm_vec3_copy(eye, fs_out->eye_pos);
}

static
void SokolRender(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolCanvas *sk_canvas = ecs_column(it, SokolCanvas, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    EcsQuery *q = ecs_column(it, EcsQuery, 3);
    ecs_entity_t ecs_entity(EcsCamera) = ecs_column_entity(it, 4);
    ecs_entity_t ecs_entity(EcsDirectionalLight) = ecs_column_entity(it, 5);

    ecs_query_t *buffers = q->query;
    vs_uniforms_t vs_u;
    fs_uniforms_t fs_u;

    /* Loop each canvas */
    int32_t i;
    for (i = 0; i < it->count; i ++) {
        int w, h;
        SDL_GL_GetDrawableSize(sk_canvas[i].sdl_window, &w, &h);
        float aspect = (float)w / (float)h;

        init_uniforms(world, &canvas[i], aspect, &vs_u, &fs_u,
            ecs_entity(EcsCamera), ecs_entity(EcsDirectionalLight));

        sg_begin_default_pass(&sk_canvas->pass_action, w, h);
        sg_apply_pipeline(sk_canvas->pip);

        /* Loop buffers */
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
                        [1] = buffer[b].normal_buffer,
                        [2] = buffer[b].color_buffer,
                        [3] = buffer[b].transform_buffer
                    },
                    .index_buffer = buffer[b].index_buffer
                };

                sg_apply_bindings(&bind);
                sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_u, sizeof(vs_uniforms_t));
                sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fs_u, sizeof(fs_uniforms_t));

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
        :flecs.components.graphics.Camera,
        :flecs.components.graphics.DirectionalLight);
}
