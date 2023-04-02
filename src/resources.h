
#ifndef SOKOL_RESOURCES_H
#define SOKOL_RESOURCES_H

sg_image sokol_target_rgba8(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count);

sg_image sokol_target_rgba16(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count);

sg_image sokol_target_rgba16f(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count,
    int32_t num_mipmaps);

sg_image sokol_target_depth(
    int32_t width, 
    int32_t height,
    int32_t sample_count);

sg_image sokol_target(
    const char *label,
    int32_t width, 
    int32_t height,
    int32_t sample_count,
    int32_t num_mipmaps,
    sg_pixel_format format);

sg_image sokol_noise_texture(int32_t width, int32_t height);

sg_image sokol_bg_texture(ecs_rgb_t color, int32_t width, int32_t height);

sg_buffer sokol_buffer_quad(void);

sg_buffer sokol_buffer_box(void);

sg_buffer sokol_buffer_box_indices(void);

int32_t sokol_box_index_count(void);

sg_buffer sokol_buffer_box_normals(void);

sg_buffer sokol_buffer_rectangle(void);

sg_buffer sokol_buffer_rectangle_indices(void);

int32_t sokol_rectangle_index_count(void);

sg_buffer sokol_buffer_rectangle_normals(void);

sg_pass_action sokol_clear_action(
    ecs_rgb_t color,
    bool clear_color,
    bool clear_depth);

const char* sokol_vs_passthrough(void);

const char* sokol_vs_depth(void);

const char* sokol_fs_depth(void);

#endif
