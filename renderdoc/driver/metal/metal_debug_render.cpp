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

#include "maths/formatpacking.h"
#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_common.h"
#include "metal_device.h"
#include "metal_function.h"
#include "metal_library.h"
#include "metal_render_command_encoder.h"
#include "metal_replay.h"
#include "metal_shaders.h"
#include "metal_shaders.metal"
#include "metal_texture.h"

void MetalReplay::InitDebugRenderer()
{
  RDCASSERT(m_debugLibrary == NULL);
  RDCASSERT(m_checkerboardPipeline == NULL);
  RDCASSERT(m_outlinePipeline == NULL);

  NS::String *nsString = NULL;
  NS::Error *error = NULL;
  nsString = NS::String::string(debugShaders, NS::UTF8StringEncoding);
  m_debugLibrary = m_pDriver->newLibraryWithSource(nsString, NULL, &error);
  nsString = NULL;
  if(Unwrap(m_debugLibrary) == NULL)
  {
    RDCERR("Error occurred when creating debug shaders library\n%s\n%s\n%s",
           error->localizedDescription(), error->localizedRecoverySuggestion(),
           error->localizedFailureReason());
    return;
  }

  nsString = NS::String::string("blit_vertex", NS::UTF8StringEncoding);
  WrappedMTLFunction *blitVertex = m_debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  nsString = NS::String::string("checkerboard_fragment", NS::UTF8StringEncoding);
  WrappedMTLFunction *checkerboardFrag = m_debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  RDMTL::RenderPipelineDescriptor descriptor;
  descriptor.colorAttachments.resize(1);
  descriptor.colorAttachments[0].pixelFormat = MTL::PixelFormatBGRA8Unorm;
  descriptor.vertexFunction = blitVertex;
  descriptor.fragmentFunction = checkerboardFrag;

  m_checkerboardPipeline = m_pDriver->newRenderPipelineStateWithDescriptor(descriptor, &error);

  nsString = NS::String::string("fullscreen_vertex", NS::UTF8StringEncoding);
  WrappedMTLFunction *outlineVertex = m_debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  nsString = NS::String::string("colour_fragment", NS::UTF8StringEncoding);
  WrappedMTLFunction *outlineFrag = m_debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  descriptor.vertexFunction = outlineVertex;
  descriptor.fragmentFunction = outlineFrag;
  m_outlinePipeline = m_pDriver->newRenderPipelineStateWithDescriptor(descriptor, &error);
}

void MetalReplay::ShutdownDebugRenderer()
{
  Unwrap(m_checkerboardPipeline)->release();
  m_checkerboardPipeline = NULL;
  Unwrap(m_debugLibrary)->release();
  m_debugLibrary = NULL;
}

void MetalReplay::RenderCheckerboard(FloatVector dark, FloatVector light)
{
  auto it = m_OutputWindows.find(m_ActiveWinID);
  if(m_ActiveWinID == 0 || it == m_OutputWindows.end())
    return;

  OutputWindow &outw = it->second;

  // if the swapchain failed to create, do nothing. We will try to recreate it
  // again in CheckResizeOutputWindow (once per render 'frame')
  if(outw.NoOutput())
    return;

  if(m_checkerboardPipeline == NULL)
    return;

  WrappedMTLCommandBuffer *commandBuffer = m_pDriver->GetNextCommandBuffer();
  if(commandBuffer == NULL)
    return;

  WrappedMTLBuffer *cbuffer = outw.CBuffers[OutputWindow::BUFFER_CHECKERBOARD];
  CheckerUBOData *data = (CheckerUBOData *)cbuffer->contents();
  data->BorderWidth = 0.0f;
  data->RectPosition = Vec2f();
  data->RectSize = Vec2f();
  data->CheckerSquareDimension = 64.0f;
  data->InnerColor = Vec4f();

  data->PrimaryColor = light;
  data->SecondaryColor = dark;

  RDMTL::RenderPassDescriptor rPassDesc;
  FillRenderPassDescriptor(outw.FB, MTL::LoadActionClear, MTL::StoreActionStore, 1.0f, 1.0f, 1.0f,
                           1.0f, rPassDesc);
  MTL::RenderPassDescriptor *mtlDescriptor(rPassDesc);

  MTL::RenderCommandEncoder *encoder = Unwrap(commandBuffer)->renderCommandEncoder(mtlDescriptor);
  encoder->setRenderPipelineState(Unwrap(m_checkerboardPipeline));
  encoder->setFragmentBuffer(Unwrap(cbuffer), 0, 0);
  encoder->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, 0, 4, 1);
  encoder->endEncoding();
}

