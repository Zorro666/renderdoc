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
#include "metal_library.h"
#include "metal_render_command_encoder.h"
#include "metal_replay.h"
#include "metal_shaders.h"
#include "metal_shaders.metal"

void MetalReplay::CreateResources()
{
  InitDebugRenderer();
  m_TexRender.Init(m_pDriver);
}

void MetalReplay::InitDebugRenderer()
{
  RDCASSERT(m_debugLibrary == NULL);
  RDCASSERT(m_checkerboardPipeline == NULL);

  NS::String *tempString;
  NS::Error *error = NULL;
  tempString = NS::String::string(debugShaders, NS::UTF8StringEncoding);
  MTL::Library *objcLibrary = m_pDriver->newLibraryWithSource(tempString, NULL, &error);
  tempString->release();
  if(GetReal(objcLibrary) == NULL)
  {
    RDCERR("Error occurred when creating debug shaders library\n%s\n%s\n%s",
           error->localizedDescription(), error->localizedRecoverySuggestion(),
           error->localizedFailureReason());
    return;
  }

  m_debugLibrary = GetWrapped(objcLibrary);
  NS::String *nsString = NULL;

  nsString = NS::String::string("blit_vertex", NS::UTF8StringEncoding);
  MTL::Function *blitVertex = m_debugLibrary->newFunctionWithName(nsString);

  nsString = NS::String::string("checkerboard_fragment", NS::UTF8StringEncoding);
  MTL::Function *checkerboardFrag = m_debugLibrary->newFunctionWithName(nsString);

  MTL::RenderPipelineDescriptor *descriptor = MTL::RenderPipelineDescriptor::alloc();
  descriptor = descriptor->init();
  MTL::RenderPipelineColorAttachmentDescriptor *colorAttachment0 =
      descriptor->colorAttachments()->object(0);
  colorAttachment0->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  descriptor->setVertexFunction(blitVertex);
  descriptor->setFragmentFunction(checkerboardFrag);

  m_checkerboardPipeline = m_pDriver->newRenderPipelineStateWithDescriptor(descriptor, NULL);
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

  WrappedMTLCommandBuffer *commandBuffer = m_pDriver->GetNextCmd();
  if(commandBuffer == NULL)
    return;

  MTL::Buffer *debugUBOBuffer = outw.m_debugUBOBuffer[0];
  CheckerUBOData *data = (CheckerUBOData *)GetWrapped(debugUBOBuffer)->contents();
  data->BorderWidth = 0.0f;
  data->RectPosition = Vec2f();
  data->RectSize = Vec2f();
  data->CheckerSquareDimension = 64.0f;
  data->InnerColor = Vec4f();

  data->PrimaryColor = light;
  data->SecondaryColor = dark;

  MTL::Texture *fb = outw.fb;
  MTL::RenderPassDescriptor *renderPass = MTL::RenderPassDescriptor::alloc();
  renderPass = renderPass->init();
  MTL::RenderPassColorAttachmentDescriptor *colorAttachment0 =
      renderPass->colorAttachments()->object(0);
  colorAttachment0->setTexture(fb);
  colorAttachment0->setStoreAction(MTL::StoreActionStore);
  colorAttachment0->setLoadAction(MTL::LoadActionClear);
  colorAttachment0->setClearColor(MTL::ClearColor(1.0, 1.0, 1.0, 1.0));

  WrappedMTLRenderCommandEncoder *commandEncoder =
      GetWrapped(commandBuffer->renderCommandEncoderWithDescriptor(renderPass));

  commandEncoder->setRenderPipelineState(m_checkerboardPipeline);
  commandEncoder->setFragmentBuffer(debugUBOBuffer, 0, 0);
  commandEncoder->drawPrimitives(MTL::PrimitiveTypeLineStrip, 0, 4, 1);
  commandEncoder->endEncoding();
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
  MTL::Function *blitVertex = debugLibrary->newFunctionWithName(nsString);

  nsString = NS::String::string("texdisplay_fragment", NS::UTF8StringEncoding);
  MTL::Function *texdisplayFrag = debugLibrary->newFunctionWithName(nsString);

  MTL::RenderPipelineDescriptor *descriptor = MTL::RenderPipelineDescriptor::alloc();
  descriptor = descriptor->init();

  MTL::RenderPipelineColorAttachmentDescriptor *colorAttachment0 =
      descriptor->colorAttachments()->object(0);
  colorAttachment0->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  colorAttachment0->setBlendingEnabled(true);

  colorAttachment0->setRgbBlendOperation(MTL::BlendOperationAdd);
  colorAttachment0->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
  colorAttachment0->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

  colorAttachment0->setAlphaBlendOperation(MTL::BlendOperationAdd);
  colorAttachment0->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
  colorAttachment0->setDestinationAlphaBlendFactor(MTL::BlendFactorZero);

  descriptor->setVertexFunction(blitVertex);
  descriptor->setFragmentFunction(texdisplayFrag);

  Pipeline = driver->newRenderPipelineStateWithDescriptor(descriptor, NULL);
}
