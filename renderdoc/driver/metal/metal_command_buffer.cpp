/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2022-2024 Baldur Karlsson
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
#include "metal_blit_command_encoder.h"
#include "metal_device.h"
#include "metal_render_command_encoder.h"
#include "metal_resources.h"
#include "metal_texture.h"

static int32_t s_activeCmdBuffers = 0;

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(MTL::CommandBuffer *realMTLCommandBuffer,
                                                 ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandBuffer, objId, wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  if(realMTLCommandBuffer && objId != ResourceId())
  {
    AllocateObjCBridge(this);
  }
  ++s_activeCmdBuffers;
}

WrappedMTLCommandBuffer::~WrappedMTLCommandBuffer()
{
  --s_activeCmdBuffers;
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_blitCommandEncoder(SerialiserType &ser,
                                                           WrappedMTLBlitCommandEncoder *encoder)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this).Unimportant();
  SERIALISE_ELEMENT_LOCAL(BlitCommandEncoder, GetResID(encoder))
      .TypedAs("MTLBlitCommandEncoder"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(CommandBuffer);
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::PassBoundary;
      action.flags |= ActionFlags::BeginPass;

      AddAction(action);
    }
    WrappedMTLCommandBuffer *cmdBuffer = m_Device->GetCurrentReplayCommandBuffer();
    MTL::BlitCommandEncoder *mtlBlitCommandEncoder = Unwrap(cmdBuffer)->blitCommandEncoder();
    WrappedMTLBlitCommandEncoder *wrappedMTLBlitCommandEncoder;
    ResourceId liveID =
        GetResourceManager()->WrapResource(mtlBlitCommandEncoder, wrappedMTLBlitCommandEncoder);
    wrappedMTLBlitCommandEncoder->SetCommandBuffer(cmdBuffer);
    if(!m_Device->IsPartialReplay())
    {
      if(GetResourceManager()->HasLiveResource(BlitCommandEncoder))
      {
        // TODO: we are leaking the original WrappedMTLBlitCommandEncoder
        GetResourceManager()->EraseLiveResource(BlitCommandEncoder);
      }
      GetResourceManager()->AddLiveResource(BlitCommandEncoder, wrappedMTLBlitCommandEncoder);
      m_Device->AddResource(BlitCommandEncoder, ResourceType::RenderPass, "Blit Encoder");
      m_Device->DerivedResource(cmdBuffer, BlitCommandEncoder);
    }
    RDCLOG("M %s blitCommandEncoder %s",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(cmdBuffer))).c_str(),
           ToStr(BlitCommandEncoder).c_str());

    RDCLOG("S %s blitCommandEncoder %s %s",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(cmdBuffer))).c_str(),
           ToStr(BlitCommandEncoder).c_str(), ToStr(liveID).c_str());
    m_Device->SetActiveBlitCommandEncoder(wrappedMTLBlitCommandEncoder);
    MetalRenderState &renderState = m_Device->GetCmdRenderState();
    // TODO: mixture of render and blit
    // ONLY ONE ACTIVE AT ONE TIME
    renderState.Init();
    renderState.blitPass = liveID;
    if(IsLoading(m_State))
    {
      RDCLOG("S CreateInfo %s", ToStr(BlitCommandEncoder).c_str());
    }
  }
  return true;
}

