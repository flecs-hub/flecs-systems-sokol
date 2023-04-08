#define LOG2 1.442695

const float transition = 0.025;
const float inv_transition = 1.0 / transition;
const float sample_height = 0.01;

vec4 c = texture(hdr, uv);
float d = rgba_to_depth(texture(depth, uv));
vec3 worldPos = world_pos_from_depth(uv, d, u_inv_mat_vp);
float intensity;

if (d > 1.0) {
    intensity = (max((u_horizon + transition) - uv.y, 0.0) * inv_transition);
    intensity = min(intensity, 1.0);
} else {
    vec3 eye = u_eye_pos;
    eye.x = -eye.x;

    vec3 delta = worldPos - eye;
    intensity = fogIntensity(eye, normalize(delta), length(delta));
    intensity = clamp(intensity, 0.0, 1.0);
}

vec4 fog_color = texture(atmos, vec2(uv.x, u_horizon + sample_height));
frag_color = mix(c, fog_color, intensity);
