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

#include "metal_command_queue.h"
#include "metal_common.h"
#include "metal_device.h"
#include "metal_resources.h"

class WrappedMTLCommandBuffer : public WrappedMTLObject
{
public:
  WrappedMTLCommandBuffer(id_MTLCommandBuffer realMTLCommandBuffer, ResourceId objId,
                          WrappedMTLDevice *wrappedMTLDevice);
  WrappedMTLCommandBuffer(WrappedMTLDevice *wrappedMTLDevice);

  void SetWrappedMTLCommandQueue(WrappedMTLCommandQueue *wrappedMTLCommandQueue);

  id_MTLCommandBuffer GetObjCWrappedMTLCommandBuffer();
  id_MTLCommandQueue GetObjCWrappedMTLCommandQueue();

  DECLARE_WRAPPED_API(id_MTLRenderCommandEncoder, renderCommandEncoderWithDescriptor,
                      MTLRenderPassDescriptor *);
  DECLARE_WRAPPED_API(void, presentDrawable, id_MTLDrawable drawable);
  DECLARE_WRAPPED_API(void, commit);

  DECLARE_FUNCTION_SERIALISED(mtlCommandBuffer_commit, WrappedMTLCommandBuffer *buffer);
  DECLARE_FUNCTION_SERIALISED(mtlCommandBuffer_renderCommandEncoderWithDescriptor,
                              WrappedMTLCommandBuffer *buffer,
                              WrappedMTLRenderCommandEncoder *encoder);
  DECLARE_FUNCTION_SERIALISED(mtlCommandBuffer_presentDrawable, WrappedMTLCommandBuffer *buffer,
                              id_MTLDrawable drawable);

  enum
  {
    TypeEnum = eResCommandBuffer
  };

private:
  id_MTLCommandBuffer CreateObjCWrappedMTLCommandBuffer();

  WrappedMTLCommandQueue *m_WrappedMTLCommandQueue;
  id_MTLCommandBuffer m_ObjCWrappedMTLCommandBuffer;
  CaptureState &m_State;
};

inline id_MTLCommandBuffer WrappedMTLCommandBuffer::GetObjCWrappedMTLCommandBuffer()
{
  return m_ObjCWrappedMTLCommandBuffer;
}

inline id_MTLCommandQueue WrappedMTLCommandBuffer::GetObjCWrappedMTLCommandQueue()
{
  return m_WrappedMTLCommandQueue->GetObjCWrappedMTLCommandQueue();
}
