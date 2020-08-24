#include <flecs_systems_sokol.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include <flecs-systems-sokol/sokol_gfx.h>

typedef struct sokol_vert3_t {
    ecs_vert3_t pos;
    ecs_rgba_t color;
} sokol_vert3_t;

typedef struct SokolCanvas {
    SDL_Window* sdl_window;
    SDL_GLContext gl_context;
	sg_pass_action pass_action;
    sg_pipeline pip;
} SokolCanvas;

typedef struct SokolBuffer {
    sg_buffer buffer;
    sg_buffer index_buffer;
    void *vertices;
    uint16_t *indices;
    int32_t count;
    int32_t index_count;
} SokolBuffer;

ECS_CTOR(SokolBuffer, ptr, {
    ptr->buffer = (sg_buffer){ 0 };
    ptr->index_buffer = (sg_buffer){ 0 };
    ptr->vertices = NULL;
    ptr->indices = NULL;
    ptr->count = 0;
    ptr->index_count = 0;
});

ECS_DTOR(SokolBuffer, ptr, {
    ecs_os_free(ptr->vertices);
    ecs_os_free(ptr->indices);
});

static
void SokolSetCanvas(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    Sdl2Window *window = ecs_column(it, Sdl2Window, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    ecs_entity_t ecs_entity(SokolCanvas) = ecs_column_entity(it, 3);

    for (int32_t i = 0; i < it->count; i ++) {
        SDL_Window *sdl_window = window->window;
        ecs_rgb_t bg_color = canvas[i].background_color;

        SDL_GLContext ctx = SDL_GL_CreateContext(sdl_window);

        sg_setup(&(sg_desc) {0});
        assert(sg_isvalid());

        /* create a shader (use vertex attribute locations) */
        sg_shader shd = sg_make_shader(&(sg_shader_desc){
            .vs.source =
                "#version 330\n"
                "layout(location=0) in vec4 position;\n"
                "layout(location=1) in vec4 color0;\n"
                "out vec4 color;\n"
                "void main() {\n"
                "  gl_Position = position;\n"
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
        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = shd,
            .index_type = SG_INDEXTYPE_UINT16,
            .layout = {
                /* test to provide attr offsets, but no buffer stride, this should compute the stride */
                .attrs = {
                    /* vertex attrs can also be bound by location instead of name (but not in GLES2) */
                    [0] = { .offset=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                    [1] = { .offset=12, .format=SG_VERTEXFORMAT_FLOAT4 }
                }
            }
        });

        sg_pass_action pass_action = (sg_pass_action) {
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

        ecs_set(world, it->entities[i], SokolCanvas, {
            .sdl_window = sdl_window,
            .gl_context = ctx,
            .pass_action = pass_action,
            .pip = pip
        });

        ecs_set(world, it->entities[i], EcsQuery, {
            ecs_query_new(world, "[in] flecs.systems.sokol.Buffer")
        });        

        ecs_trace_1("sokol initialized");
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
    SokolCanvas *canvas = ecs_column(it, SokolCanvas, 1);
    EcsQuery *q = ecs_column(it, EcsQuery, 2);
    ecs_query_t *buffers = q->query;

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        int w, h;
        SDL_GL_GetDrawableSize(canvas->sdl_window, &w, &h);
        sg_begin_default_pass(&canvas->pass_action, w, h);
        sg_apply_pipeline(canvas->pip);

        ecs_iter_t qit = ecs_query_iter(buffers);
        while (ecs_query_next(&qit)) {
            SokolBuffer *buffer = ecs_column(&qit, SokolBuffer, 1);

            int b;
            for (b = 0; b < qit.count; b ++) {
                sg_bindings bind = {
                    .vertex_buffers[0] = buffer[b].buffer,
                    .index_buffer = buffer[b].index_buffer
                };

                sg_apply_bindings(&bind);
                sg_draw(0, buffer->index_count, 1);
            }
        }

        sg_end_pass();
        sg_commit();
        SDL_GL_SwapWindow(canvas->sdl_window);
    }
}

static
void SokolAttachRect(ecs_iter_t *it) {
    EcsQuery *q = ecs_column(it, EcsQuery, 1);
    ecs_query_t *query = q->query;

    if (ecs_query_changed(query)) {
        SokolBuffer *b = ecs_column(it, SokolBuffer, 2);
        ecs_iter_t qit = ecs_query_iter(query);
        
        int32_t count = 0;
        while (ecs_query_next(&qit)) {
            count += qit.count;
        }

        if (!count) {
            if (!b->count) {
                /* Nothing to be done */
            }
        } else {
            sokol_vert3_t *vertices = b->vertices;
            uint16_t *indices = b->indices;
            int32_t vertices_count = b->count;
            bool is_new = false;

            if (!vertices) {
                is_new = true;
            }

            int size = count * sizeof(sokol_vert3_t) * 4;
            int index_size = count * 6 * sizeof(uint16_t);

            if (vertices_count < count) {
                vertices = ecs_os_realloc(vertices, size);
                indices = ecs_os_realloc(indices, index_size);
            }

            int32_t vi = 0, ii = 0;

            ecs_iter_t qit = ecs_query_iter(query);
            while (ecs_query_next(&qit)) {
                EcsPosition3 *p = ecs_column(&qit, EcsPosition3, 1);
                EcsRectangle *r = ecs_column(&qit, EcsRectangle, 2);

                int i;
                for (i = 0; i < qit.count; i ++) {
                    float x = p[i].x;
                    float y = p[i].y;
                    float w = r[i].width / 2.0;
                    float h = r[i].height / 2.0;

                    vertices[vi ++] = (sokol_vert3_t){
                        .pos = {x - w, y - h, 0},
                        .color = {1.0, 0.5, 0.5, 1.0}
                    };

                    vertices[vi ++] = (sokol_vert3_t){
                        .pos = {x + w, y - h, 0},
                        .color = {0.5, 1.0, 0.5, 1.0}
                    };

                    vertices[vi ++] = (sokol_vert3_t){
                        .pos = {x + w, y + h, 0},
                        .color = {0.5, 0.5, 1.0, 1.0}
                    };

                    vertices[vi ++] = (sokol_vert3_t){
                        .pos = {x - w, y + h, 0},
                        .color = {0.5, 1.0, 1.0, 1.0}
                    };

                    indices[ii ++] = 0;
                    indices[ii ++] = 1;
                    indices[ii ++] = 2;

                    indices[ii ++] = 0;
                    indices[ii ++] = 2;
                    indices[ii ++] = 3;          
                }
            }

            b->vertices = vertices;
            b->indices = indices;
            b->count = count;
            b->index_count = count * 6;

            if (is_new) {
                b->buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = size,
                    .content = vertices,
                });

                b->index_buffer = sg_make_buffer(&(sg_buffer_desc){
                    .size = index_size,
                    .content = indices,
                    .type = SG_BUFFERTYPE_INDEXBUFFER
                });
            } else {
                sg_update_buffer(b->buffer, vertices, size);
                sg_update_buffer(b->index_buffer, indices, index_size);
            }
        }
    }
}

void FlecsSystemsSokolImport(
    ecs_world_t *world)
{
    ECS_MODULE(world, FlecsSystemsSokol);

    ecs_set_name_prefix(world, "Sokol");
    
    ECS_IMPORT(world, FlecsSystemsSdl2);
    ECS_IMPORT(world, FlecsComponentsGui);
    ECS_IMPORT(world, FlecsComponentsTransform);
    ECS_IMPORT(world, FlecsComponentsGeometry);

    ECS_COMPONENT(world, SokolCanvas);
    ECS_COMPONENT(world, SokolBuffer);

    ecs_set_component_actions(world, SokolBuffer, {
        .ctor = ecs_ctor(SokolBuffer),
        .dtor = ecs_dtor(SokolBuffer)
    });

    ECS_SYSTEM(world, SokolSetCanvas, EcsOnSet,
        flecs.systems.sdl2.window.Window,
        flecs.components.gui.Canvas,
        :Canvas);

    ECS_SYSTEM(world, SokolUnsetCanvas, EcsUnSet, 
        Canvas);        

    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        Canvas, flecs.system.Query);

    ECS_ENTITY(world, SokolRectangleBuffer, 
        Buffer, flecs.system.Query);

        ecs_set(world, SokolRectangleBuffer, EcsQuery, {
            ecs_query_new(world, 
                "[in] flecs.components.transform.Position3,"
                "[in] flecs.components.geometry.Rectangle")
        });

    ECS_SYSTEM(world, SokolAttachRect, EcsPostLoad, 
        RectangleBuffer:flecs.system.Query, 
        RectangleBuffer:Buffer);
}
