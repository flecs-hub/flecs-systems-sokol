#include "private_api.h"

typedef struct scene_vs_uniforms_t {
    mat4 mat_vp;
    mat4 light_mat_vp;
} scene_vs_uniforms_t;

typedef struct scene_fs_uniforms_t {
    vec3 light_ambient;
    vec3 light_direction;
    vec3 light_color;
    vec3 eye_pos;
    float shadow_map_size;
} scene_fs_uniforms_t;

#define POSITION_I 0
#define NORMAL_I 1
#define COLOR_I 2
#define MATERIAL_I 3
#define TRANSFORM_I 4
#define LAYOUT_I_STR(i) #i
#define LAYOUT(loc) "layout(location=" LAYOUT_I_STR(loc) ") "

sg_pipeline init_scene_pipeline(int32_t sample_count) {
    /* create an instancing shader */
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.uniform_blocks = {
            [0] = {
                .size = sizeof(scene_vs_uniforms_t),
                .uniforms = {
                    [0] = { .name="u_mat_vp", .type=SG_UNIFORMTYPE_MAT4 },
                    [1] = { .name="u_light_vp", .type=SG_UNIFORMTYPE_MAT4 }
                },
            }
        },
        .fs = {
            .images = {
                [0] = {
                    .name = "shadow_map",
                    .image_type = SG_IMAGETYPE_2D
                }
            },
            .uniform_blocks = {
                [0] = {
                    .size = sizeof(scene_fs_uniforms_t),
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
            SOKOL_SHADER_HEADER
            "uniform mat4 u_mat_vp;\n"
            "uniform mat4 u_light_vp;\n"
            LAYOUT(POSITION_I)  "in vec3 v_position;\n"
            LAYOUT(NORMAL_I)    "in vec3 v_normal;\n"
            LAYOUT(COLOR_I)     "in vec4 i_color;\n"
            LAYOUT(MATERIAL_I)  "in vec3 i_material;\n"
            LAYOUT(TRANSFORM_I) "in mat4 i_mat_m;\n"
            "out vec4 position;\n"
            "out vec4 light_position;\n"
            "out vec3 normal;\n"
            "out vec4 color;\n"
            "out vec3 material;\n"
            "void main() {\n"
            "  vec4 pos4 = vec4(v_position, 1.0);\n"
            "  gl_Position = u_mat_vp * i_mat_m * pos4;\n"
            "  light_position = u_light_vp * i_mat_m * pos4;\n"
            "  position = (i_mat_m * pos4);\n"
            "  normal = (i_mat_m * vec4(v_normal, 0.0)).xyz;\n"
            "  color = i_color;\n"
            "  material = i_material;\n"
            "}\n",

        .fs.source =
            SOKOL_SHADER_HEADER
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
            "    depth += 0.0001;\n"
            "    return step(compare, depth);\n"
            "}\n"

            "float sampleShadowPCF(sampler2D shadowMap, vec2 uv, float texel_size, float compare) {\n"
            "    float result = 0.0;\n"
            "    for (int x = -pcf_count; x <= pcf_count; x++) {\n"
            "        for (int y = -pcf_count; y <= pcf_count; y++) {\n"
            "            result += sampleShadow(shadowMap, uv + vec2(x, y) * texel_size * texel_c, compare);\n"
            "        }\n"
            "    }\n"
            "    return result / float(pcf_samples);\n"
            "}\n"

            "void main() {\n"
            "  float specular_power = material.x;\n"
            "  float shininess = max(material.y, 1.0);\n"
            "  float emissive = material.z;\n"
            "  vec3 l = normalize(u_light_direction);\n"
            "  vec3 n = normalize(normal);\n"
            "  float n_dot_l = dot(n, l);\n"

            "  if (n_dot_l >= 0.0) {"
            "    vec3 v = normalize(u_eye_pos - position.xyz);\n"
            "    vec3 r = reflect(-l, n);\n"

            "    vec3 light_pos = light_position.xyz / light_position.w;\n"
            "    vec2 sm_uv = (light_pos.xy + 1.0) * 0.5;\n"
            "    float depth = light_position.z;\n"
            "    float texel_size = 1.0 / u_shadow_map_size;\n"
            "    float s = sampleShadowPCF(shadow_map, sm_uv, texel_size, depth);\n"
            "    s = max(s, emissive);\n"
            "    s = 1.0;\n"

            "    float r_dot_v = max(dot(r, v), 0.0);\n"
            "    float l_shiny = pow(r_dot_v * n_dot_l, shininess);\n"
            "    vec3 l_specular = vec3(specular_power * l_shiny * u_light_color);\n"
            "    vec3 l_diffuse = vec3(u_light_color) * n_dot_l;\n"
            "    vec3 l_light = (u_light_ambient + s * l_diffuse);\n"

            "    frag_color = vec4(max(vec3(emissive), l_light) * color.xyz + s * l_specular, 1.0);\n"
            "  } else {\n"
            "    vec3 light = emissive + clamp(1.0 - emissive, 0.0, 1.0) * (u_light_ambient);\n"
            "    frag_color = vec4(light * color.xyz, 1.0);\n"
            "  }\n"
            "}\n"
    });

    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
        .buffers = {
            [COLOR_I] =     { .stride = 16, .step_func=SG_VERTEXSTEP_PER_INSTANCE },
            [MATERIAL_I] =  { .stride = 12, .step_func=SG_VERTEXSTEP_PER_INSTANCE },
            [TRANSFORM_I] = { .stride = 64, .step_func=SG_VERTEXSTEP_PER_INSTANCE }
        },

            .attrs = {
                /* Static geometry */
                [POSITION_I] =      { .buffer_index=POSITION_I, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },
                [NORMAL_I] =        { .buffer_index=NORMAL_I, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT3 },

                /* Color buffer (per instance) */
                [COLOR_I] =         { .buffer_index=COLOR_I, .offset=0, .format=SG_VERTEXFORMAT_FLOAT4 },

                /* Material buffer (per instance) */
                [MATERIAL_I] =      { .buffer_index=MATERIAL_I, .offset=0, .format=SG_VERTEXFORMAT_FLOAT3 },

                /* Matrix (per instance) */
                [TRANSFORM_I] =     { .buffer_index=TRANSFORM_I, .offset=0,  .format=SG_VERTEXFORMAT_FLOAT4 },
                [TRANSFORM_I + 1] = { .buffer_index=TRANSFORM_I, .offset=16, .format=SG_VERTEXFORMAT_FLOAT4 },
                [TRANSFORM_I + 2] = { .buffer_index=TRANSFORM_I, .offset=32, .format=SG_VERTEXFORMAT_FLOAT4 },
                [TRANSFORM_I + 3] = { .buffer_index=TRANSFORM_I, .offset=48, .format=SG_VERTEXFORMAT_FLOAT4 }
            }
        },

        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = false
        },

        .colors = {{
            .pixel_format = SG_PIXELFORMAT_RGBA16F
        }},

        .cull_mode = SG_CULLMODE_BACK,

        .sample_count = sample_count
    });
}

