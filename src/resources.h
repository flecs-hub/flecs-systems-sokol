
#ifndef SOKOL_RESOURCES_H
#define SOKOL_RESOURCES_H

sg_image sokol_target_rgba8(
    int32_t width, 
    int32_t height);

sg_image sokol_target_rgba16f(
    int32_t width, 
    int32_t height);

sg_image sokol_target_depth(
    int32_t width, 
    int32_t height);

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
    ecs_rgb_t color);

const char* sokol_vs_passthrough(void);

#endif
