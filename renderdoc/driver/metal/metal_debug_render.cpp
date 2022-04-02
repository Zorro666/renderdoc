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

void MetalReplay::CreateResources()
{
  InitDebugRenderer();
  m_TexRender.Init(m_pDriver);
}

void MetalReplay::InitDebugRenderer()
{
  RDCASSERT(m_debugLibrary == NULL);
  RDCASSERT(m_checkerboardPipeline == NULL);

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

  m_checkerboardPipeline = m_pDriver->newRenderPipelineStateWithDescriptor(descriptor, NULL);
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

  WrappedMTLBuffer *debugUBOBuffer = outw.m_debugUBOBuffer[0];
  CheckerUBOData *data = (CheckerUBOData *)debugUBOBuffer->contents();
  data->BorderWidth = 0.0f;
  data->RectPosition = Vec2f();
  data->RectSize = Vec2f();
  data->CheckerSquareDimension = 64.0f;
  data->InnerColor = Vec4f();

  data->PrimaryColor = light;
  data->SecondaryColor = dark;

  WrappedMTLTexture *fb = outw.fb;
  MTL::RenderPassDescriptor *mtlRenderPass = MTL::RenderPassDescriptor::alloc();
  mtlRenderPass = mtlRenderPass->init();
  MTL::RenderPassColorAttachmentDescriptor *mtlColorAttachment0 =
      mtlRenderPass->colorAttachments()->object(0);
  mtlColorAttachment0->setTexture(Unwrap(fb));
  mtlColorAttachment0->setStoreAction(MTL::StoreActionStore);
  mtlColorAttachment0->setLoadAction(MTL::LoadActionClear);
  mtlColorAttachment0->setClearColor(MTL::ClearColor(1.0, 1.0, 1.0, 1.0));

  MTL::RenderCommandEncoder *encoder = Unwrap(commandBuffer)->renderCommandEncoder(mtlRenderPass);

  encoder->setRenderPipelineState(Unwrap(m_checkerboardPipeline));
  encoder->setFragmentBuffer(Unwrap(debugUBOBuffer), 0, 0);
  encoder->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, 0, 4, 1);
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
