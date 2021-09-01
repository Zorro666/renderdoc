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
#include "metal_device.h"
#include "metal_resources.h"

class WrappedMTLRenderCommandEncoder : public WrappedMTLObject
{
public:
  WrappedMTLRenderCommandEncoder(id_MTLRenderCommandEncoder realMTLRenderCommandEncoder,
                                 ResourceId objId, WrappedMTLDevice *wrappedMTLDevice);
  WrappedMTLRenderCommandEncoder(WrappedMTLDevice *wrappedMTLDevice);

  void SetWrappedMTLCommandBuffer(WrappedMTLCommandBuffer *wrappedMTLCommandBuffer);

  DECLARE_WRAPPED_API(void, setRenderPipelineState, id_MTLRenderPipelineState pipelineState);
  DECLARE_WRAPPED_API(void, setVertexBuffer, id_MTLBuffer buffer, NSUInteger offset,
                      NSUInteger index);
  DECLARE_WRAPPED_API(void, setFragmentBuffer, id_MTLBuffer buffer, NSUInteger offset,
                      NSUInteger index);
  DECLARE_WRAPPED_API(void, setFragmentTexture, id_MTLTexture texture, NSUInteger index);
  DECLARE_WRAPPED_API(void, setViewport, MTLViewport &viewport);
  DECLARE_WRAPPED_API(void, drawPrimitives, MTLPrimitiveType primitiveType, NSUInteger vertexStart,
                      NSUInteger vertexCount, NSUInteger instanceCount);
  DECLARE_WRAPPED_API(void, endEncoding);

  enum
  {
    TypeEnum = eResRenderCommandEncoder
  };

  DECLARE_FUNCTION_SERIALISED(mtlRenderCommandEncoder_endEncoding,
                              WrappedMTLRenderCommandEncoder *encoder);
  DECLARE_FUNCTION_SERIALISED(mtlRenderCommandEncoder_setRenderPipelineState,
                              WrappedMTLRenderCommandEncoder *encoder,
                              WrappedMTLRenderPipelineState *pipelineState);
  DECLARE_FUNCTION_SERIALISED(mtlRenderCommandEncoder_setVertexBuffer,
                              WrappedMTLRenderCommandEncoder *encoder, WrappedMTLBuffer *buffer,
                              NSUInteger_objc offset, NSUInteger_objc index);
  DECLARE_FUNCTION_SERIALISED(mtlRenderCommandEncoder_setFragmentBuffer,
                              WrappedMTLRenderCommandEncoder *encoder, WrappedMTLBuffer *buffer,
                              NSUInteger_objc offset, NSUInteger_objc index);
  DECLARE_FUNCTION_SERIALISED(mtlRenderCommandEncoder_setFragmentTexture,
                              WrappedMTLRenderCommandEncoder *encoder, WrappedMTLTexture *texture,
                              NSUInteger_objc index);
  DECLARE_FUNCTION_SERIALISED(mtlRenderCommandEncoder_drawPrimitives,
                              WrappedMTLRenderCommandEncoder *encoder,
                              MTLPrimitiveType_objc primitiveType, NSUInteger_objc vertexStart,
                              NSUInteger_objc vertexCount, NSUInteger_objc instanceCount);

private:
  id_MTLRenderCommandEncoder CreateObjCWrappedMTLRenderCommandEncoder();
  WrappedMTLCommandBuffer *m_WrappedMTLCommandBuffer;
  CaptureState &m_State;
};
