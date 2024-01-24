/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Baldur Karlsson
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

#pragma once

#include "../apple/official/metal-cpp.h"
#include "../test_common.h"

#define CHECK_MTL_VALID(object, cmd, msg)                                                         \
  do                                                                                              \
  {                                                                                               \
    if(!object)                                                                                   \
    {                                                                                             \
      fprintf(stdout, "%s:%d Metal Error: '%s' executing:\n%s\n", __FILE__, __LINE__, msg, #cmd); \
      fflush(stdout);                                                                             \
      DEBUG_BREAK();                                                                              \
      exit(1);                                                                                    \
    }                                                                                             \
  } while(0);

struct MetalGraphicsTest;

struct MetalWindow : public GraphicsWindow
{
  MetalWindow(MetalGraphicsTest *test, GraphicsWindow *win);
  virtual ~MetalWindow();
  void Shutdown();

  bool Initialised() { return m_mtlLayer != NULL; }
  void Acquire();
  CA::MetalDrawable *Drawable() const { return m_mtlDrawable; }
  MTL::Texture *BackBuffer() const { return m_mtlBackbuffer; }
  // forward GraphicsWindow functions to internal window
  void Resize(int width, int height) { m_Win->Resize(width, height); }
  bool Update() { return m_Win->Update(); }
private:
  void CreateSwapchain(NS::View *nsView, MTL::Device *mtlDevice);
  void DestroySwapchain();
  GraphicsWindow *m_Win;
  MetalGraphicsTest *m_Test;
  CA::MetalLayer *m_mtlLayer = NULL;
  CA::MetalDrawable *m_mtlDrawable = NULL;
  MTL::Texture *m_mtlBackbuffer = NULL;
};

struct MetalGraphicsTest : public GraphicsTest
{
  static const TestAPI API = TestAPI::Metal;

  MetalGraphicsTest();

  void Prepare(int argc, char **argv);
  bool Init();
  void Shutdown();
  MetalWindow *MakeWindow(int width, int height, const char *title);

  bool Running();
  void Present();

  // Helper methods
  MTL::CommandBuffer *GetCommandBuffer();
  MTL::RenderPipelineState *CreateDefaultPipeline();
  MTL::Function *CompileShaderFunction(const std::string &shaderSource, const char *functionName);
  MTL::RenderCommandEncoder *CreateRenderPass(MTL::Texture *fb, MTL::CommandBuffer *cmdBuffer);
  void WaitForGPU();

  // core objects
  MTL::Device *m_device;
  MTL::CommandQueue *m_queue;
  NS::AutoreleasePool *m_autoreleasePool;

  MetalWindow *m_mainWindow = NULL;

private:
  GraphicsWindow *MakePlatformWindow(int width, int height, const char *title);
};

extern std::string MetalDefaultVertex;
extern std::string MetalDefaultFragment;
