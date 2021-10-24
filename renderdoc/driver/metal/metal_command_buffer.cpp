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

#include "metal_command_buffer.h"
#include "core/core.h"
#include "core/settings.h"
#include "metal_manager.h"
#include "metal_render_command_encoder.h"

RDOC_DEBUG_CONFIG(
    bool, Metal_Debug_VerboseCommandRecording, false,
    "Add verbose logging around recording and submission of command buffers in Metal.");

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(id_MTLCommandBuffer realMTLCommandBuffer,
                                                 ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandBuffer, objId, wrappedMTLDevice),
      m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLCommandBuffer = CreateObjCWrappedMTLCommandBuffer();
}

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice), m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLCommandBuffer = CreateObjCWrappedMTLCommandBuffer();
}

void WrappedMTLCommandBuffer::SetWrappedMTLCommandQueue(WrappedMTLCommandQueue *wrappedMTLCommandQueue)
{
  m_WrappedMTLCommandQueue = wrappedMTLCommandQueue;
}

id_MTLRenderCommandEncoder WrappedMTLCommandBuffer::renderCommandEncoderWithDescriptor(
    MTLRenderPassDescriptor *descriptor)
{
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder;
  SERIALISE_TIME_CALL(realMTLRenderCommandEncoder =
                          real_renderCommandEncoderWithDescriptor(descriptor));
  MetalResourceManager::UnwrapHelper<id_MTLRenderCommandEncoder>::Outer *wrappedMTLRenderCommandEncoder;
  ResourceId id = GetResourceManager()->WrapResource(realMTLRenderCommandEncoder,
                                                     wrappedMTLRenderCommandEncoder);
  wrappedMTLRenderCommandEncoder->SetWrappedMTLCommandBuffer(this);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlCommandBuffer_renderCommandEncoderWithDescriptor);
      Serialise_mtlCommandBuffer_renderCommandEncoderWithDescriptor(ser, this,
                                                                    wrappedMTLRenderCommandEncoder);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(this);
    RDCASSERT(!bufferRecord->cmdInfo->isEncoding);
    bufferRecord->AddChunk(chunk);

    MetalResourceRecord *encoderRecord =
        GetResourceManager()->AddResourceRecord(wrappedMTLRenderCommandEncoder);
    if(Metal_Debug_VerboseCommandRecording())
    {
      RDCLOG("Allocate RenderCommandEncoder %s CommandBuffer %s",
             ToStr(encoderRecord->GetResourceID()).c_str(),
             ToStr(bufferRecord->GetResourceID()).c_str());
    }
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
  return wrappedMTLRenderCommandEncoder->GetObjCWrappedMTLRenderCommandEncoder();
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_mtlCommandBuffer_renderCommandEncoderWithDescriptor(
    SerialiserType &ser, WrappedMTLCommandBuffer *buffer, WrappedMTLRenderCommandEncoder *encoder)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

void WrappedMTLCommandBuffer::presentDrawable(id_MTLDrawable drawable)
{
  SERIALISE_TIME_CALL(real_presentDrawable(drawable));
  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *record = GetRecord(this);
    if(Metal_Debug_VerboseCommandRecording())
    {
      RDCLOG("Present CommandBuffer %s", ToStr(record->GetResourceID()).c_str());
    }

    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlCommandBuffer_presentDrawable);
      Serialise_mtlCommandBuffer_presentDrawable(ser, this, drawable);
      chunk = scope.Get();
    }
    record->AddChunk(chunk);
    record->cmdInfo->present = true;
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_mtlCommandBuffer_presentDrawable(
    SerialiserType &ser, WrappedMTLCommandBuffer *buffer, id_MTLDrawable drawable)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

void WrappedMTLCommandBuffer::commit()
{
  MetalResourceRecord *record = GetRecord(this);
  bool present = record->cmdInfo->present;

  SERIALISE_TIME_CALL(real_commit());
  if(IsCaptureMode(m_State))
  {
    if(Metal_Debug_VerboseCommandRecording())
    {
      RDCLOG("Commit CommandBuffer %s", ToStr(record->GetResourceID()).c_str());
    }

    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlCommandBuffer_commit);
      Serialise_mtlCommandBuffer_commit(ser, this);
      chunk = scope.Get();
    }
    record->AddChunk(chunk);

    bool capframe = IsActiveCapturing(m_State);
    if(capframe)
    {
      m_WrappedMTLDevice->AddCommandBufferRecord(record);
    }
  }
  else
  {
    // TODO: implement RD MTL replay
  }

  if(present)
  {
    m_WrappedMTLDevice->AdvanceFrame();
    m_WrappedMTLDevice->Present(NULL);
  }
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_mtlCommandBuffer_commit(SerialiserType &ser,
                                                                WrappedMTLCommandBuffer *buffer)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer, mtlCommandBuffer_commit,
                                WrappedMTLCommandBuffer *buffer);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer,
                                mtlCommandBuffer_renderCommandEncoderWithDescriptor,
                                WrappedMTLCommandBuffer *buffer,
                                WrappedMTLRenderCommandEncoder *encoder);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer, mtlCommandBuffer_presentDrawable,
                                WrappedMTLCommandBuffer *buffer, id_MTLDrawable drawable);
