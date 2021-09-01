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

#pragma once

#include "metal_common.h"

struct MetalStatePipeline
{
  void Init();
  ResourceId pipeline;
};

struct MetalRenderState
{
  enum PipelineBinding
  {
    BindNone = 0x0,
    BindGraphics = 0x1,
    BindCompute = 0x2,
    BindInitial = 0x4,
  };

  MetalRenderState();
  void Init();

  void BeginRenderPassAndApplyState(WrappedMTLDevice *device, WrappedMTLCommandBuffer *cmd,
                                    PipelineBinding binding);
  void BindPipeline(WrappedMTLDevice *device, WrappedMTLCommandBuffer *cmd, PipelineBinding binding,
                    bool subpass0);
  void EndRenderPass(WrappedMTLCommandBuffer *cmd);

  MetalStatePipeline graphics;
  // MTLRenderCommandEncoder
  ResourceId renderPass;
  // setViewport, setViewports, setScissor, setScissors
  rdcarray<MTLViewport> viewports;
  rdcarray<MTLScissorRect> scissors;
  // setBlendColorRed
  rdcfixedarray<float, 4> blendColor;
  // setDepthClipMode
  MTLDepthClipMode depthClipMode;
  // setFrontFacingWinding
  MTLWinding frontFacingWinding;
  // setTriangleFillMode
  MTLTriangleFillMode fillMode;
  // setCullMode
  MTLCullMode cullMode;
  // setDepthStencilState
  ResourceId depthStencilState;
  //  MTLDepthStencilDescriptor
  //  MTLCompareFunction depthCompareFunction;
  //  bool depthWriteEnabled;
  //  MTLStencilDescriptor frontFaceStencil;
  //  MTLStencilDescriptor backFaceStencil;
};
