#include "cgl_jake.h"
#include <stdlib.h>
#include <OpenGL/OpenGL.h>

void* JakeCGLCreateContext(void)
{
	CGLContextObj* ctx = (CGLContextObj*)malloc(sizeof(CGLContextObj));
 
	CGLPixelFormatAttribute attributes[13] = {
		kCGLPFAAccelerated,
		kCGLPFADoubleBuffer,
		kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
		kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
		kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
		kCGLPFADepthSize, (CGLPixelFormatAttribute)24,
		kCGLPFAStencilSize, (CGLPixelFormatAttribute)8,
		(CGLPixelFormatAttribute) 0
	};

	CGLPixelFormatObj pix;
	CGLError errorCode;
	GLint num;
	errorCode = CGLChoosePixelFormat(attributes, &pix, &num);
  	errorCode = CGLCreateContext(pix, NULL, ctx);
	CGLDestroyPixelFormat(pix);
	return ctx;
}

void JakeCGLMakeContextCurrent(void* context)
{
	CGLContextObj* ctx = (CGLContextObj*)(context);
	if (ctx)
		CGLSetCurrentContext(*ctx);
	else
		CGLSetCurrentContext(NULL);
}

void JakeCGLDeleteContext(void* context)
{
	CGLContextObj* ctx = (CGLContextObj*)(context);
	CGLSetCurrentContext(NULL);
	CGLDestroyContext(*ctx);
	free(ctx);
}