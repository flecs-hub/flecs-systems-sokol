#include "../private_api.h"

// Based on https://threejs.org/examples/webgl_postprocessing_sao.html

static
const char *shd_ssao_header = 
    // Increase/decrease to trade quality for performance
    "#define NUM_SAMPLES 10\n"
    // The sample kernel uses a spiral pattern so most samples are concentrated
    // close to the center. 
    "#define NUM_RINGS 4\n"
    "#define KERNEL_RADIUS 70.0\n"
    // Intensity of the effect.
    "#define INTENSITY 1.0\n"
    // Max threshold prevents darkening objects that are far away
    "#define MAX_THRESHOLD 220.0\n"
    // Min threshold decreases halo's around objects
    "#define MIN_THRESHOLD 1.0001\n"
    // Misc params, tweaked to match the renderer
    "#define BIAS 0.2\n"
    "#define SCALE 1.0\n"
    // Derived constants
    "#define ANGLE_STEP ((PI2 * float(NUM_RINGS)) / float(NUM_SAMPLES))\n"
    "#define INV_NUM_SAMPLES (1.0 / float(NUM_SAMPLES))\n"
    "#define INV_NUM_SAMPLES (1.0 / float(NUM_SAMPLES))\n"

    SOKOL_SHADER_FUNC_FLOAT_TO_RGBA
    SOKOL_SHADER_FUNC_RGBA_TO_FLOAT
    SOKOL_SHADER_FUNC_RGBA_TO_DEPTH
    SOKOL_SHADER_FUNC_POW2

    "highp float rand( const in vec2 uv ) {\n"
    "    const highp float a = 12.9898, b = 78.233, c = 43758.5453;\n"
    "    highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );\n"
    "    return fract( sin( sn ) * c );\n"
    "}\n"

    "float getDepth(const in vec2 t_uv) {\n"
    "    return rgba_to_depth(texture(t_depth, t_uv));\n"
    "}\n"

    "float getViewZ(const in float depth) {\n"
    "    return (u_near * u_far) / (depth - u_far);\n"
    "}\n"

    // Compute position in world space from depth & projection matrix
    "vec3 getViewPosition( const in vec2 screenPosition, const in float depth, const in float viewZ ) {\n"
    "    float clipW = u_mat_p[2][3] * viewZ + u_mat_p[3][3];\n"
    "    vec4 clipPosition = vec4( ( vec3( screenPosition, depth ) - 0.5 ) * 2.0, 1.0 );\n"
    "    clipPosition *= clipW; // unprojection.\n"
    "    return ( u_inv_mat_p * clipPosition ).xyz;\n"
    "}\n"

    // Compute normal from derived position. Should at some point replace it
    // with reading from a normal buffer so it works correctly with smooth
    // shading / normal maps.
    "vec3 getViewNormal( const in vec3 viewPosition, const in vec2 t_uv ) {\n"
    "    return normalize( cross( dFdx( viewPosition ), dFdy( viewPosition ) ) );\n"
    "}\n"

    "float scaleDividedByCameraFar;\n"

    // Compute occlusion of single sample
    "float getOcclusion( const in vec3 centerViewPosition, const in vec3 centerViewNormal, const in vec3 sampleViewPosition ) {\n"
    "    vec3 viewDelta = sampleViewPosition - centerViewPosition;\n"
    "    float viewDistance = length( viewDelta );\n"
    "    float scaledScreenDistance = scaleDividedByCameraFar * viewDistance;\n"
    "    float n_dot_d = dot(centerViewNormal, viewDelta);\n"
    "    float scaled_n_dot_d = max(0.0, n_dot_d / scaledScreenDistance - BIAS);\n"
    "    float result = scaled_n_dot_d / (1.0 + pow2(scaledScreenDistance));\n"
    "    if (result > MAX_THRESHOLD) {\n"
    "      result = 0.0;\n"
    "    }\n"
    "    return clamp(result, MIN_THRESHOLD, 2.0);\n"
    "}\n"

    "float getAmbientOcclusion( const in vec3 centerViewPosition, float centerDepth ) {\n"
    "  scaleDividedByCameraFar = SCALE / u_far;\n"
    "  vec3 centerViewNormal = getViewNormal( centerViewPosition, uv );\n"
    
    "  float angle = rand( uv ) * PI2;\n"
    "  vec2 radius = vec2( KERNEL_RADIUS * INV_NUM_SAMPLES );\n"

    // Use smaller kernels for objects farther away from the camera
    "  radius /= u_target_size * centerDepth * 0.05;\n"
    // Make sure tha the sample radius isn't less than a single texel, as this
    // introduces noise
    "  radius = max(radius, 5.0 / u_target_size);\n"
    
    "  vec2 radiusStep = radius;\n"
    "  float occlusionSum = 0.0;\n"

    // Collect occlusion samples
    "  for( int i = 0; i < NUM_SAMPLES; i ++ ) {\n"
    "    vec2 sampleUv = uv + vec2( cos( angle ), sin( angle ) ) * radius;\n"

    //   Don't sample outside of texture coords to prevent edge artifacts
    "    sampleUv = clamp(sampleUv, EPSILON, 1.0 - EPSILON);\n"

    "    radius += radiusStep;\n"
    "    angle += ANGLE_STEP;\n"

    "    float sampleDepth = getDepth( sampleUv );\n"
    "    float sampleDepthNorm = sampleDepth / u_far;\n"

    "    float sampleViewZ = getViewZ( sampleDepth );\n"
    "    vec3 sampleViewPosition = getViewPosition( sampleUv, sampleDepthNorm, sampleViewZ );\n"
    "    occlusionSum += getOcclusion( centerViewPosition, centerViewNormal, sampleViewPosition );\n"
    "  }\n"

    "  return occlusionSum * (float(INTENSITY) / (float(NUM_SAMPLES)));\n"
    "}\n"
    ;

