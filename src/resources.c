#include "private.h"

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
    {-0.5, -0.5, 0.0},
    { 0.5, -0.5, 0.0},
    { 0.5,  0.5, 0.0},
    {-0.5,  0.5, 0.0}
};

static
uint16_t rectangle_indices[] = {
    0, 1, 2,
    0, 2, 3
};

static
vec3 box_vertices[] = {
    {-0.5f, -0.5f, -0.5f}, // Back   
    { 0.5f, -0.5f, -0.5f},    
    { 0.5f,  0.5f, -0.5f},    
    {-0.5f,  0.5f, -0.5f},  

    {-0.5f, -0.5f,  0.5f}, // Front  
    { 0.5f, -0.5f,  0.5f},    
    { 0.5f,  0.5f,  0.5f},    
    {-0.5f,  0.5f,  0.5f}, 

    {-0.5f, -0.5f, -0.5f}, // Left   
    {-0.5f,  0.5f, -0.5f},    
    {-0.5f,  0.5f,  0.5f},    
    {-0.5f, -0.5f,  0.5f},    

    { 0.5f, -0.5f, -0.5f}, // Right   
    { 0.5f,  0.5f, -0.5f},    
    { 0.5f,  0.5f,  0.5f},    
    { 0.5f, -0.5f,  0.5f},    

    {-0.5f, -0.5f, -0.5f}, // Bottom   
    {-0.5f, -0.5f,  0.5f},    
    { 0.5f, -0.5f,  0.5f},    
    { 0.5f, -0.5f, -0.5f},    

    {-0.5f,  0.5f, -0.5f}, // Top   
    {-0.5f,  0.5f,  0.5f},    
    { 0.5f,  0.5f,  0.5f},    
    { 0.5f,  0.5f, -0.5f},    
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
        
        glm_vec3_copy(normal, normals_out[indices[v + 0]]);
        glm_vec3_copy(normal, normals_out[indices[v + 1]]);
        glm_vec3_copy(normal, normals_out[indices[v + 2]]);
    }
}

sg_image sokol_target_rgba8(
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

sg_image sokol_target_rgba16f(
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

sg_image sokol_target_depth(
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

sg_buffer sokol_buffer_quad(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices_uvs),
        .content = quad_vertices_uvs,
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_box(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(box_vertices),
        .content = box_vertices,
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_box_indices(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(box_indices),
        .content = box_indices,
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
        .size = sizeof(normals),
        .content = normals,
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_rectangle(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(rectangle_vertices),
        .content = rectangle_vertices,
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_buffer sokol_buffer_rectangle_indices(void)
{
    return sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(rectangle_indices),
        .content = rectangle_indices,
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
        .size = sizeof(normals),
        .content = normals,
        .usage = SG_USAGE_IMMUTABLE
    });
}

sg_pass_action sokol_clear_action(
    ecs_rgb_t color)
{
    return (sg_pass_action) {
        .colors[0] = {
            .action = SG_ACTION_CLEAR, 
            .val = { color.r, color.g, color.b, 1.0f }
        } 
    };
}

const char* sokol_vs_passthrough(void)
{
    return  "#version 330\n"
            "layout(location=0) in vec4 v_position;\n"
            "layout(location=1) in vec2 v_uv;\n"
            "out vec2 uv;\n"
            "void main() {\n"
            "  gl_Position = v_position;\n"
            "  uv = v_uv;\n"
            "}\n";
}
