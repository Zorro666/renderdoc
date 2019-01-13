#include "cgl_jake.h"

void JakeCocoaSwapBuffers(void* context)
{
	NSOpenGLContext* object  = (NSOpenGLContext*)context;
	if (object)
		[object flushBuffer];
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
