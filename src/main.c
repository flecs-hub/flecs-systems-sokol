#define SOKOL_IMPL
#include "private_include.h"

#define FS_MAX_MATERIALS (255)
#define SHADOW_MAP_SIZE (4096)

typedef struct vs_uniforms_t {
    mat4 mat_vp;
    mat4 light_mat_vp;
} vs_uniforms_t;

typedef struct fs_uniforms_t {
    vec3 light_ambient;
    vec3 light_direction;
    vec3 light_color;
    vec3 eye_pos;
    float shadow_map_size;
} fs_uniforms_t;

typedef struct vs_material_t {
    float specular_power;
    float shininess;
    float emissive;
} vs_material_t;

typedef struct vs_materials_t {
    vs_material_t array[FS_MAX_MATERIALS];
} vs_materials_t;

static
sg_buffer init_quad() {
    float quad_data[] = {
        -1.0f, -1.0f,  0.0f,   0, 0,
         1.0f, -1.0f,  0.0f,   1, 0,
         1.0f,  1.0f,  0.0f,   1, 1,

        -1.0f, -1.0f,  0.0f,   0, 0,
         1.0f,  1.0f,  0.0f,   1, 1,
        -1.0f,  1.0f,  0.0f,   0, 1
    };

    return sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_data),
        .content = quad_data,
        .usage = SG_USAGE_IMMUTABLE
    });
}

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
sg_pass_action init_tex_pass_action(void) 
{
    return (sg_pass_action) {
        .colors[0] = {
            .action = SG_ACTION_CLEAR, 
            .val = {0, 0, 0}
        } 
    };
}

sg_image sokol_init_render_target_8(
    int32_t width, 
    int32_t height) 
{
    sg_image_desc img_desc = {
        .render_target = true,
        .width = width,
        .height = height,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .sample_count = 1,
        .label = "color-image"
    };

    return sg_make_image(&img_desc);
}

sg_image sokol_init_render_target_16(
    int32_t width, 
    int32_t height) 
{
    sg_image_desc img_desc = {
        .render_target = true,
        .width = width,
        .height = height,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .pixel_format = SG_PIXELFORMAT_RGBA16F,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .sample_count = 1,
        .label = "color-image"
    };

    return sg_make_image(&img_desc);
}

sg_image sokol_init_render_depth_target(
    int32_t width, 
    int32_t height) 
{
    sg_image_desc img_desc = {
        .render_target = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .sample_count = 1,
        .label = "depth-image"
    };

    return sg_make_image(&img_desc);
}

static
sg_pass init_offscreen_pass(
    sg_image img, 
    sg_image depth_img) 
{
    return sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = img,
        .depth_stencil_attachment.image = depth_img,
        .label = "offscreen-pass"
    });    
}

