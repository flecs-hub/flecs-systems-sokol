#include "private.h"

void sokol_get_near_far_rect(
    float aspect,
    float near, 
    float far,
    float fov,
    ecs_rect_t *r_near,
    ecs_rect_t *r_far)    
{
    float tfov = tan(glm_rad(fov));

    float near_width = near * tfov;
    r_near->width = near_width;
    r_near->height = near_width * aspect;

    float far_width = far * tfov;
    r_far->width = far_width;
    r_far->height = far_width * aspect;
}

static
void corner(vec3 center, ecs_rect_t r, vec3 v_offset, vec3 h_offset, vec3 v) {
    vec3 c, h;
    glm_vec3_copy(center, c);

    glm_vec3_add(c, h_offset, c);
    glm_vec3_add(c, v_offset, v);
}

void sokol_get_frustrum_verts(
    float near,
    float far,
    ecs_rect_t r_near,
    ecs_rect_t r_far,
    vec3 position,
    vec3 lookat,
    vec3 cam_up,
    vec3 *verts)
{
    /* Compute forward vector */
    vec3 f;
    glm_vec3_sub(lookat, position, f);
    glm_vec3_normalize(f);

    /* Get pitch and yaw from forward */
    float opp = sqrt(pow(f[0], 2) + pow(f[2], 2));
    float pitch = atan2(f[1], opp);
    float yaw = atan2(f[0], f[2]);
    
    /* Compute rotation matrix from pitch and yaw */
    mat4 rot;
    glm_rotate_make(rot, yaw, (vec3){0, 1, 0});
    glm_rotate(rot, -pitch, (vec3){1, 0, 0});

    /* Rotate up direction */
    vec3 up;
    glm_vec3_copy(cam_up, up);
    glm_mat4_mulv3(rot, up, 1, up);

    /* Compute right-vector (perpendicular to forward & up) */
    vec3 right;
    glm_vec3_cross(f, up, right);

    /* Multiply up vector with frustrum width & height */
    vec3 near_up, far_up;
    glm_vec3_scale(up, r_near.height, near_up);
    glm_vec3_scale(up, r_far.height, far_up);

    /* Multiply right vector with frustrum width & height */
    vec3 near_right, far_right;
    glm_vec3_scale(right, r_near.width, near_right);
    glm_vec3_scale(right, r_far.width, far_right);

    /* Compute bottom & left from right & top */
    vec3 near_bottom = {-near_up[0], -near_up[1], -near_up[2]};
    vec3 near_left = {-near_right[0], -near_right[1], -near_right[2]};

    /* Compute bottom & left from right & top */
    vec3 far_bottom = {-far_up[0], -far_up[1], -far_up[2]};
    vec3 far_left = {-far_right[0], -far_right[1], -far_right[2]};

    /* Compute near & far plane centers */
    vec3 c_near;
    glm_vec3_scale(f, near, c_near);

    vec3 c_far;
    glm_vec3_scale(f, far, c_far);

    /* Compute corner vertices of near & far planes */
    corner(c_near, r_near, near_left, near_up, verts[0]);
    corner(c_near, r_near, near_right, near_up, verts[1]);
    corner(c_near, r_near, near_right, near_bottom, verts[2]);
    corner(c_near, r_near, near_left, near_bottom, verts[3]);

    corner(c_far, r_far, far_left, far_up, verts[4]);
    corner(c_far, r_far, far_right, far_up, verts[5]);
    corner(c_far, r_far, far_right, far_bottom, verts[6]);
    corner(c_far, r_far, far_left, far_bottom, verts[7]);


    // printf("f = {%f, %f, %f}\n", f[0], f[1], f[2]);
    // printf("u = {%f, %f, %f}\n", up[0], up[1], up[2]);
    // printf("r = {%f, %f, %f}\n", near_right[0], near_right[1], near_right[2]);
    // printf("pitch = %f\n", pitch);
    // printf("yaw = %f {%f, %f}\n", yaw, cos(yaw), sin(yaw));

    // printf("nv'0 = {%f, %f, %f}\n", verts[0][0], verts[0][1], verts[0][2]);
    // printf("nv'1 = {%f, %f, %f}\n", verts[1][0], verts[1][1], verts[1][2]);
    // printf("nv'2 = {%f, %f, %f}\n", verts[2][0], verts[2][1], verts[2][2]);
    // printf("nv'3 = {%f, %f, %f}\n", verts[3][0], verts[3][1], verts[3][2]);

    // printf("fv'0 = {%f, %f, %f}\n", verts[4][0], verts[4][1], verts[4][2]);
    // printf("fv'1 = {%f, %f, %f}\n", verts[5][0], verts[5][1], verts[5][2]);
    // printf("fv'2 = {%f, %f, %f}\n", verts[6][0], verts[6][1], verts[6][2]);
    // printf("fv'3 = {%f, %f, %f}\n", verts[7][0], verts[7][1], verts[7][2]);
}
