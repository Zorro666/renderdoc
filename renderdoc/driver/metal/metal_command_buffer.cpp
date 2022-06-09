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

#include "metal_command_buffer.h"
#include "metal_buffer.h"
#include "metal_device.h"
#include "metal_helpers_bridge.h"
#include "metal_render_command_encoder.h"
#include "metal_resources.h"
#include "metal_texture.h"

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(MTL::CommandBuffer *realMTLCommandBuffer,
                                                 ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandBuffer, objId, wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  AllocateObjCBridge(this);
}

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  m_ObjcBridge = NULL;
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_renderCommandEncoderWithDescriptor(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder,
    RDMTL::RenderPassDescriptor &descriptor)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT(descriptor).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(CommandBuffer);
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    WrappedMTLCommandBuffer *cmdBuffer = m_Device->GetCurrentReplayCommandBuffer();
    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //
    //      ActionDescription action;
    //      action.flags |= ActionFlags::PassBoundary;
    //      action.flags |= ActionFlags::BeginPass;
    //
    //      AddAction(action);
    //    }
    MTL::RenderPassDescriptor *mtlDescriptor(descriptor);
    MTL::RenderCommandEncoder *mtlRenderCommandEncoder =
        Unwrap(cmdBuffer)->renderCommandEncoder(mtlDescriptor);
    mtlDescriptor->release();
    WrappedMTLRenderCommandEncoder *wrappedMTLRenderCommandEncoder;
    ResourceId liveID =
        GetResourceManager()->WrapResource(mtlRenderCommandEncoder, wrappedMTLRenderCommandEncoder);
    wrappedMTLRenderCommandEncoder->SetCommandBuffer(cmdBuffer);
    if(!m_Device->IsPartialReplay())
    {
      if(GetResourceManager()->HasLiveResource(RenderCommandEncoder))
      {
        // TODO: we are leaking the original WrappedMTLRenderCommandEncoder
        GetResourceManager()->EraseLiveResource(RenderCommandEncoder);
      }
      GetResourceManager()->AddLiveResource(RenderCommandEncoder, wrappedMTLRenderCommandEncoder);
      m_Device->AddResource(RenderCommandEncoder, ResourceType::RenderPass, "Render Encoder");
      m_Device->DerivedResource(cmdBuffer, RenderCommandEncoder);
    }
    RDCLOG("M %s renderCommandEncoder %s",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(cmdBuffer))).c_str(),
           ToStr(RenderCommandEncoder).c_str());

    RDCLOG("S %s renderCommandEncoder %s %s",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(cmdBuffer))).c_str(),
           ToStr(RenderCommandEncoder).c_str(), ToStr(liveID).c_str());
    m_Device->SetActiveRenderCommandEncoder(wrappedMTLRenderCommandEncoder);
    MetalRenderState &renderState = m_Device->GetCmdRenderState();
    renderState.Init();
    renderState.renderPass = liveID;
    if(IsLoading(m_State))
    {
      MetalCreationInfo &c = m_Device->GetCreationInfo();
      c.m_RenderPass[RenderCommandEncoder].Init(descriptor);
      RDCLOG("S CreateInfo %s", ToStr(RenderCommandEncoder).c_str());
    }
  }
  return true;
}