void MetalReplay::RenderHighlightBox(float w, float h, float scale)
{
  auto it = m_OutputWindows.find(m_ActiveWinID);
  if(m_ActiveWinID == 0 || it == m_OutputWindows.end())
    return;

  OutputWindow &outw = it->second;

  // do nothing if the backbuffer failed to create
  if(outw.WindowSystem != WindowingSystem::Headless && outw.FB == NULL)
    return;

  WrappedMTLCommandBuffer *commandBuffer = m_pDriver->GetNextCommandBuffer();
  if(commandBuffer == NULL)
    return;

  RDMTL::RenderPassDescriptor rPassDesc;
  FillRenderPassDescriptor(outw.FB, MTL::LoadActionLoad, MTL::StoreActionStore, 1.0f, 1.0f, 1.0f,
                           1.0f, rPassDesc);
  MTL::RenderPassDescriptor *mtlDescriptor(rPassDesc);

  MTL::RenderCommandEncoder *encoder = Unwrap(commandBuffer)->renderCommandEncoder(mtlDescriptor);

  NS::UInteger sz = NS::UInteger(scale);
  NS::UInteger x = NS::UInteger(w / 2.0f + 0.5f);
  NS::UInteger y = NS::UInteger(h / 2.0f + 0.5f);

  WrappedMTLBuffer *cbuffer = outw.CBuffers[OutputWindow::BUFFER_CHECKERBOARD];
  Vec4f *colours = (Vec4f *)cbuffer->contents();
  colours[0] = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);
  colours[1] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);

  // Inner outline : 4 x rectangles
  // Rect 0 : x,y : 1 x sz
  // Rect 1 : x+sz, y+1 : 1 x sz
  // Rect 2 : x+1, y    : sz x 1
  // Rect 3 : x, y+sz   : sz x 1
  //        x x+1     x+sz x+sz+1
  // y      *-*------------*
  //        | |            |
  // y+1    | *----------*-*
  //        | |          | |
  //        | |          | |
  //        | |          | |
  // y+sz   *-*----------* *
  //        |            | |
  // y+sz+1 *-*----------*-*

  MTL::ScissorRect scissors[4] = {
      {x, y, 1, sz}, {x + sz, y + 1, 1, sz}, {x + 1, y, sz, 1}, {x, y + sz, sz, 1}};

  encoder->setRenderPipelineState(Unwrap(m_outlinePipeline));
  encoder->setFragmentBuffer(Unwrap(cbuffer), 0, 0);
  for(int i = 0; i < 4; ++i)
  {
    encoder->setScissorRect(scissors[i]);
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, 0, 4, 1);
  }

  // Outer outline : 4 x rectangles
  // Rect 0 : x-1, y-1    : 1 x sz+2
  // Rect 1 : x+sz+1, y   : 1 x sz+2
  // Rect 2 : x, y-1      : sz+2 x 1
  // Rect 3 : x-1, y+sz+1 : sz+2 x 1
  //      x-1 x     x+sz+1 x+sz+2
  // y-1    *-*------------*
  //        | |            |
  // y      | *----------*-*
  //        | |          | |
  //        | |          | |
  //        | |          | |
  // y+sz+1 *-*----------* *
  //        |            | |
  // y+sz+2 *-*----------*-*
  scissors[0].x--;
  scissors[0].y--;
  scissors[0].height += 2;

  scissors[1].x++;
  scissors[1].y--;
  scissors[1].height += 2;

  scissors[2].x--;
  scissors[2].y--;
  scissors[2].width += 2;

  scissors[3].x--;
  scissors[3].y++;
  scissors[3].width += 2;

  encoder->setFragmentBuffer(Unwrap(cbuffer), sizeof(Vec4f), 0);
  for(int i = 0; i < 4; ++i)
  {
    encoder->setScissorRect(scissors[i]);
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, 0, 4, 1);
  }
  encoder->endEncoding();
}

