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
#include "core/settings.h"
#include "metal_command_buffer.h"
#include "metal_manager.h"

RDOC_EXTERN_CONFIG(bool, Metal_Debug_VerboseCommandRecording);

WrappedMTLCommandQueue::WrappedMTLCommandQueue(MTL::CommandQueue *realMTLCommandQueue,
                                               ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandQueue, objId, wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = AllocateObjCWrapper(this);
}

WrappedMTLCommandQueue::WrappedMTLCommandQueue(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = NULL;
}

MTL::CommandBuffer *WrappedMTLCommandQueue::commandBuffer()
{
  MTL::CommandBuffer *realMTLCommandBuffer;
  SERIALISE_TIME_CALL(realMTLCommandBuffer = real_commandBuffer());
  MetalResourceManager::UnwrapHelper<MTL::CommandBuffer *>::Outer *wrappedMTLCommandBuffer;
  ResourceId id = GetResourceManager()->WrapResource(realMTLCommandBuffer, wrappedMTLCommandBuffer);
  wrappedMTLCommandBuffer->SetWrappedMTLCommandQueue(this);

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandQueue_commandBuffer);
      Serialise_mtlCommandQueue_commandBuffer(ser, this, wrappedMTLCommandBuffer);
      chunk = scope.Get();
    }
    MetalResourceRecord *queueRecord = GetRecord(this);
    GetResourceManager()->MarkResourceFrameReferenced(GetResID(this), eFrameRef_Read);
    MetalResourceRecord *bufferRecord =
        GetResourceManager()->AddResourceRecord(wrappedMTLCommandBuffer);
    bufferRecord->AddChunk(chunk);
    if(Metal_Debug_VerboseCommandRecording())
    {
      RDCLOG("Allocate CommandBuffer %s CommandQueue %s",
             ToStr(bufferRecord->GetResourceID()).c_str(),
             ToStr(queueRecord->GetResourceID()).c_str());
    }

    bufferRecord->cmdInfo = new MetalResources::CmdBufferRecordingInfo();
    bufferRecord->cmdInfo->allocPool = new ChunkPagePool(32 * 1024);
    bufferRecord->cmdInfo->alloc = new ChunkAllocator(*bufferRecord->cmdInfo->allocPool);
    bufferRecord->cmdInfo->queue = this;
    bufferRecord->cmdInfo->device = m_WrappedMTLDevice;
    bufferRecord->cmdInfo->present = false;
    bufferRecord->cmdInfo->isEncoding = false;

    bufferRecord->AddParent(queueRecord);
  }
  else
  {
    // TODO: implement RD MTL replay
    GetResourceManager()->AddLiveResource(id, wrappedMTLCommandBuffer);
  }

  return UnwrapObjC<MTL::CommandBuffer *>(wrappedMTLCommandBuffer);
}

template <typename SerialiserType>
bool WrappedMTLCommandQueue::Serialise_mtlCommandQueue_commandBuffer(SerialiserType &ser,
                                                                     WrappedMTLCommandQueue *queue,
                                                                     WrappedMTLCommandBuffer *buffer)
{
  SERIALISE_ELEMENT_LOCAL(CommandQueue, GetResID(queue)).TypedAs("MTLCommandQueue"_lit);
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    WrappedMTLCommandQueue *queue =
        (WrappedMTLCommandQueue *)GetResourceManager()->GetLiveResource(CommandQueue);

    MTL::CommandBuffer *realMTLCommandBuffer = queue->real_commandBuffer();

    MetalResourceManager::UnwrapHelper<MTL::CommandBuffer *>::Outer *wrappedMTLCommandBuffer;
    GetResourceManager()->WrapResource(realMTLCommandBuffer, wrappedMTLCommandBuffer);
    wrappedMTLCommandBuffer->SetWrappedMTLCommandQueue(queue);

    if(GetResourceManager()->HasLiveResource(CommandBuffer))
    {
      // TODO: we are leaking the original WrappedMTLCommandBuffer
      GetResourceManager()->EraseLiveResource(CommandBuffer);
    }
    GetResourceManager()->AddLiveResource(CommandBuffer, wrappedMTLCommandBuffer);

    m_WrappedMTLDevice->AddResource(CommandBuffer, ResourceType::CommandBuffer, "Command Buffer");
    m_WrappedMTLDevice->DerivedResource(queue, CommandBuffer);
  }
  return true;
}

MTL::CommandBuffer *WrappedMTLCommandQueue::real_commandBuffer()
{
  MTL::CommandQueue *realMTLCommandQueue = Unwrap<MTL::CommandQueue *>(this);
  MTL::CommandBuffer *realMTLCommandBuffer = realMTLCommandQueue->commandBuffer();
  return realMTLCommandBuffer;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandQueue, mtlCommandQueue_commandBuffer,
                                WrappedMTLCommandQueue *queue, WrappedMTLCommandBuffer *buffer);
