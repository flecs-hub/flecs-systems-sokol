#include <sokol_sdl_metal.h>

#define SOKOL_IMPL
#define SOKOL_METAL

#include <flecs_systems_sokol.h>
#include "sokol_gfx.h"
#include "osxentry.h"

typedef struct SokolCanvas2D {
    SDL_Window* window;
	sg_pass_action pass_action;
	sg_pipeline pip;
	sg_bindings bind;	
} SokolCanvas2D;

static
void init_sdl(void)
{	
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

static
void deinit_sdl(void)
{
	SDL_Quit();
}

static
void sokol_init(
	const void* mtl_device,
	SokolCanvas2D *canvas) 
{
	/* setup sokol */
	sg_desc desc = {
		.mtl_device = mtl_device,
		.mtl_renderpass_descriptor_cb = sdl_osx_mtk_get_render_pass_descriptor,
		.mtl_drawable_cb = sdl_osx_mtk_get_drawable
	};

	sg_setup(&desc);
	
	/* setup pass action to clear to red */
	canvas->pass_action = (sg_pass_action) {
		.colors[0] = { .action = SG_ACTION_CLEAR, .val = { 0.0f, 0.0f, 0.0f, 1.0f } }
	};
	
	/* a vertex buffer with 3 vertices */
	float vertices[] = {
		// positions        colors
		0.0f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f
	};

	canvas->bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.size = sizeof(vertices),
		.content = vertices
	});
	
	/* a shader pair, compiled from source code */
	sg_shader shd = sg_make_shader(&(sg_shader_desc){
		/*
		 The shader main() function cannot be called 'main' in
		 the Metal shader languages, thus we define '_main' as the
		 default function. This can be override with the
		 sg_shader_desc.vs.entry and sg_shader_desc.fs.entry fields.
		 */
		.vs.source =
			"#include <metal_stdlib>\n"
			"using namespace metal;\n"
			"struct vs_in {\n"
			"  float4 position [[attribute(0)]];\n"
			"  float4 color [[attribute(1)]];\n"
			"};\n"
			"struct vs_out {\n"
			"  float4 position [[position]];\n"
			"  float4 color;\n"
			"};\n"
			"vertex vs_out _main(vs_in inp [[stage_in]]) {\n"
			"  vs_out outp;\n"
			"  outp.position = inp.position;\n"
			"  outp.color = inp.color;\n"
			"  return outp;\n"
			"}\n",

		.fs.source =
			"#include <metal_stdlib>\n"
			"using namespace metal;\n"
			"fragment float4 _main(float4 color [[stage_in]]) {\n"
			"  return color;\n"
			"};\n"
	});
	
	/* create a pipeline object */
	canvas->pip = sg_make_pipeline(&(sg_pipeline_desc){
		/* Metal has explicit attribute locations, and the vertex layout
		 has no gaps, so we don't need to provide stride, offsets
		 or attribute names
		 */
		.layout = {
			.attrs = {
				[0] = { .format = SG_VERTEXFORMAT_FLOAT3 },
				[1] = { .format = SG_VERTEXFORMAT_FLOAT4 }
			},
		},
		.shader = shd
	});
}

static
void SokolDeinit(ecs_rows_t *rows)  
{
	sg_shutdown();	
	deinit_sdl();
}

static
void SokolCreateCanvas(ecs_rows_t *rows)
{
	ECS_COLUMN(rows, EcsCanvas2D, canvas, 1);
	ECS_COLUMN_COMPONENT(rows, SokolCanvas2D, 2);

	ecs_world_t *world = rows->world;
	ecs_entity_t self = rows->entities[0];

	/* Add SokolCanvas2D to canvas entity */
	ecs_add(world, self, SokolCanvas2D);
	SokolCanvas2D *sokol_canvas = ecs_get_ptr(world, self, SokolCanvas2D);
	memset(sokol_canvas, 0, sizeof(SokolCanvas2D));

	/* Create SDL window */
	SDL_Window* window = SDL_CreateWindow(
		canvas->title, 
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, 
		canvas->window.width, 
		canvas->window.height, 
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

	sokol_canvas->window = window;

	/* Obtain  metal device */
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	void* metal_layer = SDL_RenderGetMetalLayer(renderer);
	SDL_DestroyRenderer(renderer);
	const void* mtl_device = sdl_osx_start(metal_layer, 1);

	/* Initialize sokol structures */
	sokol_init(mtl_device, sokol_canvas);

	ecs_os_dbg("sokol window initialized");
}

static
void sokol_render(ecs_rows_t *rows)
{
	ECS_COLUMN(rows, SokolCanvas2D, canvas, 1);

	int w, h;
	SDL_GetWindowSize(canvas->window, &w, &h);
	
	/* draw one frame */
	sg_begin_default_pass(&canvas->pass_action, w, h);

	sg_apply_pipeline(canvas->pip);
	sg_apply_bindings(&canvas->bind);
	sg_draw(0, 3, 1);
	
	sg_end_pass();
	sg_commit();
}

static
void SokolRender(ecs_rows_t *rows) 
{
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0) {
		switch (e.type) {
			case SDL_QUIT: 
				ecs_quit(rows->world); 
				break;
		}
	}

	sdl_osx_frame(rows, sokol_render);
}

void FlecsSystemsSokolImport(
    ecs_world_t *world,
    int flags)
{
	init_sdl();

	ECS_MODULE(world, FlecsSystemsSokol);

	ECS_COMPONENT(world, SokolCanvas2D);

	/* Deinit */ 
	ECS_SYSTEM(world, SokolDeinit, EcsOnRemove, SYSTEM.EcsHidden);

	/* Window creation */
	ECS_SYSTEM(world, SokolCreateCanvas, EcsOnSet, EcsCanvas2D, .SokolCanvas2D, SYSTEM.EcsHidden);

	/* Frame rendering */
	ECS_SYSTEM(world, SokolRender, EcsOnStore, SokolCanvas2D, SYSTEM.EcsHidden);
}
