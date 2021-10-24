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

#include "metal_common.h"

template <>
rdcstr DoStringise(const MetalChunk &el)
{
  RDCCOMPILE_ASSERT((uint32_t)MetalChunk::Max == 1014, "Chunks changed without updating names");

  BEGIN_ENUM_STRINGISE(MetalChunk)
  {
    STRINGISE_ENUM_CLASS(MTLCreateSystemDefaultDevice);
    STRINGISE_ENUM_CLASS(mtlDevice_newBufferWithBytes);
    STRINGISE_ENUM_CLASS(mtlDevice_newCommandQueue);
    STRINGISE_ENUM_CLASS(mtlDevice_newDefaultLibrary);
    STRINGISE_ENUM_CLASS(mtlDevice_newRenderPipelineStateWithDescriptor);
    STRINGISE_ENUM_CLASS(mtlCommandBuffer_commit);
    STRINGISE_ENUM_CLASS(mtlCommandBuffer_presentDrawable);
    STRINGISE_ENUM_CLASS(mtlCommandBuffer_renderCommandEncoderWithDescriptor);
    STRINGISE_ENUM_CLASS(mtlCommandQueue_commandBuffer);
    STRINGISE_ENUM_CLASS(mtlLibrary_newFunctionWithName);
    STRINGISE_ENUM_CLASS(mtlRenderCommandEncoder_drawPrimitives);
    STRINGISE_ENUM_CLASS(mtlRenderCommandEncoder_endEncoding);
    STRINGISE_ENUM_CLASS(mtlRenderCommandEncoder_setRenderPipelineState);
    STRINGISE_ENUM_CLASS(mtlRenderCommandEncoder_setVertexBuffer);
    STRINGISE_ENUM_CLASS_NAMED(Max, "Max Chunk");
  }
  END_ENUM_STRINGISE()
}
