#include "etc/sokol/shaders/constants.glsl"

vec4 float_to_rgba(const in float v) {
  vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
  enc = fract(enc);
  enc -= enc.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);
  return enc;
}

float rgba_to_float(const in vec4 rgba) {
  return dot(rgba, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 160581375.0));
}

float pow2(const in float v) {
  return v * v;
}

float rgba_to_depth(vec4 rgba) {
  return rgba_to_float(rgba);
}

vec4 depth_to_rgba(vec3 pos) {
    return float_to_rgba(gl_FragCoord.z);
}

float rgba_to_depth_log(vec4 rgba) {
  return rgba_to_float(rgba);
}

highp float rand( const in vec2 uv ) {
    const highp float a = 12.9898, b = 78.233, c = 43758.5453;
    highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
    return fract( sin( sn ) * c );
}

float getViewZ(const in float near, const in float far, const in float depth) {
    return (near * far) / (depth - far);
}

// Compute position in world space from depth & projection matrix
vec3 getViewPosition( const in vec2 screenPosition, const in float depth, const in float viewZ, const in mat4 mat_p, const in mat4 inv_mat_p ) {
    float clipW = mat_p[2][3] * viewZ + mat_p[3][3];
    vec4 clipPosition = vec4( ( vec3( screenPosition, depth ) - 0.5 ) * 2.0, 1.0 );
    clipPosition *= clipW; // unprojection.
    return ( inv_mat_p * clipPosition ).xyz;
}

vec3 world_pos_from_depth(vec2 uv, float depth, mat4 inv_mat_vp)
{
    vec4 clip_space_position = vec4(uv * 2.0 - vec2(1.0), 2.0 * depth - 1.0, 1.0);
    vec4 position = inv_mat_vp * clip_space_position;
    return(position.xyz / position.w);
}