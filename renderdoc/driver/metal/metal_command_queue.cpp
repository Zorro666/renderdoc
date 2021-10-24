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

#include "metal_command_queue.h"
#include "core/core.h"
#include "core/settings.h"
#include "metal_command_buffer.h"
#include "metal_manager.h"

RDOC_EXTERN_CONFIG(bool, Metal_Debug_VerboseCommandRecording);

WrappedMTLCommandQueue::WrappedMTLCommandQueue(id_MTLCommandQueue realMTLCommandQueue,
                                               ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandQueue, objId, wrappedMTLDevice),
      m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLCommandQueue = CreateObjCWrappedMTLCommandQueue();
}

WrappedMTLCommandQueue::WrappedMTLCommandQueue(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice), m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLCommandQueue = CreateObjCWrappedMTLCommandQueue();
}

id_MTLCommandBuffer WrappedMTLCommandQueue::commandBuffer()
{
  id_MTLCommandBuffer realMTLCommandBuffer;
  SERIALISE_TIME_CALL(realMTLCommandBuffer = real_commandBuffer());
  MetalResourceManager::UnwrapHelper<id_MTLCommandBuffer>::Outer *wrappedMTLCommandBuffer;
  ResourceId id = GetResourceManager()->WrapResource(realMTLCommandBuffer, wrappedMTLCommandBuffer);
  wrappedMTLCommandBuffer->SetWrappedMTLCommandQueue(this);

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlCommandQueue_commandBuffer);
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

    bufferRecord->cmdInfo = new CmdBufferRecordingInfo();
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

  return wrappedMTLCommandBuffer->GetObjCWrappedMTLCommandBuffer();
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
  }
  return true;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandQueue, mtlCommandQueue_commandBuffer,
                                WrappedMTLCommandQueue *queue, WrappedMTLCommandBuffer *buffer);
