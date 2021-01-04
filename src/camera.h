#ifndef SOKOL_CAMERA_H
#define SOKOL_CAMERA_H

void sokol_get_near_far_rect(
    float aspect,
    float near,
    float far, 
    float fov,
    ecs_rect_t *r_near,
    ecs_rect_t *r_far);

void sokol_get_frustrum_verts(
    float near,
    float far,
    ecs_rect_t r_near,
    ecs_rect_t r_far,
    vec3 position,
    vec3 lookat,
    vec3 up,
    vec3 *verts);

#endif
