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
#include "metal_device.h"
#include "metal_render_command_encoder.h"
#include "metal_replay.h"
#include "metal_texture.h"

bool MetalReplay::RenderTexture(TextureDisplay cfg)
{
  auto it = m_OutputWindows.find(m_ActiveWinID);
  if(it == m_OutputWindows.end())
  {
    RDCERR("output window not bound");
    return false;
  }

  OutputWindow &outw = it->second;

  // if the swapchain failed to create, do nothing. We will try to recreate it
  // again in CheckResizeOutputWindow (once per render 'frame')
  if(outw.NoOutput())
    return false;

  const MetalTextureState *textureState = m_pDriver->FindConstTextureState(cfg.resourceId);
  if(!textureState)
  {
    RDCWARN("Could not find texture info for texture %s", ToStr(cfg.resourceId).c_str());
    return false;
  }

  return RenderTextureInternal(cfg, *textureState, m_TexRender.Pipeline, outw.MTLPassDesc,
                               outw.CBuffers[OutputWindow::BUFFER_RENDERTEXTURE],
                               eTexDisplay_MipShift | eTexDisplay_BlendAlpha);
}

bool MetalReplay::RenderTextureInternal(TextureDisplay cfg, const MetalTextureState &textureState,
                                        WrappedMTLRenderPipelineState *rPipeline,
                                        MTL::RenderPassDescriptor *rPassDesc,
                                        WrappedMTLBuffer *debugCBuffer, int flags)
{
  if(rPipeline == NULL)
    return false;

  if(rPassDesc == NULL)
    return false;

  const bool mipShift = (flags & eTexDisplay_MipShift) != 0;
  const MetalTextureInfo &iminfo = textureState.textureInfo;

  int displayformat = 0;

  char *bufferStart = (char *)debugCBuffer->contents();

  TexDisplayUBOData *data = (TexDisplayUBOData *)bufferStart;

  float x = cfg.xOffset;
  float y = cfg.yOffset;

  data->Position.x = x;
  data->Position.y = y;
  data->HDRMul = cfg.hdrMultiplier;
  data->DecodeYUV = cfg.decodeYUV ? 1 : 0;

  Vec4u YUVDownsampleRate = {};
  Vec4u YUVAChannels = {};

  data->YUVDownsampleRate = YUVDownsampleRate;
  data->YUVAChannels = YUVAChannels;

  int32_t tex_x = (int32_t)iminfo.extent.width;
  int32_t tex_y = (int32_t)iminfo.extent.height;
  int32_t tex_z = (int32_t)iminfo.extent.depth;

  if(cfg.scale <= 0.0f)
  {
    float xscale = float(m_DebugWidth) / float(tex_x);
    float yscale = float(m_DebugHeight) / float(tex_y);

    // update cfg.scale for use below
    float scale = cfg.scale = RDCMIN(xscale, yscale);

    if(yscale > xscale)
    {
      data->Position.x = 0;
      data->Position.y = (float(m_DebugHeight) - (tex_y * scale)) * 0.5f;
    }
    else
    {
      data->Position.y = 0;
      data->Position.x = (float(m_DebugWidth) - (tex_x * scale)) * 0.5f;
    }
  }

  data->Channels.x = cfg.red ? 1.0f : 0.0f;
  data->Channels.y = cfg.green ? 1.0f : 0.0f;
  data->Channels.z = cfg.blue ? 1.0f : 0.0f;
  data->Channels.w = cfg.alpha ? 1.0f : 0.0f;

  if(cfg.rangeMax <= cfg.rangeMin)
    cfg.rangeMax += 0.00001f;

  data->RangeMinimum = cfg.rangeMin;
  data->InverseRangeSize = 1.0f / (cfg.rangeMax - cfg.rangeMin);

  data->FlipY = cfg.flipY ? 1 : 0;

  data->MipLevel = (int)cfg.subresource.mip;
  data->Slice = 0;

  if(iminfo.type != MTL::TextureType3D)
  {
    // uint32_t numSlices = RDCMAX((uint32_t)iminfo.arrayLayers, 1U);
    uint32_t numSlices = 1;

    uint32_t sliceFace = RDCCLAMP(cfg.subresource.slice, 0U, numSlices - 1);
    data->Slice = (float)sliceFace + 0.001f;
  }
  else
  {
    float slice = (float)RDCCLAMP(cfg.subresource.slice, 0U, (uint32_t)(iminfo.extent.depth - 1));

    // when sampling linearly, we need to add half a pixel to ensure we only sample the desired
    // slice
    if(cfg.subresource.mip == 0 && cfg.scale < 1.0f &&
       (displayformat & (TEXDISPLAY_UINT_TEX | TEXDISPLAY_SINT_TEX)) == 0)
      slice += 0.5f;
    else
      slice += 0.001f;

    data->Slice = slice;
  }

  data->TextureResolutionPS.x = float(RDCMAX(1, tex_x >> cfg.subresource.mip));
  data->TextureResolutionPS.y = float(RDCMAX(1, tex_y >> cfg.subresource.mip));
  data->TextureResolutionPS.z = float(RDCMAX(1, tex_z >> cfg.subresource.mip));

  if(mipShift)
    data->MipShift = float(1 << cfg.subresource.mip);
  else
    data->MipShift = 1.0f;

  data->Scale = cfg.scale;

  int sampleIdx = (int)RDCCLAMP(cfg.subresource.sample, 0U, (uint32_t)iminfo.sampleCount);

  if(cfg.subresource.sample == ~0U)
    sampleIdx = -iminfo.sampleCount;

  data->SampleIdx = sampleIdx;

  data->OutputRes.x = (float)m_DebugWidth;
  data->OutputRes.y = (float)m_DebugHeight;

  int textype = 0;

  if(iminfo.type == MTL::TextureType1D)
  {
    textype = RESTYPE_TEX1D;
  }
  else if(iminfo.type == MTL::TextureType3D)
  {
    textype = RESTYPE_TEX3D;
  }
  else if(iminfo.type == MTL::TextureType2D)
  {
    textype = RESTYPE_TEX2D;
    if(iminfo.sampleCount != 1)
      textype = RESTYPE_TEX2DMS;
  }

  displayformat |= textype;

  //  descSetBinding += textype;

  //  if(!IsSRGBFormat(texviews.castedFormat) && cfg.linearDisplayAsGamma)
  //    displayformat |= TEXDISPLAY_GAMMA_CURVE;

  if(cfg.overlay == DebugOverlay::NaN)
    displayformat |= TEXDISPLAY_NANS;

  if(cfg.overlay == DebugOverlay::Clipping)
    displayformat |= TEXDISPLAY_CLIPPING;

  data->OutputDisplayFormat = displayformat;

  data->RawOutput = cfg.rawOutput ? 1 : 0;

  HeatmapData heatmapData = {};
  if(cfg.overlay == DebugOverlay::QuadOverdrawDraw || cfg.overlay == DebugOverlay::QuadOverdrawPass)
  {
    heatmapData.HeatmapMode = HEATMAP_LINEAR;
  }
  else if(cfg.overlay == DebugOverlay::TriangleSizeDraw ||
          cfg.overlay == DebugOverlay::TriangleSizePass)
  {
    heatmapData.HeatmapMode = HEATMAP_TRISIZE;
  }

  if(heatmapData.HeatmapMode)
  {
    memcpy(heatmapData.ColorRamp, colorRamp, sizeof(colorRamp));

    RDCCOMPILE_ASSERT(sizeof(heatmapData.ColorRamp) == sizeof(colorRamp),
                      "C++ color ramp array is not the same size as the shader array");
  }

  HeatmapData *ptr = (HeatmapData *)(bufferStart + sizeof(TexDisplayUBOData));
  memcpy(ptr, &heatmapData, sizeof(HeatmapData));

  WrappedMTLCommandBuffer *commandBuffer = m_pDriver->GetNextCommandBuffer();
  if(commandBuffer == NULL)
    return false;

  MTL::RenderCommandEncoder *mtlCommandEncoder =
      Unwrap(commandBuffer)->renderCommandEncoder(rPassDesc);

  WrappedMTLTexture *source = textureState.wrappedTexture;

  mtlCommandEncoder->setRenderPipelineState(Unwrap(rPipeline));
  mtlCommandEncoder->setFragmentBuffer(Unwrap(debugCBuffer), 0, 0);
  mtlCommandEncoder->setFragmentBuffer(Unwrap(debugCBuffer), sizeof(TexDisplayUBOData), 1);
  mtlCommandEncoder->setFragmentTexture(Unwrap(source), 0);
  mtlCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, 0, 4, 1);
  mtlCommandEncoder->endEncoding();
  return true;
}
