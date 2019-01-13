#import <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>

// taken from Arseny's comment explaining how to make Qt widgets metal compatible:
// https://github.com/KhronosGroup/MoltenVK/issues/78#issuecomment-369838674
extern "C" void *makeNSViewMetalCompatible(void *handle)
{
  NSView *view = (NSView *)handle;
  assert([view isKindOfClass:[NSView class]]);

  if(![view.layer isKindOfClass:[CAMetalLayer class]])
  {
    [view setLayer:[CAMetalLayer layer]];
    [view setWantsLayer:YES];
  }

  return view.layer;
}

extern "C" id JakeMakeNSViewGLCompatible(id handle)
{
	NSView *view = (NSView *)handle;
	assert([view isKindOfClass:[NSView class]]);
	[view setWantsBestResolutionOpenGLSurface:YES];
	return view.window;
}

extern "C" void* JakeCreateNSOpenGLContext(id view)
{
	NSOpenGLPixelFormatAttribute attributes[14] = {
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAClosestPolicy,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		0 };

	id pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    if (pixelFormat == nil)
    {
        fprintf(stderr, "NSOpenGLPixelFormat: Failed to find a suitable pixel format");
        return NULL;
    }

    id object = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:NULL];
    if (object == nil)
    {
		[pixelFormat release];
		pixelFormat = nil;
        fprintf(stderr, "NSOpenGLContext: Failed to create OpenGL context with pixelFormat");
        return NULL;
    }
	[pixelFormat release];
	pixelFormat = nil;

    [object setView:view];
	[object makeCurrentContext];

	return object;
}