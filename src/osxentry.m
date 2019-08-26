#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <sokol_sdl_metal.h>
#include "osxentry.h"

int _sample_count;
CAMetalLayer* metal_layer;
MTLRenderPassDescriptor* pass_descriptor;
id<CAMetalDrawable> next_drawable;
id<MTLTexture> depth_texture;
id<MTLTexture> sample_texture;

void _sdl_osx_create_textures() {
	CGSize drawableSize = metal_layer.drawableSize;
	MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
											width:drawableSize.width
											height:drawableSize.height
											mipmapped:NO];
	descriptor.textureType = (_sample_count > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
	descriptor.sampleCount = _sample_count;
	descriptor.storageMode = MTLStorageModePrivate;
	descriptor.usage = MTLTextureUsageRenderTarget;
	depth_texture = [metal_layer.device newTextureWithDescriptor:descriptor];
	depth_texture.label = @"DepthStencil";
	
	descriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
	sample_texture = [metal_layer.device newTextureWithDescriptor:descriptor];
	sample_texture.label = @"SampleTexture";
}
	
//------------------------------------------------------------------------------
const void* sdl_osx_start(void* cametal_layer, int sample_count) {
	_sample_count = sample_count;
	metal_layer = (__bridge __typeof__ (CAMetalLayer*))cametal_layer;
	metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	
	_sdl_osx_create_textures();

	return (__bridge const void*)metal_layer.device;
}

void sdl_osx_frame(ecs_rows_t *rows, sdl_osx_frame_func frame_cb) {
	@autoreleasepool {
		pass_descriptor = [[MTLRenderPassDescriptor alloc] init];
		frame_cb(rows);
		pass_descriptor = nil;
		next_drawable = nil;
	}
}

/* get an MTLRenderPassDescriptor */
const void* sdl_osx_mtk_get_render_pass_descriptor() {
	next_drawable = metal_layer.nextDrawable;
	if (_sample_count > 1 )
	{
		if (next_drawable.texture.width != sample_texture.width || next_drawable.texture.height != sample_texture.height) {
			_sdl_osx_create_textures();
		}
		pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
		pass_descriptor.colorAttachments[0].texture = sample_texture;
		pass_descriptor.colorAttachments[0].resolveTexture = next_drawable.texture;
	}
	else
	{
		pass_descriptor.colorAttachments[0].texture = next_drawable.texture;
	}
	
	pass_descriptor.depthAttachment.texture = depth_texture;
	pass_descriptor.stencilAttachment.texture = depth_texture;
	
	return (__bridge const void*)pass_descriptor;
}

/* get the current CAMetalDrawable */
const void* sdl_osx_mtk_get_drawable() {
	return (__bridge const void*)next_drawable;
}