WrappedMTLRenderCommandEncoder *WrappedMTLCommandBuffer::renderCommandEncoderWithDescriptor(
    RDMTL::RenderPassDescriptor &descriptor)
{
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder;
  MTL::RenderPassDescriptor *mtlDescriptor(descriptor);
  SERIALISE_TIME_CALL(realMTLRenderCommandEncoder =
                          Unwrap(this)->renderCommandEncoder(mtlDescriptor));
  mtlDescriptor->release();
  WrappedMTLRenderCommandEncoder *wrappedMTLRenderCommandEncoder;
  ResourceId id = GetResourceManager()->WrapResource(realMTLRenderCommandEncoder,
                                                     wrappedMTLRenderCommandEncoder);
  wrappedMTLRenderCommandEncoder->SetCommandBuffer(this);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_renderCommandEncoderWithDescriptor);
      Serialise_renderCommandEncoderWithDescriptor(ser, wrappedMTLRenderCommandEncoder, descriptor);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(this);
    bufferRecord->AddChunk(chunk);

    MetalResourceRecord *encoderRecord =
        GetResourceManager()->AddResourceRecord(wrappedMTLRenderCommandEncoder);

    for(int i = 0; i < descriptor.colorAttachments.count(); ++i)
    {
      WrappedMTLTexture *texture = descriptor.colorAttachments[i].texture;
      if(texture != NULL)
      {
        bufferRecord->MarkResourceFrameReferenced(GetResID(texture), eFrameRef_Read);
      }
    }
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
  return wrappedMTLRenderCommandEncoder;
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_presentDrawable(SerialiserType &ser, MTL::Drawable *drawable)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(CommandBuffer);
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::Present;

      AddAction(action);
    }
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    WrappedMTLCommandBuffer *cmdBuffer = m_Device->GetCurrentReplayCommandBuffer();
    RDCLOG("M %s present", ToStr(GetResourceManager()->GetOriginalID(GetResID(cmdBuffer))).c_str());
  }
  return true;
}

void WrappedMTLCommandBuffer::presentDrawable(MTL::Drawable *drawable)
{
  // To avoid metal assert about accessing drawable texture after calling present
  MTL::Texture *mtlBackBuffer = ObjC::Get_Texture(drawable);

  SERIALISE_TIME_CALL(Unwrap(this)->presentDrawable(drawable));
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_presentDrawable);
      Serialise_presentDrawable(ser, drawable);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(this);
    bufferRecord->AddChunk(chunk);
    bufferRecord->cmdInfo->present = true;
    bufferRecord->cmdInfo->outputLayer = ObjC::Get_Layer(drawable);
    bufferRecord->cmdInfo->backBuffer = GetWrapped(mtlBackBuffer);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_enqueue(SerialiserType &ser)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(CommandBuffer);
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    WrappedMTLCommandBuffer *cmdBuffer = m_Device->GetCurrentReplayCommandBuffer();
    RDCLOG("M %s enqueue", ToStr(GetResourceManager()->GetOriginalID(GetResID(cmdBuffer))).c_str());
  }
  return true;
}

void WrappedMTLCommandBuffer::enqueue()
{
  SERIALISE_TIME_CALL(Unwrap(this)->enqueue());
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_enqueue);
      Serialise_enqueue(ser);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(this);
    bufferRecord->AddChunk(chunk);
    m_Device->EnqueueCommandBufferRecord(bufferRecord);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_waitUntilCompleted(SerialiserType &ser)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(NULL);
    if(IsActiveReplaying(m_State))
    {
      RDCLOG("M %s wait",
             ToStr(GetResourceManager()->GetOriginalID(GetResID(CommandBuffer))).c_str());
      Unwrap(CommandBuffer)->waitUntilCompleted();
    }
  }
  return true;
}

