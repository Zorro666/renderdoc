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

#include "../test_common.h"

#include "../apple/official/metal-cpp.h"

#include "metal_test.h"

#if defined(__APPLE__)
#include "../apple/apple_window.h"
#else
#error UNKNOWN PLATFORM
#endif

static std::string MetalShaderCommon = R"(

#include <metal_stdlib>
using namespace metal;

struct V2F
{
  float4 position [[position]];
  float3 color;
};

)";

std::string MetalDefaultVertex = MetalShaderCommon + R"(

struct DefaultA2V {
    float3 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

V2F vertex vertexMain(DefaultA2V vert [[stage_in]])
{
  V2F o;
  o.position = float4( vert.position, 1.0 );
  o.color = float3( vert.color );
  return o;
}

)";

std::string MetalDefaultFragment = MetalShaderCommon + R"(

float4 fragment fragmentMain( V2F in [[stage_in]] )
{
  return float4( in.color, 1.0 );
}

)";

MetalGraphicsTest::MetalGraphicsTest()
{
}

void MetalGraphicsTest::Prepare(int argc, char **argv)
{
#if defined(__APPLE__)
  AppleWindow::Init();
#else
#error UNKNOWN PLATFORM
#endif
  GraphicsTest::Prepare(argc, argv);
}

bool MetalGraphicsTest::Init()
{
  // parse parameters here to override parameters
  if(!GraphicsTest::Init())
    return false;

  m_device = MTL::CreateSystemDefaultDevice();
  m_queue = m_device->newCommandQueue();

  if(!headless)
  {
    m_mainWindow = MakeWindow(screenWidth, screenHeight, "Autotesting (Metal)");

    if(!m_mainWindow->Initialised())
    {
      TEST_ERROR("Error creating surface");
      return false;
    }
  }

  return true;
}

MetalWindow *MetalGraphicsTest::MakeWindow(int width, int height, const char *title)
{
#if defined(__APPLE__)
  GraphicsWindow *platWin = new AppleWindow(width, height, title);
#else
#error UNKNOWN PLATFORM
#endif

  return new MetalWindow(this, platWin);
}

void MetalGraphicsTest::Shutdown()
{
  if(m_device)
  {
    WaitForGPU();
    delete m_mainWindow;

    m_queue->release();
    m_queue = NULL;
    m_device->release();
    m_device = NULL;
  }
  if(m_autoreleasePool)
  {
    m_autoreleasePool->drain();
    m_autoreleasePool = NULL;
  }
}

bool MetalGraphicsTest::Running()
{
  if(!FrameLimit())
    return false;

  if(!m_mainWindow->Update())
    return false;

  return true;
}

void MetalGraphicsTest::Present()
{
  MTL::CommandBuffer *cmdBuffer = GetCommandBuffer();
  cmdBuffer->presentDrawable(m_mainWindow->Drawable());
  cmdBuffer->commit();
  m_mainWindow->Acquire();
}

MTL::CommandBuffer *MetalGraphicsTest::GetCommandBuffer()
{
  return m_queue->commandBuffer();
}

void MetalGraphicsTest::WaitForGPU()
{
  MTL::CommandBuffer *cmdBuffer = GetCommandBuffer();
  cmdBuffer->commit();
  cmdBuffer->waitUntilCompleted();
}

