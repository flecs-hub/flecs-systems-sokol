#include "private_include.h"

static
const char *shd_threshold =
    "float thresh = 1.0;\n"
    "vec4 c = texture(tex, uv);\n"
    "c.r = max(c.r - thresh, 0);\n"
    "c.g = max(c.g - thresh, 0);\n"
    "c.b = max(c.b - thresh, 0);\n"
    "frag_color = c;\n";

static
const char *shd_h_blur =
    "float kernel = 50.0;\n"
    "vec4 sum = vec4(0.0);\n"
    "float width = 800;\n"
    "float height = 600;\n"
    "float x = uv.x;\n"
    "float y = uv.y;\n"
    "float i, g;\n"

    "kernel = kernel / width;\n"
    "kernel = kernel / (width / height);\n"
    "for(i=-kernel; i<=kernel; i+=1 / width) {\n"
	"	g = i / kernel;\n"
	"	g *= g;\n"
	"	sum += texture(tex, vec2(x + i, y)) * exp(-(g) * 5);\n"
	"}\n"
    "frag_color = sum / 20;\n";

static
const char *shd_v_blur =
    "float kernel = 50.0;\n"
    "vec4 sum = vec4(0.0);\n"
    "float width = 800;\n"
    "float height = 600;\n"
    "float x = uv.x;\n"
    "float y = uv.y;\n"
    "float i, g;\n"

    "kernel = kernel / width;\n"
    "for(i=-kernel; i<=kernel; i+=1 / width) {\n"
	"	g = i / kernel;\n"
	"	g *= g;\n"
	"	sum += texture(tex, vec2(x, y + i)) * exp(-(g) * 5);\n"
	"}\n"
    "frag_color = sum / 20;\n";  

static
const char *shd_blend =
    "frag_color = texture(tex0, uv) + texture(tex1, uv);\n";

SokolEffect sokol_init_bloom(
    int width, int height)
{
    SokolEffect fx = {0};
    int blur_w = width * 0.5;
    int blur_h = height * 0.5;

    int threshold_pass = sokol_effect_add_pass(&fx, (sokol_pass_desc_t){
        .width = blur_w, 
        .height = blur_h, 
        .shader = shd_threshold,
        .inputs = {
            [0] = { .name = "tex", .id = 0 }
        }
    });

    int blur_h_pass = sokol_effect_add_pass(&fx, (sokol_pass_desc_t){
        .width = blur_w, 
        .height = blur_h, 
        .shader = shd_h_blur,
        .inputs = {
            [0] = { .name = "tex", .id = threshold_pass }
        }
    });

    int blur_v_pass = sokol_effect_add_pass(&fx, (sokol_pass_desc_t){
        .width = blur_w, 
        .height = blur_h, 
        .shader = shd_v_blur,
        .inputs = {
            [0] = { .name = "tex", .id = blur_h_pass }
        }
    });

    sokol_effect_add_pass(&fx, (sokol_pass_desc_t){
        .width = width, 
        .height = height, 
        .shader = shd_blend,
        .inputs = {
            [0] = { .name = "tex0", .id = 0 },
            [1] = { .name = "tex1", .id = blur_v_pass }
        }
    });

    return fx;
}
