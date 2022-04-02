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

#include "metal_command_queue.h"
#include "core/core.h"
#include "metal_command_buffer.h"
#include "metal_device.h"
#include "metal_manager.h"

WrappedMTLCommandQueue::WrappedMTLCommandQueue(MTL::CommandQueue *realMTLCommandQueue,
                                               ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandQueue, objId, wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  AllocateObjCBridge(this);
}

WrappedMTLCommandQueue::WrappedMTLCommandQueue(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  m_ObjcBridge = NULL;
}

template <typename SerialiserType>
bool WrappedMTLCommandQueue::Serialise_commandBuffer(SerialiserType &ser,
                                                     WrappedMTLCommandBuffer *buffer)
{
  SERIALISE_ELEMENT_LOCAL(CommandQueue, this);
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //
    //      ActionDescription action;
    //      action.flags |= ActionFlags::NoFlags;
    //      action.flags |= ActionFlags::CommandBufferBoundary;
    //
    //      AddAction(action);
    //    }
    // TODO: implement RD MTL replay
    //    MTL::CommandBuffer *realMTLCommandBuffer = Unwrap(CommandQueue)->commandBuffer();
    //
    //    WrappedMTLCommandBuffer *wrappedMTLCommandBuffer;
    //    GetResourceManager()->WrapResource(realMTLCommandBuffer, wrappedMTLCommandBuffer);
    //    wrappedMTLCommandBuffer->SetCommandQueue(CommandQueue);
    WrappedMTLCommandBuffer *wrappedMTLCommandBuffer = m_Device->GetNextCommandBuffer();

    if(GetResourceManager()->HasLiveResource(CommandBuffer))
    {
      // TODO: we are leaking the original WrappedMTLCommandBuffer
      GetResourceManager()->EraseLiveResource(CommandBuffer);
    }
    GetResourceManager()->AddLiveResource(CommandBuffer, wrappedMTLCommandBuffer);

    m_Device->AddResource(CommandBuffer, ResourceType::CommandBuffer, "Command Buffer");
    m_Device->DerivedResource(CommandQueue, CommandBuffer);
  }
  return true;
}

WrappedMTLCommandBuffer *WrappedMTLCommandQueue::commandBuffer()
{
  MTL::CommandBuffer *realMTLCommandBuffer;
  SERIALISE_TIME_CALL(realMTLCommandBuffer = Unwrap(this)->commandBuffer());
  WrappedMTLCommandBuffer *wrappedMTLCommandBuffer;
  ResourceId id = GetResourceManager()->WrapResource(realMTLCommandBuffer, wrappedMTLCommandBuffer);
  wrappedMTLCommandBuffer->SetCommandQueue(this);

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandQueue_commandBuffer);
      Serialise_commandBuffer(ser, wrappedMTLCommandBuffer);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord =
        GetResourceManager()->AddResourceRecord(wrappedMTLCommandBuffer);
    bufferRecord->AddChunk(chunk);
    bufferRecord->cmdInfo = new MetalCmdBufferRecordingInfo(this);
  }
  else
  {
    // TODO: implement RD MTL replay
    GetResourceManager()->AddLiveResource(id, wrappedMTLCommandBuffer);
  }

  return wrappedMTLCommandBuffer;
}

INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLCommandQueue, WrappedMTLCommandBuffer *,
                                            commandBuffer);