static
sg_pipeline init_pipeline(void) {
    /* create an instancing shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.uniform_blocks = {
            [0] = {
                .size = sizeof(vs_uniforms_t),
                .uniforms = {
                    [0] = { .name="u_mat_vp", .type=SG_UNIFORMTYPE_MAT4 },
                    [1] = { .name="u_light_vp", .type=SG_UNIFORMTYPE_MAT4 }
                },
            },
            [1] = {
                .size = sizeof(vs_materials_t),
                .uniforms = {
                    [0] = { .name="u_materials", .type=SG_UNIFORMTYPE_FLOAT3, .array_count=FS_MAX_MATERIALS}
                }
            } 
        },
        .fs = {
            .images = {
                [0] = {
                    .name = "shadow_map",
                    .type = SG_IMAGETYPE_2D
                }
            },
            .uniform_blocks = {
                [0] = {
                    .size = sizeof(fs_uniforms_t),
                    .uniforms = {
                        [0] = { .name="u_light_ambient", .type=SG_UNIFORMTYPE_FLOAT3 },
                        [1] = { .name="u_light_direction", .type=SG_UNIFORMTYPE_FLOAT3 },
                        [2] = { .name="u_light_color", .type=SG_UNIFORMTYPE_FLOAT3 },
                        [3] = { .name="u_eye_pos", .type=SG_UNIFORMTYPE_FLOAT3 },
                        [4] = { .name="u_shadow_map_size", .type=SG_UNIFORMTYPE_FLOAT }
                    }
                }
            }        
        },
        .vs.source =
            "#version 330\n"
            "uniform mat4 u_mat_vp;\n"
            "uniform mat4 u_light_vp;\n"
            "uniform vec3 u_materials[255];\n"
            "layout(location=0) in vec4 v_position;\n"
            "layout(location=1) in vec3 v_normal;\n"
            "layout(location=2) in vec4 i_color;\n"
            "layout(location=3) in uint i_material;\n"
            "layout(location=4) in mat4 i_mat_m;\n"
            "out vec4 position;\n"
            "out vec4 light_position;\n"
            "out vec3 normal;\n"
            "out vec4 color;\n"
            "out vec3 material;\n"
            "flat out uint material_id;\n"
            "void main() {\n"
            "  gl_Position = u_mat_vp * i_mat_m * v_position;\n"
            "  light_position = u_light_vp * i_mat_m * v_position;\n"
            "  position = (i_mat_m * v_position);\n"
            "  normal = (i_mat_m * vec4(v_normal, 0.0)).xyz;\n"
            "  color = i_color;\n"
            "  material = u_materials[i_material];\n"
            "}\n",
        .fs.source =
            "#version 330\n"
            "uniform vec3 u_light_ambient;\n"
            "uniform vec3 u_light_direction;\n"
            "uniform vec3 u_light_color;\n"
            "uniform vec3 u_eye_pos;\n"
            "uniform float u_shadow_map_size;\n"
            "uniform sampler2D shadow_map;\n"
            "in vec4 position;\n"
            "in vec4 light_position;\n"
            "in vec3 normal;\n"
            "in vec4 color;\n"
            "in vec3 material;\n"
            "out vec4 frag_color;\n"

            "const int pcf_count = 2;\n"
            "const int pcf_samples = (2 * pcf_count + 1) * (2 * pcf_count + 1);\n"
            "const float texel_c = 1.0;\n"

            "float decodeDepth(vec4 rgba) {\n"
            "    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));\n"
            "}\n"

            "float sampleShadow(sampler2D shadowMap, vec2 uv, float compare) {\n"
            "    float depth = decodeDepth(texture(shadowMap, vec2(uv.x, uv.y)));\n"
            "    depth += 0.00001;\n"
            "    return step(compare, depth);\n"
            "}\n"

            "float sampleShadowPCF(sampler2D shadowMap, vec2 uv, float texel_size, float compare) {\n"
            "    float result = 0.0;\n"
            "    for (int x = -pcf_count; x <= pcf_count; x++) {\n"
            "        for (int y = -pcf_count; y <= pcf_count; y++) {\n"
            "            result += sampleShadow(shadowMap, uv + vec2(x, y) * texel_size * texel_c, compare);\n"
            "        }\n"
            "    }\n"
            "    return result / pcf_samples;\n"
            "}\n"

            "void main() {\n"
            "  float specular_power = material.x;\n"
            "  float shininess = material.y;\n"
            "  float emissive = material.z;\n"
            "  vec4 ambient = vec4(u_light_ambient, 0);\n"
            "  vec3 l = normalize(u_light_direction);\n"
            "  vec3 n = normalize(normal);\n"
            "  float dot_n_l = dot(n, l);\n"

            "  if (dot_n_l >= 0.0) {"
            "    vec3 v = normalize(u_eye_pos - position.xyz);\n"
            "    vec3 r = reflect(-l, n);\n"

            "    vec3 light_pos = light_position.xyz / light_position.w;\n"
            "    vec2 sm_uv = (light_pos.xy + 1.0) * 0.5;\n"
            "    float depth = light_position.z;\n"
            "    float texel_size = 1.0 / u_shadow_map_size;\n"
            "    float s = sampleShadowPCF(shadow_map, sm_uv, texel_size, depth);\n"

            "    float r_dot_v = max(dot(r, v), 0.0);\n"
            "    vec4 specular = vec4(specular_power * pow(r_dot_v, shininess) * dot_n_l * u_light_color, 0);\n"
            "    vec4 diffuse = vec4(u_light_color, 0) * dot_n_l;\n"
            "    vec4 light = emissive + clamp(1.0 - emissive, 0, 1.0) * (ambient + s * diffuse);\n"
            "    frag_color = light * color + s * specular;\n"
            "  } else {\n"
            "    vec4 light = emissive + clamp(1.0 - emissive, 0, 1.0) * (ambient);\n"
            "    frag_color = light * color;\n"
            "  }\n"
            "}\n"
    });

    /* create a pipeline object (default render state is fine) */
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers = {
                [2] = { .stride = 16, .step_func=SG_VERTEXSTEP_PER_INSTANCE },
                [3] = { .stride = 4,  .step_func=SG_VERTEXSTEP_PER_INSTANCE },
                [4] = { .stride = 64, .step_func=SG_VERTEXSTEP_PER_INSTANCE }
            },

            .attrs = {
                /* Static geometry */
                [0] = { .buffer_index=0, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=1, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },

                /* Color buffer (per instance) */
                [2] = { .buffer_index=2, .offset=0, .format=SG_VERTEXFORMAT_FLOAT4 },

                /* Material id buffer (per instance) */
                [3] = { .buffer_index=3, .offset=0, .format=SG_VERTEXFORMAT_FLOAT },                

                /* Matrix (per instance) */
                [4] = { .buffer_index=4, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT4 },
                [5] = { .buffer_index=4, .offset=16, .format=SG_VERTEXFORMAT_FLOAT4 },
                [6] = { .buffer_index=4, .offset=32, .format=SG_VERTEXFORMAT_FLOAT4 },
                [7] = { .buffer_index=4, .offset=48, .format=SG_VERTEXFORMAT_FLOAT4 }
            }
        },
        .blend = {
            .color_format = SG_PIXELFORMAT_RGBA16F,
            .depth_format = SG_PIXELFORMAT_DEPTH
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true
        },
        .rasterizer.cull_mode = SG_CULLMODE_BACK
    });
}

