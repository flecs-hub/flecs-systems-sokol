/** @file Post process effects.
 */

#ifndef SOKOL_FX_H
#define SOKOL_FX_H

#define SOKOL_DEPTH_C 0.05
#define SOKOL_LOG_DEPTH

#define SOKOL_SHADER_FUNC_FLOAT_TO_RGBA \
    "vec4 float_to_rgba(const in float v) {\n" \
    "  vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;\n" \
    "  enc = fract(enc);\n" \
    "  enc -= enc.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);\n" \
    "  return enc;\n" \
    "}\n\n"

#define SOKOL_SHADER_FUNC_RGBA_TO_FLOAT \
    "float rgba_to_float(const in vec4 rgba) {\n" \
    "  return dot(rgba, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 160581375.0));\n" \
    "}\n\n"

#define SOKOL_SHADER_FUNC_POW2 \
    "float pow2(const in float v) {\n" \
    "  return v * v;\n" \
    "}\n\n"

#ifdef SOKOL_LOG_DEPTH
#define SOKOL_SHADER_FUNC_RGBA_TO_DEPTH__(C) \
    "float rgba_to_depth(vec4 rgba) {\n" \
    "  float d = rgba_to_float(rgba);\n" \
    "  d *= log("#C" * u_far + 1.0);\n" \
    "  d = exp(d);\n" \
    "  d -= 1.0;\n" \
    "  d /= "#C";\n" \
    "  return d;\n" \
    "}\n\n"
#else
#define SOKOL_SHADER_FUNC_RGBA_TO_DEPTH__(C) \
    "float rgba_to_depth(vec4 rgba) {\n" \
    "  float d = rgba_to_float(rgba);\n" \
    "  return d * u_far;\n" \
    "}\n\n"
#endif

#define SOKOL_SHADER_FUNC_RGBA_TO_DEPTH_(C) \
    SOKOL_SHADER_FUNC_RGBA_TO_DEPTH__(C)

#define SOKOL_SHADER_FUNC_RGBA_TO_DEPTH \
    SOKOL_SHADER_FUNC_RGBA_TO_DEPTH_(SOKOL_DEPTH_C)

#define SOKOL_SHADER_FUNC_RGBA_TO_DEPTH_LOG \
    "float rgba_to_depth_log(vec4 rgba) {\n" \
    "  return rgba_to_float(rgba);\n" \
    "}\n"

extern const char *shd_blur_hdr;
extern const char *shd_blur;

SokolFx sokol_init_hdr(
    int width, 
    int height);

SokolFx sokol_init_fog(
    int width,
    int height);

SokolFx sokol_init_ssao(
    int width, 
    int height);

SokolFx sokol_init_blend(
    int width, 
    int height);

void sokol_fog_set_params(
    SokolFx *fx,
    float r,
    float g,
    float b,
    float density,
    float uv_horizon);

sokol_fx_resources_t* sokol_init_fx(
    int width, 
    int height);

void sokol_update_fx(
    sokol_fx_resources_t *fx, 
    int width, 
    int height);

#endif
