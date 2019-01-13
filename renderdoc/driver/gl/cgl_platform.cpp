/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "cgl_dispatch_table.h"
#include "gl_common.h"
#include "hooks/hooks.h"
#include "gl_dispatch_table.h"

//#include "cgl_cocoa.h"
#include "cgl_cocoa_context.h"
#include "cgl_jake.h"

typedef struct JakeGLContext
{
  id cocoa_object;
  void* cgl_ctx;
} JakeGLContext;

class CGLPlatform : public GLPlatform
{
  bool MakeContextCurrent(GLWindowingData data)
  {
    JakeGLContext* ctx = (JakeGLContext*)(data.ctx);
    if (ctx->cgl_ctx)
      JakeCGLMakeContextCurrent(ctx->cgl_ctx);
    else
      JakeCocoaMakeContextCurrent(ctx->cocoa_object);
    return true;
  }

  GLWindowingData CloneTemporaryContext(GLWindowingData share)
  {
    GLWindowingData ret;
    return ret;
  }

  void DeleteClonedContext(GLWindowingData context)
  {
    JakeGLContext* ctx = (JakeGLContext*)(context.ctx);
    if (ctx->cgl_ctx)
      JakeCGLDeleteContext(ctx->cgl_ctx);
    else
      JakeCocoaDeleteContext(ctx->cocoa_object);
  }

  void DeleteReplayContext(GLWindowingData context)
  {
    JakeCGLMakeContextCurrent(NULL);
    JakeCocoaMakeContextCurrent(NULL);
    JakeGLContext* ctx = (JakeGLContext*)(context.ctx);
    if (ctx->cgl_ctx)
      JakeCGLDeleteContext(ctx->cgl_ctx);
    else
      JakeCocoaDeleteContext(ctx->cocoa_object);
  }

  void SwapBuffers(GLWindowingData context)
  {
    RDCDEBUG("SwapBuffers not implemented");
    // JAKE TODO: JakeSwapBuffers(context.ctx);
  }

  void GetOutputWindowDimensions(GLWindowingData context, int32_t &w, int32_t &h)
  {
    RDCDEBUG("GetOutputWindowDimensions not implemented");
    // JAKE: Cocoa method has to be called from the main thread
    //JakeWindow* window = (JakeWindow*)context.wnd;
    w = 256;
    h = 256;
    //int width = 0;
    //int height = 0;
    //JakeGetWindowSize(window, &width, &height);
    //w = width;
    //h = height;
  }

  bool IsOutputWindowVisible(GLWindowingData context)
  {
    GLNOTIMP("Optimisation missing - output window always returning true");
    return true;
  }

  GLWindowingData MakeOutputWindow(WindowingData window, bool depth, GLWindowingData share_context)
  {
    GLWindowingData ret = {};
    if (window.system == WindowingSystem::MacOS)
    {
      JakeGLContext* ctx = (JakeGLContext*)malloc(sizeof(JakeGLContext));
      ctx->cgl_ctx = NULL;
      ctx->cocoa_object = window.macOS.ctx_object;
      RDCASSERT(!ctx->cgl_ctx);
      RDCASSERT(ctx->cocoa_object);
      ret.ctx = ctx;

      JakeWindow* wnd = (JakeWindow*)malloc(sizeof(JakeWindow));
      wnd->object = window.macOS.window_object;
      wnd->view = window.macOS.window_view;
      wnd->width = 128;
      wnd->height = 128;
      ret.wnd = wnd;
    }
    else if(window.system == WindowingSystem::Unknown)
    {
      //ret.ctx = JakeGetCurrentContext();
      //RDCASSERT(ret.ctx);
      //ret.wnd = JakeGetCurrentWindow();
      //RDCASSERT(ret.wnd);
      JakeGLContext* ctx = (JakeGLContext*)malloc(sizeof(JakeGLContext));
      ctx->cgl_ctx = JakeCGLCreateContext();
      ctx->cocoa_object = NULL;
      RDCASSERT(ctx->cgl_ctx);
      RDCASSERT(!ctx->cocoa_object);
      ret.ctx = ctx;
      ret.wnd = NULL;
    }
    else
    {
      RDCERR("Unexpected window system %u", system);
    }
    return ret;
  }

  void *GetReplayFunction(const char *funcname)
  {

    return NULL;
  }

  bool CanCreateGLESContext() { return false; }
  bool PopulateForReplay() { return CGL.PopulateForReplay(); }
  ReplayStatus InitialiseAPI(GLWindowingData &replayContext, RDCDriver api)
  {
    RDCASSERT(api == RDCDriver::OpenGL);

    // JAKE: Return the current context and Window 
    // JAKE: which have to be already created on the main thread
    //replayContext.ctx = JakeGetCurrentContext();
    //replayContext.wnd = JakeGetCurrentWindow();
    //replayContext.ctx = JakeCGLCreateContext();
    //replayContext.wnd = NULL;

    JakeGLContext* ctx = (JakeGLContext*)malloc(sizeof(JakeGLContext));
    ctx->cgl_ctx = JakeCGLCreateContext();
    RDCASSERT(ctx->cgl_ctx);
    ctx->cocoa_object = NULL;
    replayContext.ctx = ctx;
    replayContext.wnd = NULL;

    /*
    JakeCGLMakeContextCurrent(ctx->cgl_ctx);
    int numExts = 0;
    GL.glGetIntegerv(eGL_NUM_EXTENSIONS, &numExts);
    RDCLOG("JAKE: numExts: %d", numExts);

    const char* vendor = (const char *)GL.glGetString(eGL_VENDOR);
    const char *renderer = (const char *)GL.glGetString(eGL_RENDERER);
    const char *version = (const char *)GL.glGetString(eGL_VERSION);
    RDCLOG("JAKE: %s / %s / %s", vendor, renderer, version);
    GLint major = 0;
    GLint minor = 0;
    GL.glGetIntegerv(eGL_MAJOR_VERSION, &major);
    GL.glGetIntegerv(eGL_MINOR_VERSION, &minor);
    RDCLOG("JAKE: %d / %d", major, minor);
    */
    return ReplayStatus::Succeeded;
  }

  void DrawQuads(float width, float height, const std::vector<Vec4f> &vertices) { RDCERR("DrawQuads not supported");}
} cglPlatform;

CGLDispatchTable CGL = {};

GLPlatform &GetGLPlatform()
{
  return cglPlatform;
}

bool CGLDispatchTable::PopulateForReplay()
{
  RDCDEBUG("Initialising CGL function pointers");
  LibraryHooks::RegisterHooks();

  bool symbols_ok = true;
/* JAKE

  #define LOAD_FUNC(func)                                                                           \
  if(!this->func)                                                                                   \
    this->func = (CONCAT(PFN_, func))Process::GetFunctionAddress(NULL, STRINGIZE(func));          \
                                                                                                    \
  if(!func)                                                                                         \
  {                                                                                                 \
    symbols_ok = false;                                                                             \
    RDCWARN("Unable to load '%s'", STRINGIZE(func));                                                \
  }

  CGL_HOOKED_SYMBOLS(LOAD_FUNC)
  CGL_NONHOOKED_SYMBOLS(LOAD_FUNC)
#undef LOAD_FUNC
*/
  return symbols_ok;
}

