#include "etc/sokol/shaders/common.glsl"
#include "etc/sokol/shaders/atmosphere.glsl"

float fogIntensity(vec3 origin, vec3 direction, float dist) {
    return (u_density / u_falloff) * exp(-origin.y * u_falloff) * (1.0 - exp(-dist * direction.y * u_falloff)) / direction.y;
}