void WrappedMTLCommandBuffer::waitUntilCompleted()
{
  SERIALISE_TIME_CALL(Unwrap(this)->waitUntilCompleted());
  if(IsCaptureMode(m_State))
  {
    bool capframe = IsActiveCapturing(m_State);
    if(capframe)
    {
      Chunk *chunk = NULL;
      {
        CACHE_THREAD_SERIALISER();
        SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_waitUntilCompleted);
        Serialise_waitUntilCompleted(ser);
        chunk = scope.Get();
      }
      MetalResourceRecord *bufferRecord = GetRecord(this);
      bufferRecord->AddChunk(chunk);
    }
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

void WrappedMTLCommandBuffer::label(const char *label)
{
  SERIALISE_TIME_CALL(NS::String *nsLabel = NS::String::string(label, NS::UTF8StringEncoding);
                      Unwrap(this)->setLabel(nsLabel));
  if(IsCaptureMode(m_State))
  {
    METAL_NOT_IMPLEMENTED();
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_commit(SerialiserType &ser)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(CommandBuffer);
    WrappedMTLCommandBuffer *cmdBuffer = m_Device->GetCurrentReplayCommandBuffer();
    m_Device->ReplayCommandBufferCommit(cmdBuffer);
  }
  return true;
}

void WrappedMTLCommandBuffer::commit()
{
  SERIALISE_TIME_CALL(Unwrap(this)->commit());
  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *bufferRecord = GetRecord(this);
    int64_t submitIndex = bufferRecord->cmdInfo->submitIndex;
    bool capture = ((submitIndex == 0) && IsActiveCapturing(m_State)) ||
                   ((submitIndex > 0) && m_Device->ShouldCaptureEnqueuedCommandBuffer(submitIndex));
    if(capture)
    {
      m_Device->StartAddCommandBufferRecord(bufferRecord);
      bufferRecord->AddRef();
      Chunk *chunk = NULL;
      std::unordered_set<ResourceId> refIDs;
      bufferRecord->AddReferencedIDs(refIDs);
      // snapshot/detect any CPU modifications
      // to referenced shared storage mode MTLBuffer contents
      for(auto it = refIDs.begin(); it != refIDs.end(); ++it)
      {
        ResourceId id = *it;
        MetalResourceRecord *record = GetResourceManager()->GetResourceRecord(id);
        if(record->m_Type == eResBuffer)
        {
          MetalBufferInfo *bufInfo = record->bufInfo;
          if(bufInfo->storageMode == MTL::StorageModeShared)
          {
            size_t diffStart = 0;
            size_t diffEnd = 0;
            bool foundDifference = true;
            if(bufInfo->baseSnapshot.isEmpty())
            {
              bufInfo->baseSnapshot.assign(bufInfo->data, bufInfo->length);
              diffEnd = bufInfo->length;
            }
            else
            {
              foundDifference = FindDiffRange(bufInfo->data, bufInfo->baseSnapshot.data(),
                                              bufInfo->length, diffStart, diffEnd);
              if(diffEnd <= diffStart)
                foundDifference = false;
            }

            if(foundDifference)
            {
              if(bufInfo->data == NULL)
              {
                RDCERR("Writing buffer memory %s that isn't currently mapped", ToStr(id).c_str());
                return;
              }
              Chunk *chunk = NULL;
              {
                CACHE_THREAD_SERIALISER();
                SCOPED_SERIALISE_CHUNK(MetalChunk::MTLBuffer_InternalModifyCPUContents);
                ((WrappedMTLBuffer *)record->m_Resource)
                    ->Serialise_InternalModifyCPUContents(ser, diffStart, diffEnd);
                chunk = scope.Get();
              }
              bufferRecord->AddChunk(chunk);
            }
          }
        }
      }
      bufferRecord->MarkResourceFrameReferenced(GetResID(m_CommandQueue), eFrameRef_Read);
      // pull in frame refs from this command buffer
      bufferRecord->AddResourceReferences(GetResourceManager());
      {
        CACHE_THREAD_SERIALISER();
        SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_commit);
        Serialise_commit(ser);
        chunk = scope.Get();
      }
      bufferRecord->AddChunk(chunk);
      m_Device->EndAddCommandBufferRecord(bufferRecord);
    }
    else
    {
      if(bufferRecord->cmdInfo->submitIndex != 0)
      {
        m_Device->CommitCommandBufferRecord(bufferRecord);
      }
    }
    bool present = bufferRecord->cmdInfo->present;
    if(present)
    {
      m_Device->AdvanceFrame();
      m_Device->Present(bufferRecord);
    }
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLCommandBuffer,
                                            WrappedMTLRenderCommandEncoder *encoder,
                                            renderCommandEncoderWithDescriptor,
                                            RDMTL::RenderPassDescriptor &descriptor);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, presentDrawable,
                                MTL::Drawable *drawable);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, enqueue);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, commit);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, waitUntilCompleted);
