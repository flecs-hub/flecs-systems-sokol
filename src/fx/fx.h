/** @file Post process effects.
 */

#ifndef SOKOL_FX_H
#define SOKOL_FX_H

#define SOKOL_ENCODE_FLOAT_RGB \
    "vec4 float_to_rgb(float v) {\n" \
    "    vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;\n" \
    "    enc = fract(enc);\n" \
    "    enc -= enc.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);\n" \
    "    return enc;\n" \
    "}\n"

#define SOKOL_DECODE_FLOAT_RGB \
    "float rgb_to_float(vec4 rgba) {\n" \
    "    return dot(rgba, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 160581375.0));\n" \
    "}\n"

SokolFx sokol_init_hdr(
    int width, int height);

SokolFx sokol_init_fog(
    int width, int height);

sokol_fx_resources_t* sokol_init_fx(
    int width, int height);

#endif