WrappedMTLBlitCommandEncoder *WrappedMTLCommandBuffer::blitCommandEncoder()
{
  MTL::BlitCommandEncoder *realMTLBlitCommandEncoder;
  SERIALISE_TIME_CALL(realMTLBlitCommandEncoder = Unwrap(this)->blitCommandEncoder());
  WrappedMTLBlitCommandEncoder *wrappedMTLBlitCommandEncoder;
  ResourceId id =
      GetResourceManager()->WrapResource(realMTLBlitCommandEncoder, wrappedMTLBlitCommandEncoder);
  wrappedMTLBlitCommandEncoder->SetCommandBuffer(this);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_blitCommandEncoder);
      Serialise_blitCommandEncoder(ser, wrappedMTLBlitCommandEncoder);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(this);
    bufferRecord->AddChunk(chunk);

    MetalResourceRecord *encoderRecord =
        GetResourceManager()->AddResourceRecord(wrappedMTLBlitCommandEncoder);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
  return wrappedMTLBlitCommandEncoder;
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

  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(CommandBuffer);
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    m_Device->SetCurrentCommandBufferRenderPassDescriptor(descriptor);
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.customName = StringFormat::Fmt("StartRenderEncoder(%s)",
                                            m_Device->MakeRenderPassOpString(true).c_str());
      action.flags |= ActionFlags::PassBoundary;
      action.flags |= ActionFlags::BeginPass;
      AddAction(action);
    }
    WrappedMTLCommandBuffer *cmdBuffer = m_Device->GetCurrentReplayCommandBuffer();
    MTL::RenderPassDescriptor *mtlDescriptor(descriptor);
    MTL::RenderCommandEncoder *mtlRenderCommandEncoder =
        Unwrap(cmdBuffer)->renderCommandEncoder(mtlDescriptor);
    mtlDescriptor->release();
    // TODO: don't make a new ID : replace liveID with new wrapped resource
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
    // TODO: mixture of render and blit
    // ONLY ONE ACTIVE AT ONE TIME
    renderState.Init();
    renderState.renderPass = liveID;
    MetalCreationInfo &c = m_Device->GetCreationInfo();
    c.m_RenderPass[liveID].Init(descriptor);
    RDCLOG("S CreateInfo %s", ToStr(liveID).c_str());
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
bool WrappedMTLCommandBuffer::Serialise_presentDrawable(SerialiserType &ser,
                                                        WrappedMTLTexture *presentedImage)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this);
  SERIALISE_ELEMENT(presentedImage).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(CommandBuffer);
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      ResourceId presentedImageId = GetResourceManager()->GetOriginalID(GetResID(presentedImage));
      action.customName = StringFormat::Fmt("presentDrawable(%s)", ToStr(presentedImageId).c_str());
      action.flags |= ActionFlags::Present;
      action.copyDestination = presentedImageId;
      m_Device->SetLastPresentedIamge(presentedImageId);
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
  SERIALISE_TIME_CALL(Unwrap(this)->presentDrawable(drawable));
  if(IsCaptureMode(m_State))
  {
    MetalDrawableInfo info = m_Device->UnregisterDrawableInfo(drawable);
    WrappedMTLTexture *presentedImage = info.texture;
    if(presentedImage)
    {
      Chunk *chunk = NULL;
      {
        CACHE_THREAD_SERIALISER();
        SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_presentDrawable);
        Serialise_presentDrawable(ser, presentedImage);
        chunk = scope.Get();
      }
      MetalResourceRecord *bufferRecord = GetRecord(this);
      bufferRecord->AddChunk(chunk);
      bufferRecord->cmdInfo->presented = true;
      bufferRecord->cmdInfo->outputLayer = info.mtlLayer;
      bufferRecord->cmdInfo->backBuffer = presentedImage;
    }
    else
    {
      RDCERR("Ignoring presentDrawable on untracked MTLDrawable");
    }
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
  MTL::CommandBuffer *mtlCommandBuffer = Unwrap(this);
  bool isCapture = IsCaptureMode(m_State);
  // During capture keep the real resource alive
  // It will be released when it is no longer required to be tracked
  if(isCapture)
    mtlCommandBuffer->retain();
  SERIALISE_TIME_CALL(mtlCommandBuffer->commit());
  if(isCapture)
  {
    MetalResourceRecord *bufferRecord = GetRecord(this);
    m_Device->CaptureCmdBufCommit(bufferRecord);
  }
  else
  {
    // TODO: implement RD MTL replay
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
    WrappedMTLCommandBuffer *cmdBuffer = m_Device->GetCurrentReplayCommandBuffer();
    RDCLOG("M %s enqueue", ToStr(GetResourceManager()->GetOriginalID(GetResID(cmdBuffer))).c_str());
    m_Device->ReplayCommandBufferEnqueue(cmdBuffer);
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
    m_Device->CaptureCmdBufEnqueue(bufferRecord);
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
    if(IsActiveCapturing(m_State))
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

INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLCommandBuffer,
                                            WrappedMTLBlitCommandEncoder *encoder,
                                            blitCommandEncoder);
INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLCommandBuffer,
                                            WrappedMTLRenderCommandEncoder *encoder,
                                            renderCommandEncoderWithDescriptor,
                                            RDMTL::RenderPassDescriptor &descriptor);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, presentDrawable,
                                WrappedMTLTexture *presentedImage);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, commit);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, enqueue);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, waitUntilCompleted);
