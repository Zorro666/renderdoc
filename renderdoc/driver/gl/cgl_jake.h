#ifndef CGL_JAKE_H
#define CGL_JAKE_H

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

void* JakeCGLCreateContext(void);
void JakeCGLMakeContextCurrent(void* context);
void JakeCGLDeleteContext(void* context);

void JakeCocoaMakeContextCurrent(void* context);
void JakeCocoaDeleteContext(void* context);

#endif // #ifndef CGL_JAKE_H
