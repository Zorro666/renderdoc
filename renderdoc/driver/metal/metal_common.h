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
  MTLBuffer_contents,
  MTLDevice_newBuffer,
  MTLDevice_newBufferWithLength,
  MTLDevice_newCommandQueue,
  MTLDevice_newDefaultLibrary,
  MTLDevice_newLibraryWithSource,
  MTLDevice_newRenderPipelineStateWithDescriptor,
  MTLDevice_newTextureWithDescriptor,
  MTLCommandBuffer_commit,
  MTLCommandBuffer_presentDrawable,
  MTLCommandBuffer_renderCommandEncoderWithDescriptor,
  MTLCommandQueue_commandBuffer,
  MTLLibrary_newFunctionWithName,
  MTLRenderCommandEncoder_drawPrimitives,
  MTLRenderCommandEncoder_endEncoding,
  MTLRenderCommandEncoder_setRenderPipelineState,
  MTLRenderCommandEncoder_setFragmentBuffer,
  MTLRenderCommandEncoder_setVertexBuffer,
  MTLRenderCommandEncoder_setFragmentTexture,
  Max
};

DECLARE_REFLECTION_ENUM(MetalChunk);

#define GETSET_PROPERTY(STRUCT, TYPE, PROPERTY) \
  TYPE Get_##PROPERTY(STRUCT *pStruct);         \
  void Set_##PROPERTY(STRUCT *pStruct, TYPE PROPERTY);

#define GETSET_PROPERTY_INHERITED(STRUCT, BASE, TYPE, PROPERTY)                            \
  inline TYPE Get_##PROPERTY(STRUCT *pStruct) { return Get_##PROPERTY((BASE *)pStruct); }; \
  inline void Set_##PROPERTY(STRUCT *pStruct, TYPE PROPERTY)                               \
  {                                                                                        \
    Set_##PROPERTY((BASE *)pStruct, PROPERTY);                                             \
  };

#define MTL_GET(STRUCT, PROPERTY, ...) MTL::Get_##PROPERTY(STRUCT, ##__VA_ARGS__)
#define MTL_SET(STRUCT, PROPERTY, VALUE, ...) MTL::Set_##PROPERTY(STRUCT, VALUE, ##__VA_ARGS__)

// similar to RDCUNIMPLEMENTED but without the debugbreak
#define METAL_NOT_IMPLEMENTED(...)                                            \
  do                                                                          \
  {                                                                           \
    RDCWARN("Metal '%s' not implemented -" __VA_ARGS__, __PRETTY_FUNCTION__); \
  } while((void)0, 0)

// similar to RDCUNIMPLEMENTED but for things that are hit often so we don't want to fire the
// debugbreak.
#define METAL_NOT_IMPLEMENTED_ONCE(...)                                           \
  do                                                                              \
  {                                                                               \
    static bool msgprinted = false;                                               \
    if(!msgprinted)                                                               \
      RDCDEBUG("Metal '%s' not implemented - " __VA_ARGS__, __PRETTY_FUNCTION__); \
    msgprinted = true;                                                            \
  } while((void)0, 0)

BlendMultiplier MakeBlendMultiplier(MTLBlendFactor blend);
BlendOperation MakeBlendOp(MTLBlendOperation op);
byte MakeWriteMask(MTLColorWriteMask mask);
ResourceFormat MakeResourceFormat(MTLPixelFormat format);
uint32_t GetByteSize(uint32_t width, uint32_t height, uint32_t depth, MTLPixelFormat format,
                     uint32_t mip);
