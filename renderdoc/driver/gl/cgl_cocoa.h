#if 0
#ifndef CGL_COCOA_H
#define CGL_COCOA_H

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#else
typedef void* id;
#endif

typedef struct JakeWindow
{
	int width;
	int height;
	id object;
	id view;
} JakeWindow;

void JakeCreateWindow(int width, int height, const char* title);
JakeWindow* JakeGetCurrentWindow(void);
void JakeGetWindowSize(JakeWindow* window, int* width, int* height);
//id JakeGetViewFromWindow(JakeWindow* window);

//void JakeDestroyWindow(JakeWindow* window);

void JakeSwapBuffers(void* context);
*/

#endif // #ifndef CGL_COCOA_H

#endif // #if 0