static
sg_pipeline init_tex_pipeline() {
    /* create an instancing shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source =
            "#version 330\n"
            "layout(location=0) in vec4 v_position;\n"
            "layout(location=1) in vec2 v_uv;\n"
            "out vec2 uv;\n"
            "void main() {\n"
            "  gl_Position = v_position;\n"
            "  uv = v_uv;\n"
            "}\n",
        .fs = {
            .source =
                "#version 330\n"
                "uniform sampler2D tex;\n"
                "out vec4 frag_color;\n"
                "in vec2 uv;\n"
                "void main() {\n"
                "  frag_color = texture(tex, uv);\n"
                "}\n"
                ,
            .images[0] = {
                .name = "tex",
                .type = SG_IMAGETYPE_2D
            }
        }
    });

    /* create a pipeline object (default render state is fine) */
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {         
            .attrs = {
                /* Static geometry (position, uv) */
                [0] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT2 }
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = false
        }
    });
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
        glm_vec3_copy(cam->position, fs_out->eye_pos);
    } else {
        glm_perspective(30, aspect, 0.5, 100.0, mat_p);
        glm_lookat(eye, center, up, mat_v);
        glm_vec3_copy(eye, fs_out->eye_pos);
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

    fs_out->shadow_map_size = SHADOW_MAP_SIZE;
}

static
void init_materials(
    ecs_query_t *mat_q, 
    vs_materials_t *mat_u)
{
    ecs_iter_t qit = ecs_query_iter(mat_q);
    while (ecs_query_next(&qit)) {
        SokolMaterial *mat = ecs_column(&qit, SokolMaterial, 1);
        EcsSpecular *spec = ecs_column(&qit, EcsSpecular, 2);
        EcsEmissive *em = ecs_column(&qit, EcsEmissive, 3);

        int i;
        if (spec) {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].specular_power = spec[i].specular_power;
                mat_u->array[id].shininess = spec[i].shininess;
            }
        } else {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].specular_power = 0;
                mat_u->array[id].shininess = 0;
            }            
        }

        if (em) {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].emissive = em[i].value;
            }
        } else {
            for (i = 0; i < qit.count; i ++) {
                uint16_t id = mat[i].material_id;
                mat_u->array[id].emissive = 0;
            }
        }
    }    
}

