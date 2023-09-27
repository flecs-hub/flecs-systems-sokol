#include "private_api.h"

static
float quad_vertices_uvs[] = {
    -1.0f, -1.0f,  0.0f,   0, 0,
     1.0f, -1.0f,  0.0f,   1, 0,
     1.0f,  1.0f,  0.0f,   1, 1,

    -1.0f, -1.0f,  0.0f,   0, 0,
     1.0f,  1.0f,  0.0f,   1, 1,
    -1.0f,  1.0f,  0.0f,   0, 1
};

static
vec3 rectangle_vertices[] = {
    {-0.5,  0.5, 0.0},
    { 0.5,  0.5, 0.0},
    { 0.5, -0.5, 0.0},
    {-0.5, -0.5, 0.0},
};

static
uint16_t rectangle_indices[] = {
    0, 1, 2,
    0, 2, 3
};

static
vec3 box_vertices[] = {
    {-0.5f,  0.5f, -0.5f},  
    { 0.5f,  0.5f, -0.5f},    
    { 0.5f, -0.5f, -0.5f},    
    {-0.5f, -0.5f, -0.5f}, // Back   

    {-0.5f,  0.5f,  0.5f}, 
    { 0.5f,  0.5f,  0.5f},    
    { 0.5f, -0.5f,  0.5f},    
    {-0.5f, -0.5f,  0.5f}, // Front  

    {-0.5f, -0.5f,  0.5f},    
    {-0.5f,  0.5f,  0.5f},    
    {-0.5f,  0.5f, -0.5f},    
    {-0.5f, -0.5f, -0.5f}, // Left   

    { 0.5f, -0.5f,  0.5f},    
    { 0.5f,  0.5f,  0.5f},    
    { 0.5f,  0.5f, -0.5f},    
    { 0.5f, -0.5f, -0.5f}, // Right   

    { 0.5f, -0.5f, -0.5f},    
    { 0.5f, -0.5f,  0.5f},    
    {-0.5f, -0.5f,  0.5f},    
    {-0.5f, -0.5f, -0.5f}, // Bottom   

    { 0.5f,  0.5f, -0.5f},    
    { 0.5f,  0.5f,  0.5f},    
    {-0.5f,  0.5f,  0.5f},    
    {-0.5f,  0.5f, -0.5f}, // Top   
};

static
uint16_t box_indices[] = {
    0,  1,  2,   0,  2,  3,
    6,  5,  4,   7,  6,  4,
    8,  9,  10,  8,  10, 11,
    14, 13, 12,  15, 14, 12,
    16, 17, 18,  16, 18, 19,
    22, 21, 20,  23, 22, 20,
};

#define EPSILON 0.0000001

static
void compute_flat_normals(
    vec3 *vertices,
    uint16_t *indices,
    int32_t count,
    vec3 *normals_out)
{
    int32_t v;
    for (v = 0; v < count; v += 3) {
        vec3 vec1, vec2, normal;
        glm_vec3_sub(vertices[indices[v + 0]], vertices[indices[v + 1]], vec1);
        glm_vec3_sub(vertices[indices[v + 0]], vertices[indices[v + 2]], vec2);
        glm_vec3_crossn(vec2, vec1, normal);

        if (fabs(normal[0]) < GLM_FLT_EPSILON) {
            normal[0] = 0;
        }
        if (fabs(normal[1]) < GLM_FLT_EPSILON) {
            normal[1] = 0;
        }
        if (fabs(normal[2]) < GLM_FLT_EPSILON) {
            normal[2] = 0;
        }

        glm_vec3_copy(normal, normals_out[indices[v + 0]]);
        glm_vec3_copy(normal, normals_out[indices[v + 1]]);
        glm_vec3_copy(normal, normals_out[indices[v + 2]]);
    }
}

sg_image sokol_target(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count,
    int32_t num_mipmaps,
    sg_pixel_format format)
{
    sg_image_desc img_desc;
    
    if (num_mipmaps > 1) {
        img_desc = (sg_image_desc){
            .render_target = true,
            .width = width,
            .height = height,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .pixel_format = format,
            .min_filter = SG_FILTER_LINEAR_MIPMAP_LINEAR,
            .mag_filter = SG_FILTER_LINEAR,
            .sample_count = sample_count,
            .num_mipmaps = num_mipmaps,
            .label = label
        };
    } else {
        img_desc = (sg_image_desc){
            .render_target = true,
            .width = width,
            .height = height,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .pixel_format = format,
            .min_filter = SG_FILTER_LINEAR,
            .mag_filter = SG_FILTER_LINEAR,
            .sample_count = sample_count,
            .num_mipmaps = num_mipmaps,
            .label = label
        };
    }

    return sg_make_image(&img_desc);
}

