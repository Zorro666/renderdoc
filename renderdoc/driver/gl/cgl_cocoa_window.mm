#if 0 
#include "cgl_cocoa.h"
#include "cgl_cocoa_context.h"

JakeWindow* s_jakeWindows[MAX_NSGL_CONTEXTS] = {NULL};

void JakeCreateWindowInternal(int width, int height, const char* title)
{
	size_t s_jakeCreateCurrentWindow = 0;
	if (s_jakeCreateCurrentWindow >= MAX_NSGL_CONTEXTS)
	{
		return;
	}
	JakeWindow* window = (JakeWindow*)malloc(sizeof(JakeWindow));

	window->width  = width;
	window->height = height;

	NSRect contentRect = NSMakeRect(0, 0, width, height);
	window->object = [[NSWindow alloc]
						initWithContentRect:contentRect
						styleMask:NSBorderlessWindowMask
						backing:NSBackingStoreBuffered
						defer:NO];

	window->view = [[NSView alloc] initWithFrame:contentRect];
	[window->view setWantsBestResolutionOpenGLSurface:YES];
	[window->object setTitle:[NSString stringWithUTF8String:title]];
	[window->object setContentView:window->view];

	[window->object orderFront:nil];

    if (!JakeCreateNSOpenGLContext(window->view))
    {
		free(window);
    }
	s_jakeWindows[s_jakeCreateCurrentWindow] = window;
	s_jakeCreateCurrentWindow++;
}

void JakeCreateWindow(int width, int height, const char* title)
{
	for (int i = 0; i < MAX_NSGL_CONTEXTS; ++i)
	{
		JakeCreateWindowInternal(width, height, title);
	}
}

void JakeDestroyWindow(JakeWindow* window)
{
	//JAKE: TODO
	//JakeDeleteContext(window->context);

    [window->object setDelegate:nil];
    [window->view release];
    window->view = nil;

    [window->object close];
    window->object = nil;

	free(window);
}

id JakeGetViewFromWindow(JakeWindow* window)
{
	return window->view;
}

JakeWindow* JakeGetCurrentWindow(void)
{
	size_t s_jakeGetCurrentWindow = 0;
	if (s_jakeGetCurrentWindow >= MAX_NSGL_CONTEXTS)
	{
		return NULL;
	}
	JakeWindow* window = s_jakeWindows[s_jakeGetCurrentWindow];
	s_jakeGetCurrentWindow++;
	return window;
}

void JakeGetWindowSize(JakeWindow* window, int* width, int* height)
{
	id view = window->view;
	const NSRect viewFrameRect = [view frame];
	const NSRect frameBufferRect = [view convertRectToBacking:viewFrameRect];

	*width = (int)frameBufferRect.size.width;
	*height = (int)frameBufferRect.size.height;
}

#endif // #if 0 