void MetalReplay::TextureRendering::Init(WrappedMTLDevice *driver)
{
  RDCCOMPILE_ASSERT(sizeof(CheckerUBOData) < 2048, "CheckerUBOData is too large");
  RDCCOMPILE_ASSERT(sizeof(TexDisplayUBOData) + sizeof(HeatmapData) < 2048,
                    "TexDisplayUBOData + HeatmapData is too large");

  MetalReplay *replay = driver->GetReplay();
  WrappedMTLLibrary *debugLibrary = replay->GetDebugShaderLibrary();
  if(debugLibrary == NULL)
    return;

  NS::String *nsString = NULL;

  nsString = NS::String::string("blit_vertex", NS::UTF8StringEncoding);
  WrappedMTLFunction *blitVertex = debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  nsString = NS::String::string("texdisplay_fragment", NS::UTF8StringEncoding);
  WrappedMTLFunction *texdisplayFrag = debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  RDMTL::RenderPipelineDescriptor descriptor;
  descriptor.colorAttachments.resize(1);
  RDMTL::RenderPipelineColorAttachmentDescriptor &colorAttachment0 = descriptor.colorAttachments[0];
  colorAttachment0.pixelFormat = MTL::PixelFormatBGRA8Unorm;
  colorAttachment0.blendingEnabled = true;

  colorAttachment0.rgbBlendOperation = MTL::BlendOperationAdd;
  colorAttachment0.sourceRGBBlendFactor = MTL::BlendFactorSourceAlpha;
  colorAttachment0.destinationRGBBlendFactor = MTL::BlendFactorOneMinusSourceAlpha;

  colorAttachment0.alphaBlendOperation = MTL::BlendOperationAdd;
  colorAttachment0.sourceAlphaBlendFactor = MTL::BlendFactorOne;
  colorAttachment0.destinationAlphaBlendFactor = MTL::BlendFactorZero;

  descriptor.vertexFunction = blitVertex;
  descriptor.fragmentFunction = texdisplayFrag;

  Pipeline = driver->newRenderPipelineStateWithDescriptor(descriptor, NULL);
}

void MetalReplay::TextureRendering::Destroy(WrappedMTLDevice *driver)
{
  Unwrap(Pipeline)->release();
  Pipeline = NULL;
}

void MetalReplay::PickPixel::Init(WrappedMTLDevice *driver)
{
  MetalReplay *replay = driver->GetReplay();
  WrappedMTLLibrary *debugLibrary = replay->GetDebugShaderLibrary();
  if(debugLibrary == NULL)
    return;

  MTL::PixelFormat fbFormat = MTL::PixelFormatRGBA32Float;
  NS::String *nsString = NULL;

  nsString = NS::String::string("blit_vertex", NS::UTF8StringEncoding);
  WrappedMTLFunction *blitVertex = debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  nsString = NS::String::string("texdisplay_fragment", NS::UTF8StringEncoding);
  WrappedMTLFunction *texdisplayFrag = debugLibrary->newFunctionWithName(nsString);
  nsString = NULL;

  RDMTL::TextureDescriptor texDesc;
  texDesc.pixelFormat = fbFormat;
  texDesc.width = 1;
  texDesc.height = 1;
  texDesc.usage = (MTL::TextureUsage)(MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget);
  FB = driver->newTextureWithDescriptor(texDesc);

  PipePass.Init(driver, blitVertex, texdisplayFrag, FB, MTL::LoadActionClear, MTL::StoreActionStore,
                0.0f, 0.0f, 0.0f, 1.0f);

  CBuffer = driver->newBufferWithLength(2048, MTL::ResourceStorageModeShared);
}

