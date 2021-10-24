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

#include "metal_command_queue.h"
#include "metal_common.h"
#include "metal_device.h"
#include "metal_resources.h"

class WrappedMTLCommandBuffer : public WrappedMTLObject
{
public:
  WrappedMTLCommandBuffer(MTL::CommandBuffer *realMTLCommandBuffer, ResourceId objId,
                          WrappedMTLDevice *wrappedMTLDevice);
  WrappedMTLCommandBuffer(WrappedMTLDevice *wrappedMTLDevice);

  void SetWrappedMTLCommandQueue(WrappedMTLCommandQueue *wrappedMTLCommandQueue);

  MTL::CommandQueue *GetObjCWrappedMTLCommandQueue();

  void CopyTexture(MTL::Texture *source, MTL::Texture *destination);

  DECLARE_WRAPPED_API(MTL::RenderCommandEncoder *, renderCommandEncoderWithDescriptor,
                      MTL::RenderPassDescriptor *);
  DECLARE_WRAPPED_API(void, presentDrawable, MTL::Drawable *drawable);
  DECLARE_WRAPPED_API(void, commit);
  DECLARE_WRAPPED_API(void, enqueue);
  DECLARE_WRAPPED_API(void, waitUntilCompleted);
  DECLARE_WRAPPED_API(void, label, const char *label);

  DECLARE_FUNCTION_SERIALISED(mtlCommandBuffer_commit, WrappedMTLCommandBuffer *buffer);
  DECLARE_FUNCTION_SERIALISED(mtlCommandBuffer_renderCommandEncoderWithDescriptor,
                              WrappedMTLCommandBuffer *buffer,
                              WrappedMTLRenderCommandEncoder *encoder,
                              MTL::RenderPassDescriptor *descriptor);
  DECLARE_FUNCTION_SERIALISED(mtlCommandBuffer_presentDrawable, WrappedMTLCommandBuffer *buffer,
                              MTL::Drawable *drawable);

  enum
  {
    TypeEnum = eResCommandBuffer
  };

private:
  WrappedMTLCommandQueue *m_WrappedMTLCommandQueue;
};

inline MTL::CommandQueue *WrappedMTLCommandBuffer::GetObjCWrappedMTLCommandQueue()
{
  return UnwrapObjC<MTL::CommandQueue *>(m_WrappedMTLCommandQueue);
}
