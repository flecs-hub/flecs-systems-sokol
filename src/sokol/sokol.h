// #ifdef __APPLE__
// #include <OpenGL/gl3.h>
// #else
// #include <GL/glew.h>
// #endif

/* Code is used as an importable module, so apps must provide their own main */
#define SOKOL_NO_ENTRY

/* Select graphics API implementation */
#define SOKOL_GLCORE33

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