static
const char *shd_ssao =
    // Depth (range = 0 .. u_far)
    "float centerDepth = getDepth( uv );\n"
    // Depth (range = 0 .. 1)
    "float centerDepthNorm = centerDepth / u_far;\n"

    "float centerViewZ = getViewZ( centerDepth );\n"
    "vec3 viewPosition = getViewPosition( uv, centerDepthNorm, centerViewZ );\n"
    "float ambientOcclusion = getAmbientOcclusion( viewPosition, centerDepth );\n"

    // Store value as rgba to increase precision
    "frag_color = float_to_rgba(ambientOcclusion);\n"
    ;

static
const char *shd_blend_mult_header = 
    SOKOL_SHADER_FUNC_RGBA_TO_FLOAT
    ;

static
const char *shd_blend_mult =
    "float ambientOcclusion = rgba_to_float(texture(t_occlusion, uv));\n"
    "frag_color = (1.0 - ambientOcclusion) * texture(t_scene, uv);\n"
    ;

#define SSAO_INPUT_SCENE SOKOL_FX_INPUT(0)
#define SSAO_INPUT_DEPTH SOKOL_FX_INPUT(1)

SokolFx sokol_init_ssao(
    int width, int height)
{
    ecs_trace("sokol: initialize fog effect");

    SokolFx fx = {0};
    fx.name = "SSAO";

    // Ambient occlusion shader 
    int32_t ao = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "ssao",
        .outputs = {{width, height}},
        .shader_header = shd_ssao_header,
        .shader = shd_ssao,
        .color_format = SG_PIXELFORMAT_RGBA8,
        .inputs = { "t_depth" },
        .steps = {
            [0] = {
                .inputs = { {SSAO_INPUT_DEPTH} }
            }
        }
    });

    // Blur to reduce the noise, so we can keep sample count low
    int blur = sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "blur",
        .outputs = {{512}},
        .shader_header = shd_blur_hdr,
        .shader = shd_blur,
        .color_format = SG_PIXELFORMAT_RGBA8,
        .inputs = { "tex" },
        .params = { "horizontal" },
        .steps = {
            [0] = { 
                .name = "ssao hblur",
                .inputs = { {SOKOL_FX_PASS(ao)} },
                .params = { 1.0 },
                .loop_count = 2
            },
            [1] = { 
                .name = "ssao vblur",
                .inputs = { { -1 } },
                .params = { 0.0 }
            }
        }
    });

    // Multiply ambient occlusion with the scene
    sokol_fx_add_pass(&fx, &(sokol_fx_pass_desc_t){
        .name = "blend",
        .outputs = {{width, height}},
        .shader_header = shd_blend_mult_header,
        .shader = shd_blend_mult,
        .color_format = SG_PIXELFORMAT_RGBA16F,
        .inputs = { "t_scene", "t_occlusion" },
        .steps = {
            [0] = {
                .inputs = { {SSAO_INPUT_SCENE}, {SOKOL_FX_PASS(blur)} }
            }
        }
    });

    return fx;
}