void MetalReplay::PickPixel::Destroy(WrappedMTLDevice *driver)
{
  Unwrap(CBuffer)->release();
  CBuffer = NULL;
  PipePass.Destroy(driver);
  Unwrap(FB)->release();
  FB = NULL;
}

void FillRenderPassDescriptor(WrappedMTLTexture *fb, MTL::LoadAction loadAction,
                              MTL::StoreAction storeAction, float clearR, float clearG,
                              float clearB, float clearA, RDMTL::RenderPassDescriptor &rPassDesc)
{
  rPassDesc.colorAttachments.resize(1);
  RDMTL::RenderPassColorAttachmentDescriptor &colorAttachment0 = rPassDesc.colorAttachments[0];
  colorAttachment0.texture = fb;
  colorAttachment0.storeAction = storeAction;
  colorAttachment0.loadAction = loadAction;
  colorAttachment0.clearColor = MTL::ClearColor(clearR, clearG, clearB, clearA);
}

void MetalReplay::RenderPipePass::Init(WrappedMTLDevice *driver, WrappedMTLFunction *vertexFunction,
                                       WrappedMTLFunction *fragmentFunction,
                                       WrappedMTLTexture *texture, MTL::LoadAction loadAction,
                                       MTL::StoreAction storeAction, float clearR, float clearG,
                                       float clearB, float clearA)
{
  RDMTL::RenderPipelineDescriptor rPipeDesc;
  rPipeDesc.colorAttachments.resize(1);
  RDMTL::RenderPipelineColorAttachmentDescriptor &pipeColorAttachment0 =
      rPipeDesc.colorAttachments[0];
  pipeColorAttachment0.pixelFormat = Unwrap(texture)->pixelFormat();
  pipeColorAttachment0.blendingEnabled = true;

  pipeColorAttachment0.rgbBlendOperation = MTL::BlendOperationAdd;
  pipeColorAttachment0.sourceRGBBlendFactor = MTL::BlendFactorSourceAlpha;
  pipeColorAttachment0.destinationRGBBlendFactor = MTL::BlendFactorOneMinusSourceAlpha;

  pipeColorAttachment0.alphaBlendOperation = MTL::BlendOperationAdd;
  pipeColorAttachment0.sourceAlphaBlendFactor = MTL::BlendFactorOne;
  pipeColorAttachment0.destinationAlphaBlendFactor = MTL::BlendFactorZero;

  rPipeDesc.vertexFunction = vertexFunction;
  rPipeDesc.fragmentFunction = fragmentFunction;

  NS::Error *error;
  Pipeline = driver->newRenderPipelineStateWithDescriptor(rPipeDesc, &error);

  RDMTL::RenderPassDescriptor rPassDesc;
  rPassDesc.colorAttachments.resize(1);
  RDMTL::RenderPassColorAttachmentDescriptor &passColorAttachment0 = rPassDesc.colorAttachments[0];
  passColorAttachment0.texture = texture;
  passColorAttachment0.storeAction = storeAction;
  passColorAttachment0.loadAction = loadAction;
  passColorAttachment0.clearColor = MTL::ClearColor(clearR, clearG, clearB, clearA);

  MTLPassDesc = (MTL::RenderPassDescriptor *)(rPassDesc);
}

void MetalReplay::RenderPipePass::Destroy(WrappedMTLDevice *driver)
{
  Unwrap(Pipeline)->release();
  Pipeline = NULL;
  MTLPassDesc->release();
  MTLPassDesc = NULL;
}