sokol_offscreen_pass_t sokol_init_scene_pass(
    ecs_rgb_t background_color,
    int32_t w, 
    int32_t h,
    int32_t sample_count,
    sokol_offscreen_pass_t *depth_pass_out) 
{
    sg_image color_target = sokol_target_rgba16f( w, h, sample_count, 4);
    sg_image depth_target = sokol_target_depth(w, h, sample_count);

    ecs_trace("sokol: initialize scene pass");

    sg_pass pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = color_target,
        .depth_stencil_attachment.image = depth_target
    });

    *depth_pass_out = sokol_init_depth_pass(w, h, depth_target, sample_count);

    ecs_trace("sokol: initialize scene pipeline");

    return (sokol_offscreen_pass_t){
        .pass_action = sokol_clear_action(background_color, true, false),
        .pass = pass,
        .pip = init_scene_pipeline(sample_count),
        .color_target = color_target,
        .depth_target = depth_target
    };   
}

static
void scene_draw_instances(
    SokolGeometry *geometry,
    sokol_instances_t *instances,
    sg_image shadow_map)
{
    if (!instances->instance_count) {
        return;
    }

    sg_bindings bind = {
        .vertex_buffers = {
            [POSITION_I] =  geometry->vertex_buffer,
            [NORMAL_I] =    geometry->normal_buffer,
            [COLOR_I] =     instances->color_buffer,
            [MATERIAL_I] =  instances->material_buffer,
            [TRANSFORM_I] = instances->transform_buffer
        },
        .index_buffer = geometry->index_buffer,
        .fs_images[0] = shadow_map
    };

    sg_apply_bindings(&bind);
    sg_draw(0, geometry->index_count, instances->instance_count);
}

void sokol_run_scene_pass(
    sokol_offscreen_pass_t *pass,
    sokol_render_state_t *state)
{
    scene_vs_uniforms_t vs_u;
    glm_mat4_copy(state->uniforms.mat_vp, vs_u.mat_vp);
    glm_mat4_copy(state->uniforms.light_mat_vp, vs_u.light_mat_vp);
    
    scene_fs_uniforms_t fs_u;
    glm_vec3_copy(state->uniforms.light_ambient, fs_u.light_ambient);
    glm_vec3_copy(state->uniforms.light_direction, fs_u.light_direction);
    glm_vec3_copy(state->uniforms.light_color, fs_u.light_color);
    glm_vec3_copy(state->uniforms.eye_pos, fs_u.eye_pos);
    fs_u.shadow_map_size = state->uniforms.shadow_map_size;

    /* Render to offscreen texture so screen-space effects can be applied */
    sg_begin_pass(pass->pass, &pass->pass_action);
    sg_apply_pipeline(pass->pip);

    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){&vs_u, sizeof(scene_vs_uniforms_t)});
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &(sg_range){&fs_u, sizeof(scene_fs_uniforms_t)});

    /* Loop geometry, render scene */
    ecs_iter_t qit = ecs_query_iter(state->world, state->q_scene);
    while (ecs_query_next(&qit)) {
        SokolGeometry *geometry = ecs_term(&qit, SokolGeometry, 1);

        int b;
        for (b = 0; b < qit.count; b ++) {
            scene_draw_instances(&geometry[b], &geometry[b].solid, state->shadow_map);
            scene_draw_instances(&geometry[b], &geometry[b].emissive, state->shadow_map);
        }
    }
    sg_end_pass();
}

#undef POSITION_I
#undef NORMAL_I
#undef COLOR_I
#undef MATERIAL_I
#undef TRANSFORM_I
#undef LAYOUT
