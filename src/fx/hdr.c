#include "../private_api.h"

// ITU BT.709: vec3(0.2126, 0.7152, 0.0722)
// ITU BT.601: vec3(0.299, 0.587, 0.114)

static
const char *shd_threshold =
    "vec4 c = texture(hdr, uv, mipmap);\n"
    "float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "float l_norm = l / threshold;\n"
    "float p = min(l_norm * l_norm, l_norm);\n"
    "frag_color = c * p;\n";

const char *shd_blur_hdr =
    "const float gauss[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n";

const char *shd_blur =
    "float offset = 0.0;\n"
    "vec4 result = gauss[0] * texture(tex, vec2(uv.x, uv.y));\n"
    "ivec2 ts = textureSize(tex, 0);\n"
    "float px = 1.0 / float(ts.x);\n"
    "float py = 1.0 / float(ts.y);\n"
    "if (horizontal == 0.0) {\n"
    "   for (int i = 1; i < 5; i ++) {\n"
    "     offset += px / u_aspect;\n"
    "     result += gauss[i] * texture(tex, vec2(uv.x + offset, uv.y));\n"
    "     result += gauss[i] * texture(tex, vec2(uv.x - offset, uv.y));\n"
    "   }\n"
    "} else {\n"
    "   for (int i = 1; i < 5; i ++) {\n"
    "     offset += py;\n"
    "     result += gauss[i] * texture(tex, vec2(uv.x, uv.y + offset));\n"
    "     result += gauss[i] * texture(tex, vec2(uv.x, uv.y - offset));\n"
    "   }\n"
    "}\n"
    "frag_color = result;\n";

static
const char *shd_blend =
    "frag_color = max(texture(dst, uv) * blend_dst, texture(src, uv) * blend_src);\n";

static
const char *shd_hdr =
    "vec3 c = texture(hdr, uv).rgb;\n"
    "vec3 b = texture(bloom, uv).rgb;\n"
    "float b_clip = dot(vec3(0.333), b);\n"
    "b = b + pow(b_clip, 3.0);\n"
    "c = c + b;\n"
    "c = pow(c, vec3(1.0 / gamma));\n"
    "frag_color = vec4(c, 1.0);\n"
    ;

#define FX_INPUT_HDR (0)
#define FX_INPUT_LUM (1)

SokolFx sokol_init_hdr(
    int width, int height)
{
    ecs_trace("sokol: initialize hdr effect");

    SokolFx fx = {0};
    fx.name = "Hdr";
    fx.width = width;
    fx.height = height;

    int threshold = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "threshold",
        .outputs = {{64}, {256}},
        .shader = shd_threshold,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "hdr" },
        .params = { "threshold", "mipmap" },
        .steps = {
            [0] = {
                .name = "halo",
                .params = { 4.0, 1.0 },
                .inputs = { {FX_INPUT_HDR} }
            },
            [1] = {
                .name = "glow",
                .params = { 3.0 },
                .inputs = { {FX_INPUT_HDR} },
                .output = 1
            }
        }
    });

    int blur = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "blur",
        .outputs = {{64}, {256}},
        .shader_header = shd_blur_hdr,
        .shader = shd_blur,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "tex" },
        .params = { "horizontal" },
        .steps = {
            /* halo */
            [0] = { 
                .name = "halo hblur",
                .inputs = { {SOKOL_FX_PASS(threshold), 0} },
                .params = { 1.0 },
                .loop_count = 4
            },
            [1] = { 
                .name = "halo vblur",
                .inputs = { { -1 } },
                .params = { 0.0 }
            },
            
            /* glow */
            [2] = { 
                .name = "glow hblur",
                .inputs = { {SOKOL_FX_PASS(threshold), 1} },
                .params = { 1.0 },
                .loop_count = 2,
                .output = 1,
            },
            [3] = { 
                .name = "glow vblur",
                .inputs = { { -1 } },
                .params = { 0.0 }
            }
        }
    });

    int bloom = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "bloom",
        .outputs = {{256}},
        .shader = shd_blend,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "dst", "src" },
        .params = { "blend_dst", "blend_src" },
        .steps = {
            [0] = {
                .name = "halo + glow",
                .inputs = { {SOKOL_FX_PASS(blur), 0}, {SOKOL_FX_PASS(blur), 1} }, 
                .params = { 1.0, 1.0 }
            }
        }
    });

    sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "hdr",
        .outputs = {{ .global_size = true }},
        .shader = shd_hdr,
        .inputs = { "hdr", "bloom" },
        .params = { "exposure", "gamma" },
        .steps = {
            [0] = {
                .inputs = { {SOKOL_FX_INPUT(0)}, {SOKOL_FX_PASS(bloom), 0} },
                .params = { 1.0, 2.2 }
            }
        }
    });

    return fx;
}
