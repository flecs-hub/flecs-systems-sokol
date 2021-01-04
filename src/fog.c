#include "private.h"

 
//   #define LOG2 1.442695
 
//   float fogDistance = length(v_position);
// //   float fogAmount = smoothstep(u_fogNear, u_fogFar, fogDistance);
//   float fogAmount = 1. - exp2(-u_fogDensity * u_fogDensity * fogDistance * fogDistance * LOG2);
//   fogAmount = clamp(fogAmount, 0., 1.);
 
//   gl_FragColor = mix(color, u_fogColor, fogAmount);  

static
const char *shd_fog_hdr =
    "float decodeDepth(vec4 rgba) {\n"
    "    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));\n"
    "}\n"
    "#define LOG2 1.442695\n";

static
const char *shd_fog =
    "float fogDensity = 0.12;\n"
    "vec4 fogColor = vec4(0.5, 0.5, 1.0, 1.0);\n"
    "float fogDistance = decodeDepth(texture(depth, uv));\n"
    "float fogAmount = 1.0 - exp2(-fogDensity * fogDensity * fogDistance * fogDistance * LOG2);\n"
    "fogAmount = clamp(fogAmount, 0.0, 1.0);\n"
    "frag_color = mix(texture(tex, uv), fogColor, fogAmount);\n";

SokolEffect sokol_init_fog(
    int width, int height)
{
    SokolEffect fx = sokol_effect_init(2);

    sokol_effect_add_pass(&fx, (sokol_fx_pass_desc_t){
        .width = width, 
        .height = height, 
        .shader = shd_fog,
        .shader_header = shd_fog_hdr,
        .inputs = {
            [0] = { .name = "tex", .id = 0 },
            [1] = { .name = "depth", .id = 1 }
        }
    });

    return fx;
}
