#if 0
#include "cgl_cocoa_context.h"

#import <Cocoa/Cocoa.h>

JakeNSOpenGLContext* s_nsOpenGLContexts[MAX_NSGL_CONTEXTS];

int JakeCreateNSOpenGLContext(id view)
{
	size_t s_jakeCreateCurrentContext = 0;
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

	JakeNSOpenGLContext* context = (JakeNSOpenGLContext*)malloc(sizeof(JakeNSOpenGLContext));
    context->pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    if (context->pixelFormat == nil)
    {
        fprintf(stderr, "NSOpenGLPixelFormat: Failed to find a suitable pixel format");
        return 0;
    }

    context->object = [[NSOpenGLContext alloc] initWithFormat:context->pixelFormat shareContext:NULL];
    if (context->object == nil)
    {
        fprintf(stderr, "NSOpenGLContext: Failed to create OpenGL context with pixelFormat");
        return 0;
    }

    [context->object setView:view];
	[context->object makeCurrentContext];
	s_nsOpenGLContexts[s_jakeCreateCurrentContext] = context;
	s_jakeCreateCurrentContext++;
    return 1;
}

void* JakeGetCurrentContext(void)
{
	size_t s_jakeGetCurrentContext = 0;
	if (s_jakeGetCurrentContext >= MAX_NSGL_CONTEXTS)
	{
		return NULL;
	}
	JakeNSOpenGLContext* context = s_nsOpenGLContexts[s_jakeGetCurrentContext];
	s_jakeGetCurrentContext++;
	return context;
}

void JakeSwapBuffers(void* context)
{
	JakeNSOpenGLContext* ctx = (JakeNSOpenGLContext*)context;
	if (ctx)
		[ctx->object flushBuffer];
}

void JakeCocoaMakeContextCurrent(void* context)
{
	if (context)
	{
		NSOpenGLContext* object  = (NSOpenGLContext*)context;
		[object makeCurrentContext];
	}
	else
		[NSOpenGLContext clearCurrentContext];
}

void JakeCocoaDeleteContext(void* context)
{
	NSOpenGLContext* object  = (NSOpenGLContext*)context;
	[object clearDrawable];
	[object release];
}

#endif // #if 0