sg_image sokol_target_rgba8(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count)
{
    return sokol_target(label, width, height, sample_count, 1, SG_PIXELFORMAT_RGBA8);
}

sg_image sokol_target_rgba16(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count)
{
    return sokol_target(label, width, height, sample_count, 1, SG_PIXELFORMAT_RGBA16);
}

sg_image sokol_target_rgba16f(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count,
    int32_t num_mipmaps) 
{
    return sokol_target(label, width, height, sample_count, num_mipmaps, SG_PIXELFORMAT_RGBA16F);
}

sg_image sokol_target_depth(
    int32_t width, 
    int32_t height,
    int32_t sample_count) 
{
    sg_image_desc img_desc = {
        .render_target = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .sample_count = sample_count,
        .label = "Depth target"
    };

    return sg_make_image(&img_desc);
}

sg_buffer sokol_buffer_quad(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .data = { quad_vertices_uvs, sizeof(quad_vertices_uvs) },
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_box(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .data = { box_vertices, sizeof(box_vertices) },
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_box_indices(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .data = { box_indices, sizeof(box_indices) },
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE
    });
}

int32_t sokol_box_index_count(void)
{
    return 36;
}

sg_buffer sokol_buffer_box_normals(void)
{
    vec3 normals[24];
    compute_flat_normals(
        box_vertices, box_indices, sokol_box_index_count(), normals);

    return sg_make_buffer(&(sg_buffer_desc){
        .data = { normals, sizeof(normals) },
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_rectangle(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .data = { rectangle_vertices, sizeof(rectangle_vertices) },
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_rectangle_indices(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .data = { rectangle_indices, sizeof(rectangle_indices) },
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE
    });
}

int32_t sokol_rectangle_index_count(void)
{
    return 6;
}

sg_buffer sokol_buffer_rectangle_normals(void)
{
    vec3 normals[4];
    compute_flat_normals(rectangle_vertices, 
        rectangle_indices, sokol_rectangle_index_count(), normals);

    return sg_make_buffer(&(sg_buffer_desc){
        .data = { normals, sizeof(normals) },
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_pass_action sokol_clear_action(
    ecs_rgb_t color,
    bool clear_color,
    bool clear_depth)
{
    sg_pass_action action = {0};
    if (clear_color) {
        action.colors[0] = (sg_color_attachment_action){
            .action = SG_ACTION_CLEAR,
            .value = {color.r, color.g, color.b, 1.0f}
        };
    } else {
        action.colors[0].action = SG_ACTION_DONTCARE;
    }

    if (clear_depth) {
        action.depth.action = SG_ACTION_CLEAR;
        action.depth.value = 1.0;
    } else {
        action.depth.action = SG_ACTION_DONTCARE;
        action.stencil.action = SG_ACTION_DONTCARE;
    }

    return action;
}

const char* sokol_vs_passthrough(void)
{
    return  SOKOL_SHADER_HEADER
            "layout(location=0) in vec4 v_position;\n"
            "layout(location=1) in vec2 v_uv;\n"
            "out vec2 uv;\n"
            "void main() {\n"
            "  gl_Position = v_position;\n"
            "  uv = v_uv;\n"
            "}\n";
}

sg_image sokol_noise_texture(
    int32_t width, 
    int32_t height)
{
    uint32_t *data = ecs_os_malloc_n(uint32_t, width * height);
    for (int32_t x = 0; x < width; x ++) {
        for (int32_t y = 0; y < height; y ++) {
            data[x + y * x] = rand();
        }
    }

    sg_image img = sg_make_image(&(sg_image_desc){
        .width = width,
        .height = height,
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT,
        .pixel_format = SG_PIXELFORMAT_R8,
        .label = "Noise texture",
        .data.subimage[0][0] = {
            .ptr = data,
            .size = width * height * sizeof(char)
        }
    });

    ecs_os_free(data);

    return img;
}

sg_image sokol_bg_texture(ecs_rgb_t color, int32_t width, int32_t height)
{
    uint32_t *data = ecs_os_malloc_n(uint32_t, width * height);

    for (int32_t x = 0; x < width; x ++) {
        for (int32_t y = 0; y < height; y ++) {
            uint32_t c = (uint32_t)(color.r * 256);
            c += (uint32_t)(color.g * 256) << 8;
            c += (uint32_t)(color.b * 256) << 16;
            c += 255u << 24;

            data[x + y * width] = c;
        }
    }

    sg_image img = sg_make_image(&(sg_image_desc){
        .width = width,
        .height = height,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .label = "Background texture",
        .data.subimage[0][0] = {
            .ptr = data,
            .size = width * height * 4
        }
    });

    return img;
}
