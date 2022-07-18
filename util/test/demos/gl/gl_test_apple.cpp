/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2019-2021 Baldur Karlsson
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

#include "gl_test.h"
#include "apple/apple_window.h"

void OpenGLGraphicsTest::Prepare(int argc, char **argv)
{
  GraphicsTest::Prepare(argc, argv);
}

bool OpenGLGraphicsTest::Init()
{
  if(!GraphicsTest::Init())
    return false;

  if(!AppleWindow::Init())
  {
    TEST_ERROR("Error failed to initialise AppleWindow");
    return false;
  }

  mainWindow = MakeWindow(screenWidth, screenHeight, screenTitle);
  mainContext = MakeContext(mainWindow, NULL);
  if(!mainWindow || !mainContext)
  {
    delete mainWindow;
    TEST_ERROR("Error failed to initialise main window and context");
    return false;
  }

  ActivateContext(mainWindow, mainContext);

  if(!gladLoadGL())
  {
    TEST_ERROR("Error initialising glad");
    return false;
  }

  PostInit();

  return true;
}

GraphicsWindow *OpenGLGraphicsTest::MakeWindow(int width, int height, const char *title)
{
  if(mainWindow)
  {
    TEST_ERROR("Error can't create more than one window");
    return nullptr;
  }
  return new AppleWindow(width, height, title);
}

void *OpenGLGraphicsTest::MakeContext(GraphicsWindow *win, void *share)
{
  if((share != nullptr) && (share != win))
  {
    TEST_ERROR("Error can't make more than one context per window");
    return nullptr;
  }

  AppleWindow *window = (AppleWindow *)win;
  //  return (void *)COCOA_NewGLContext(window->cocoa_window);
  return NULL;
}

void OpenGLGraphicsTest::DestroyContext(void *ctx)
{
  //  COCOA_DeleteGLContext((COCOAcontext *)ctx);
}

void OpenGLGraphicsTest::ActivateContext(GraphicsWindow *win, void *ctx, bool alt)
{
  AppleWindow *window = (AppleWindow *)win;
  //  COCOA_SetGLContext(window->cocoa_window, (COCOAcontext *)ctx);
}

void OpenGLGraphicsTest::Present(GraphicsWindow *win)
{
  AppleWindow *window = (AppleWindow *)win;
  //  COCOA_SwapBuffers(window->cocoa_window);
}
