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

#include <stdint.h>
#include <string.h>
#include "serialise/serialiser.h"
#include "metal_common.h"

class MetalResourceManager;
class MetalReplay;

// TODO: where does Metal specify it is max 8 color attachments
const uint32_t MAX_RENDER_PASS_COLOR_ATTACHMENTS = 8;

#define METALCPP_WRAPPED_PROTOCOLS(FUNC) \
  FUNC(Buffer);                          \
  FUNC(CommandBuffer);                   \
  FUNC(CommandQueue);                    \
  FUNC(Device);                          \
  FUNC(Function);                        \
  FUNC(Library);                         \
  FUNC(RenderCommandEncoder);            \
  FUNC(RenderPipelineState);             \
  FUNC(Texture);

#define DECLARE_OBJC_HELPERS(CPPTYPE)                                \
  class WrappedMTL##CPPTYPE;                                         \
  extern WrappedMTL##CPPTYPE *GetWrapped(MTL::CPPTYPE *objCWrapped); \
  extern MTL::CPPTYPE *GetReal(MTL::CPPTYPE *objCWrapped);           \
  extern bool IsObjCWrapped(MTL::CPPTYPE *objCWrapped);              \
  extern ResourceId GetId(MTL::CPPTYPE *objCWrapped);                \
  extern MTL::CPPTYPE *AllocateObjCWrapper(WrappedMTL##CPPTYPE *wrapped);

METALCPP_WRAPPED_PROTOCOLS(DECLARE_OBJC_HELPERS)
#undef DECLARE_OBJC_HELPERS

#define MTL_DECLARE_REFLECTION_OBJECT(TYPE)      \
  template <>                                    \
  inline rdcliteral TypeName<MTL::TYPE *>()      \
  {                                              \
    return STRING_LITERAL(STRINGIZE(MTL##TYPE)); \
  }                                              \
  template <class SerialiserType>                \
  void DoSerialise(SerialiserType &ser, MTL::TYPE *&el);

#define MTL_DECLARE_REFLECTION_TYPE(TYPE)        \
  template <>                                    \
  inline rdcliteral TypeName<MTL::TYPE>()        \
  {                                              \
    return STRING_LITERAL(STRINGIZE(MTL##TYPE)); \
  }                                              \
  template <class SerialiserType>                \
  void DoSerialise(SerialiserType &ser, MTL::TYPE &el);

MTL_DECLARE_REFLECTION_OBJECT(TextureDescriptor)
MTL_DECLARE_REFLECTION_OBJECT(RenderPipelineDescriptor);
MTL_DECLARE_REFLECTION_OBJECT(RenderPipelineColorAttachmentDescriptor);
MTL_DECLARE_REFLECTION_OBJECT(RenderPassDescriptor);
MTL_DECLARE_REFLECTION_OBJECT(RenderPassAttachmentDescriptor);
MTL_DECLARE_REFLECTION_OBJECT(RenderPassColorAttachmentDescriptor);
MTL_DECLARE_REFLECTION_OBJECT(RenderPassDepthAttachmentDescriptor);
MTL_DECLARE_REFLECTION_OBJECT(RenderPassStencilAttachmentDescriptor);

MTL_DECLARE_REFLECTION_TYPE(PrimitiveType);
MTL_DECLARE_REFLECTION_TYPE(PixelFormat);
MTL_DECLARE_REFLECTION_TYPE(TextureType);
MTL_DECLARE_REFLECTION_TYPE(PrimitiveTopologyClass);
MTL_DECLARE_REFLECTION_TYPE(ResourceOptions);
MTL_DECLARE_REFLECTION_TYPE(CPUCacheMode);
MTL_DECLARE_REFLECTION_TYPE(StorageMode);
MTL_DECLARE_REFLECTION_TYPE(HazardTrackingMode);
MTL_DECLARE_REFLECTION_TYPE(TextureUsage);
MTL_DECLARE_REFLECTION_TYPE(TextureSwizzleChannels);
MTL_DECLARE_REFLECTION_TYPE(TextureSwizzle);
MTL_DECLARE_REFLECTION_TYPE(StoreActionOptions);
MTL_DECLARE_REFLECTION_TYPE(ColorWriteMask);
MTL_DECLARE_REFLECTION_TYPE(LoadAction);
MTL_DECLARE_REFLECTION_TYPE(StoreAction);
MTL_DECLARE_REFLECTION_TYPE(BlendOperation);
MTL_DECLARE_REFLECTION_TYPE(BlendFactor);
MTL_DECLARE_REFLECTION_TYPE(Winding);
MTL_DECLARE_REFLECTION_TYPE(DepthClipMode);
MTL_DECLARE_REFLECTION_TYPE(TriangleFillMode);
MTL_DECLARE_REFLECTION_TYPE(CullMode);
MTL_DECLARE_REFLECTION_TYPE(TessellationFactorFormat);
MTL_DECLARE_REFLECTION_TYPE(TessellationControlPointIndexType)
MTL_DECLARE_REFLECTION_TYPE(TessellationFactorStepFunction);
MTL_DECLARE_REFLECTION_TYPE(TessellationPartitionMode);
MTL_DECLARE_REFLECTION_TYPE(ClearColor);
MTL_DECLARE_REFLECTION_TYPE(Size);

template <>
inline rdcliteral TypeName<NS::String *>()
{
  return "NSString"_lit;
}
template <class SerialiserType>
void DoSerialise(SerialiserType &ser, NS::String *&el);

void MTLFixupForMetalDriverAssert();
CA::MetalDrawable *MTLGetNextDrawable(void *layer);
