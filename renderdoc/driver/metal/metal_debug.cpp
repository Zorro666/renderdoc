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

#include "maths/formatpacking.h"
#include "metal_metal.h"
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
  RDCASSERT(m_checkerboardPipeline == id_MTLRenderPipelineState());

  NSString *debugShaders_objc = MTL::NewNSStringFromUTF8(debugShaders);

  NSError *error = NULL;
  id_MTLLibrary objcLibrary = m_pDriver->newLibraryWithSource(debugShaders_objc, NULL, &error);
  if(GetReal(objcLibrary) == NULL)
  {
    RDCERR("Error occurred when creating debug shaders library\n%s\n",
           MTL::Get_localizedDescription(error), MTL::Get_localizedRecoverySuggestion(error),
           MTL::Get_localizedFailureReason(error));
    return;
  }

  m_debugLibrary = GetWrapped(objcLibrary);
  id_MTLFunction blitVertex =
      m_debugLibrary->newFunctionWithName(MTL::NewNSStringFromUTF8("blit_vertex"));
  id_MTLFunction checkerboardFrag =
      m_debugLibrary->newFunctionWithName(MTL::NewNSStringFromUTF8("checkerboard_fragment"));

  MTLRenderPipelineDescriptor *descriptor = MTL::NewMTLRenderPipelineDescriptor();
  MTLRenderPipelineColorAttachmentDescriptor *colorAttachment0 =
      MTL_GET(descriptor, colorAttachment, 0);
  MTL_SET(colorAttachment0, pixelFormat, MTLPixelFormatBGRA8Unorm);
  MTL_SET(descriptor, vertexFunction, blitVertex);
  MTL_SET(descriptor, fragmentFunction, checkerboardFrag);

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

  id_MTLBuffer debugUBOBuffer = outw.m_debugUBOBuffer[0];
  CheckerUBOData *data = (CheckerUBOData *)GetWrapped(debugUBOBuffer)->contents();
  data->BorderWidth = 0.0f;
  data->RectPosition = Vec2f();
  data->RectSize = Vec2f();
  data->CheckerSquareDimension = 64.0f;
  data->InnerColor = Vec4f();

  data->PrimaryColor = light;
  data->SecondaryColor = dark;

  id_MTLTexture fb = outw.fb;
  MTLRenderPassDescriptor *renderPass = MTL::NewMTLRenderPassDescriptor();
  MTLRenderPassColorAttachmentDescriptor *colorAttachment0 = MTL_GET(renderPass, colorAttachment, 0);
  MTL_SET(colorAttachment0, texture, fb);
  MTL_SET(colorAttachment0, storeAction, MTLStoreActionStore);
  MTL_SET(colorAttachment0, loadAction, MTLLoadActionClear);
  MTL_SET(colorAttachment0, clearColor, MTLClearColorMake(1.0, 1.0, 1.0, 1.0));

  WrappedMTLRenderCommandEncoder *commandEncoder =
      GetWrapped(commandBuffer->renderCommandEncoderWithDescriptor(renderPass));

  commandEncoder->setRenderPipelineState(m_checkerboardPipeline);
  commandEncoder->setFragmentBuffer(debugUBOBuffer, 0, 0);
  commandEncoder->drawPrimitives(MTLPrimitiveTypeTriangleStrip, 0, 4, 1);
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

  id_MTLFunction blitVertex =
      debugLibrary->newFunctionWithName(MTL::NewNSStringFromUTF8("blit_vertex"));
  id_MTLFunction texdisplayFrag =
      debugLibrary->newFunctionWithName(MTL::NewNSStringFromUTF8("texdisplay_fragment"));

  MTLRenderPipelineDescriptor *descriptor = MTL::NewMTLRenderPipelineDescriptor();

  MTLRenderPipelineColorAttachmentDescriptor *colorAttachment0 =
      MTL_GET(descriptor, colorAttachment, 0);
  MTL_SET(colorAttachment0, pixelFormat, MTLPixelFormatBGRA8Unorm);
  MTL_SET(colorAttachment0, blendingEnabled, true);

  MTL_SET(colorAttachment0, rgbBlendOperation, MTLBlendOperationAdd);
  MTL_SET(colorAttachment0, sourceRGBBlendFactor, MTLBlendFactorSourceAlpha);
  MTL_SET(colorAttachment0, destinationRGBBlendFactor, MTLBlendFactorOneMinusSourceAlpha);

  MTL_SET(colorAttachment0, alphaBlendOperation, MTLBlendOperationAdd);
  MTL_SET(colorAttachment0, sourceAlphaBlendFactor, MTLBlendFactorOne);
  MTL_SET(colorAttachment0, destinationAlphaBlendFactor, MTLBlendFactorZero);

  MTL_SET(descriptor, vertexFunction, blitVertex);
  MTL_SET(descriptor, fragmentFunction, texdisplayFrag);

  Pipeline = driver->newRenderPipelineStateWithDescriptor(descriptor, NULL);
}
