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

#include "common/common.h"
#include "metal_resources.h"
#include "metal_stringise.h"
#include "metal_types.h"

enum class MetalChunk : uint32_t
{
  MTLCreateSystemDefaultDevice = (uint32_t)SystemChunk::FirstDriverChunk,
  mtlDevice_newBufferWithBytes,
  mtlDevice_newCommandQueue,
  mtlDevice_newDefaultLibrary,
  mtlDevice_newRenderPipelineStateWithDescriptor,
  mtlCommandBuffer_commit,
  mtlCommandBuffer_presentDrawable,
  mtlCommandBuffer_renderCommandEncoderWithDescriptor,
  mtlCommandQueue_commandBuffer,
  mtlLibrary_newFunctionWithName,
  mtlRenderCommandEncoder_drawPrimitives,
  mtlRenderCommandEncoder_endEncoding,
  mtlRenderCommandEncoder_setRenderPipelineState,
  mtlRenderCommandEncoder_setVertexBuffer,
  Max
};

DECLARE_REFLECTION_ENUM(MetalChunk);