MTL::RenderPipelineState *MetalGraphicsTest::CreateDefaultPipeline()
{
  MTL::Function *vertexFunction = CompileShaderFunction(MetalDefaultVertex, "vertexMain");
  MTL::Function *fragmentFunction = CompileShaderFunction(MetalDefaultFragment, "fragmentMain");

  MTL::RenderPipelineDescriptor *pipelineDesc = MTL::RenderPipelineDescriptor::alloc()->init();
  pipelineDesc->setVertexFunction(vertexFunction);
  pipelineDesc->setFragmentFunction(fragmentFunction);
  pipelineDesc->colorAttachments()->object(0)->setPixelFormat(
      MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

  MTL::VertexDescriptor *vertexDesc = MTL::VertexDescriptor::vertexDescriptor();
  MTL::VertexAttributeDescriptor *attribute0 = vertexDesc->attributes()->object(0);
  attribute0->setFormat(MTL::VertexFormatFloat3);
  attribute0->setOffset(offsetof(DefaultA2V, pos));
  attribute0->setBufferIndex(0);
  MTL::VertexAttributeDescriptor *attribute1 = vertexDesc->attributes()->object(1);
  attribute1->setFormat(MTL::VertexFormatFloat4);
  attribute1->setOffset(offsetof(DefaultA2V, col));
  attribute1->setBufferIndex(0);
  MTL::VertexBufferLayoutDescriptor *layout0 = vertexDesc->layouts()->object(0);
  layout0->setStride(sizeof(DefaultA2V));

  pipelineDesc->setVertexDescriptor(vertexDesc);

  NS::Error *pError = NULL;
  MTL::RenderPipelineState *rpState = m_device->newRenderPipelineState(pipelineDesc, &pError);
  CHECK_MTL_VALID(rpState, newRenderPipelineState, pError->localizedDescription()->utf8String());

  vertexFunction->release();
  fragmentFunction->release();
  pipelineDesc->release();

  return rpState;
}

MTL::Function *MetalGraphicsTest::CompileShaderFunction(const std::string &shaderSource,
                                                        const char *functionName)
{
  NS::Error *pError = NULL;
  MTL::Library *library = m_device->newLibrary(
      NS::String::string(shaderSource.data(), NS::StringEncoding::UTF8StringEncoding), NULL, &pError);
  CHECK_MTL_VALID(library, newLibrary, pError->localizedDescription()->utf8String());

  MTL::Function *mtlFunction =
      library->newFunction(NS::String::string(functionName, NS::StringEncoding::UTF8StringEncoding));
  library->release();

  return mtlFunction;
}

MTL::RenderCommandEncoder *MetalGraphicsTest::CreateRenderPass(MTL::Texture *fb,
                                                               MTL::CommandBuffer *cmdBuffer)
{
  MTL::RenderPassDescriptor *passDesc = MTL::RenderPassDescriptor::renderPassDescriptor();
  MTL::RenderPassColorAttachmentDescriptorArray *colorAttachments = passDesc->colorAttachments();
  colorAttachments->object(0)->setTexture(fb);
  colorAttachments->object(0)->setClearColor(MTL::ClearColor(0.2, 0.2, 0.2, 1.0));
  colorAttachments->object(0)->setStoreAction(MTL::StoreActionStore);
  colorAttachments->object(0)->setLoadAction(MTL::LoadActionClear);

  return cmdBuffer->renderCommandEncoder(passDesc);
}

MetalWindow::MetalWindow(MetalGraphicsTest *test, GraphicsWindow *win) : GraphicsWindow(win->title)
{
  m_Test = test;
  m_Win = win;

  NS::View *nsView = (NS::View *)((AppleWindow *)win)->view;
  CreateSwapchain(nsView, test->m_device);
  Acquire();
}

MetalWindow::~MetalWindow()
{
  DestroySwapchain();
  delete m_Win;
}

void MetalWindow::CreateSwapchain(NS::View *nsView, MTL::Device *mtlDevice)
{
  m_mtlLayer = (CA::MetalLayer *)nsView->layer();
  assert(object_getClass(m_mtlLayer) == objc_getClass("CAMetalLayer"));
  m_mtlLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
  m_mtlLayer->setFramebufferOnly(true);
  m_mtlLayer->setDevice(mtlDevice);
  assert(m_mtlDrawable == NULL);
  assert(m_mtlBackbuffer == NULL);
}

void MetalWindow::DestroySwapchain()
{
  m_mtlLayer = NULL;
  m_mtlBackbuffer = NULL;
}

void MetalWindow::Acquire()
{
  if(m_mtlDrawable)
    m_mtlDrawable->release();
  m_mtlDrawable = m_mtlLayer->nextDrawable();
  m_mtlDrawable->retain();
  m_mtlBackbuffer = m_mtlDrawable->texture();
}