static
void SokolSetRenderer(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    Sdl2Window *window = ecs_column(it, Sdl2Window, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    ecs_entity_t ecs_entity(SokolRenderer) = ecs_column_entity(it, 3);
    ecs_entity_t ecs_entity(SokolGeometry) = ecs_column_entity(it, 4);
    ecs_entity_t ecs_entity(SokolMaterial) = ecs_column_entity(it, 5);

    for (int32_t i = 0; i < it->count; i ++) {
        int w, h;
        SDL_Window *sdl_window = window->window;
        SDL_GLContext ctx = SDL_GL_CreateContext(sdl_window);
        SDL_GL_GetDrawableSize(sdl_window, &w, &h);

        sg_setup(&(sg_desc) {0});
        assert(sg_isvalid());
        ecs_trace_1("sokol initialized");

        sg_image offscreen_tex = sokol_init_render_target_16(w, h);
        sg_image offscreen_depth_tex = sokol_init_render_depth_target(w, h);

        ecs_set(world, it->entities[i], SokolRenderer, {
            .sdl_window = sdl_window,
            .gl_context = ctx,
            .pass_action = init_pass_action(&canvas[i]),
            .tex_pass_action = init_tex_pass_action(),
            .pip = init_pipeline(),
            .tex_pip = init_tex_pipeline(),
            .offscreen_tex = offscreen_tex,
            .offscreen_depth_tex = offscreen_depth_tex,
            .offscreen_pass = init_offscreen_pass(offscreen_tex, offscreen_depth_tex),
            .offscreen_quad = init_quad(),
            .shadow_pass = sokol_init_shadow_pass(SHADOW_MAP_SIZE),
            .fx_bloom = sokol_init_bloom(w, h)
        });

        ecs_trace_1("sokol canvas initialized");

        ecs_set_trait(world, it->entities[i], SokolGeometry, EcsQuery, {
            ecs_query_new(world, "[in] flecs.systems.sokol.Geometry")
        });

        ecs_set_trait(world, it->entities[i], SokolMaterial, EcsQuery, {
            ecs_query_new(world, 
                "[in] flecs.systems.sokol.Material,"
                "[in] ?flecs.components.graphics.Specular,"
                "[in] ?flecs.components.graphics.Emissive,"
                "     ?Prefab")
        });

        sokol_init_buffers(world);

        ecs_trace_1("sokol buffer support initialized");
    }
}

static
void SokolUnsetRenderer(ecs_iter_t *it) {
    SokolRenderer *canvas = ecs_column(it, SokolRenderer, 1);

    int32_t i;
    for (i = 0; i < it->count; i ++) {
        sg_shutdown();
        SDL_GL_DeleteContext(canvas[i].gl_context);
    }
}

static
void SokolRegisterMaterial(ecs_iter_t *it) {
    ecs_entity_t ecs_entity(SokolMaterial) = ecs_column_entity(it, 1);

    static uint16_t next_material = 1; /* Material 0 is the default material */

    int i;
    for (i = 0; i < it->count; i ++) {
        ecs_set(it->world, it->entities[i], SokolMaterial, {
            next_material ++
        });
    }

    ecs_assert(next_material < FS_MAX_MATERIALS, ECS_INVALID_PARAMETER, NULL);
}

static
void draw_instances(
    SokolRenderer *canvas,
    SokolGeometry *geometry,
    sokol_instances_t *instances)
{
    if (!instances->instance_count) {
        return;
    }

    sg_bindings bind = {
        .vertex_buffers = {
            [0] = geometry->vertex_buffer,
            [1] = geometry->normal_buffer,
            [2] = instances->color_buffer,
            [3] = instances->material_buffer,
            [4] = instances->transform_buffer
        },
        .index_buffer = geometry->index_buffer,
        .fs_images[0] = canvas->shadow_pass.color_tex
    };

    sg_apply_bindings(&bind);
    sg_draw(0, geometry->index_count, instances->instance_count);
}

static
void SokolRender(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    SokolRenderer *sk_canvas = ecs_column(it, SokolRenderer, 1);
    EcsCanvas *canvas = ecs_column(it, EcsCanvas, 2);
    EcsQuery *q_buffers = ecs_column(it, EcsQuery, 3);
    EcsQuery *q_mats = ecs_column(it, EcsQuery, 4);
    ecs_entity_t ecs_entity(EcsCamera) = ecs_column_entity(it, 5);
    ecs_entity_t ecs_entity(EcsDirectionalLight) = ecs_column_entity(it, 6);

    ecs_query_t *buffers = q_buffers->query;
    vs_uniforms_t vs_u;
    fs_uniforms_t fs_u;
    vs_materials_t mat_u = {};

    /* Update material buffer when changed */
    bool materials_changed = false;
    if (ecs_query_changed(q_mats->query)) {
        init_materials(q_mats->query, &mat_u);
        materials_changed = true;
    }

    /* Loop each canvas */
    int32_t i;
    for (i = 0; i < it->count; i ++) {
        int w, h;
        SDL_GL_GetDrawableSize(sk_canvas[i].sdl_window, &w, &h);
        float aspect = (float)w / (float)h;

        /* Get light data (if set) */
        const EcsDirectionalLight *light_data = NULL;
        ecs_entity_t light = canvas[i].directional_light;
        if (light) {
            light_data = ecs_get(world, light, EcsDirectionalLight);
        }

        /* Render shadow map */
        sokol_run_shadow_pass(
            buffers, &sk_canvas[i].shadow_pass, light_data, vs_u.light_mat_vp);

        init_uniforms(world, &canvas[i], aspect, &vs_u, &fs_u,
            ecs_entity(EcsCamera), ecs_entity(EcsDirectionalLight));

        /* Render to offscreen texture so screen-space effects can be applied */
        sg_begin_pass(sk_canvas->offscreen_pass, &sk_canvas->pass_action);
        sg_apply_pipeline(sk_canvas->pip);

        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_u, sizeof(vs_uniforms_t));
        sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fs_u, sizeof(fs_uniforms_t));
        if (materials_changed) {
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 1, &mat_u, sizeof(vs_materials_t));
        }

        /* Loop buffers, render scene */
        ecs_iter_t qit = ecs_query_iter(buffers);
        while (ecs_query_next(&qit)) {
            SokolGeometry *geometry = ecs_column(&qit, SokolGeometry, 1);

            int b;
            for (b = 0; b < qit.count; b ++) {
                draw_instances(sk_canvas, &geometry[b], &geometry[b].solid);
                draw_instances(sk_canvas, &geometry[b], &geometry[b].emissive);
            }
        }
        sg_end_pass();

        /* Apply bloom effect */
        sg_image tex_fx = sokol_effect_run(
            sk_canvas, &sk_canvas->fx_bloom, sk_canvas->offscreen_tex);

        /* Render resulting offscreen texture to screen */
        sg_begin_default_pass(&sk_canvas->tex_pass_action, w, h);
        sg_apply_pipeline(sk_canvas->tex_pip);

        sg_bindings bind = {
            .vertex_buffers = { 
                [0] = sk_canvas->offscreen_quad 
            },
            .fs_images[0] = tex_fx
        };

        sg_apply_bindings(&bind);
        sg_draw(0, 6, 1);
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
    
    ECS_IMPORT(world, FlecsSystemsSdl2);
    ECS_IMPORT(world, FlecsComponentsGui);

    ECS_COMPONENT(world, SokolRenderer);
    ECS_COMPONENT(world, SokolMaterial);

    ECS_IMPORT(world, FlecsSystemsSokolGeometry);

    ECS_SYSTEM(world, SokolSetRenderer, EcsOnSet,
        flecs.systems.sdl2.window.Window,
        flecs.components.gui.Canvas,
        :Renderer,
        :Geometry,
        :Material);

    ECS_SYSTEM(world, SokolUnsetRenderer, EcsUnSet, 
        Renderer);

    ECS_SYSTEM(world, SokolRegisterMaterial, EcsPostLoad,
        [out] !flecs.systems.sokol.Material,
        [in]   flecs.components.graphics.Specular || 
               flecs.components.graphics.Emissive,
               ?Prefab);

    ECS_SYSTEM(world, SokolRender, EcsOnStore, 
        Renderer, 
        flecs.components.gui.Canvas, 
        flecs.system.Query FOR Geometry,
        flecs.system.Query FOR Material,
        :flecs.components.graphics.Camera,
        :flecs.components.graphics.DirectionalLight);      
}
