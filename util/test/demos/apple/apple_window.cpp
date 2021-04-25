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

#include "apple_window.h"

AppleGLWindow::~AppleGLWindow()
{
  COCOA_DeleteWindow(cocoa_window);
  cocoa_window = nullptr;
}

AppleGLWindow::AppleGLWindow(int width, int height, const char *title)
{
  cocoa_window = COCOA_NewWindow(width, height, title);
}

bool AppleGLWindow::Init()
{
  if(!COCOA_Initialize())
  {
    return false;
  }
  return true;
}

void AppleGLWindow::Resize(int width, int height)
{
  TEST_ERROR("Resize is not implemented");
}

bool AppleGLWindow::Update()
{
  if(COCOA_WindowShouldClose(cocoa_window))
    return false;

  COCOA_Poll();
  return true;
}
