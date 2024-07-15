
/* Code is used as an importable module, so apps must provide their own main */
#define SOKOL_NO_ENTRY

/* Select graphics API implementation */
#ifdef __EMSCRIPTEN__
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE33
#endif

#ifndef __EMSCRIPTEN__
#define SOKOL_SHADER_HEADER SOKOL_SHADER_VERSION SOKOL_SHADER_PRECISION
#define SOKOL_SHADER_VERSION "#version 330\n"
#define SOKOL_SHADER_PRECISION "precision highp float;\n"
#else
#define SOKOL_SHADER_HEADER SOKOL_SHADER_VERSION SOKOL_SHADER_PRECISION
#define SOKOL_SHADER_VERSION  "#version 300 es\n"
#define SOKOL_SHADER_PRECISION "precision highp float;\n"
#endif

#ifdef NDEBUG
#define SOKOL_ASSERT(c)
#else
#define SOKOL_DEBUG
#endif

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
