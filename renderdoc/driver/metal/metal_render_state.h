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

#include "metal_common.h"

struct MetalStatePipeline
{
  void Init();
  ResourceId pipeline;

  struct BoundBuffer
  {
    ResourceId buffer;
    NS::UInteger offset;
    NS::UInteger index;
  };

  struct BoundTexture
  {
    ResourceId texture;
    NS::UInteger index;
  };

  rdcarray<BoundBuffer> vertexBuffers;
  rdcarray<BoundBuffer> fragmentBuffers;
  rdcarray<BoundTexture> fragmentTextures;
};

struct MetalRenderState
{
  enum PipelineBinding
  {
    BindNone = 0,
    BindRender = 1 << 0,
    BindBlit = 1 << 2,
    BindCompute = 1 << 3,
  };

  MetalRenderState();
  void Init();

  void BeginBlitPass(WrappedMTLDevice *device, WrappedMTLCommandBuffer *cmdBuffer);
  void EndBlitPass();

  void BeginRenderPassAndApplyState(WrappedMTLDevice *device, WrappedMTLCommandBuffer *cmdBuffer,
                                    PipelineBinding binding);
  void BindPipeline(WrappedMTLDevice *device, WrappedMTLCommandBuffer *cmdBuffer,
                    PipelineBinding binding);
  void EndRenderPass();

  WrappedMTLRenderCommandEncoder *renderCommandEncoder = NULL;
  WrappedMTLBlitCommandEncoder *blitCommandEncoder = NULL;

  MetalStatePipeline graphics;
  // MTLRenderCommandEncoder
  ResourceId renderPass;
  // MTLBlitCommandEncoder
  ResourceId blitPass;
  // setViewport, setViewports, setScissor, setScissors
  rdcarray<MTL::Viewport> viewports;
  rdcarray<MTL::ScissorRect> scissors;
  // setBlendColor
  rdcfixedarray<float, 4> blendColor;
  // setDepthClipMode
  MTL::DepthClipMode depthClipMode;
  // setFrontFacingWinding
  MTL::Winding frontFacingWinding;
  // setTriangleFillMode
  MTL::TriangleFillMode fillMode;
  // setCullMode
  MTL::CullMode cullMode;
  // setDepthStencilState
  ResourceId depthStencilState;
  //  MTLDepthStencilDescriptor
  //  MTLCompareFunction depthCompareFunction;
  //  bool depthWriteEnabled;
  //  MTLStencilDescriptor frontFaceStencil;
  //  MTLStencilDescriptor backFaceStencil;
};
