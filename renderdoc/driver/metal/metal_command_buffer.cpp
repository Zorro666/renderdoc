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
#include "core/settings.h"
#include "metal_device.h"
#include "metal_render_command_encoder.h"
#include "metal_resources.h"
#include "metal_texture.h"

RDOC_DEBUG_CONFIG(
    bool, Metal_Debug_VerboseCommandRecording, false,
    "Add verbose logging around recording and submission of command buffers in Metal.");

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(MTL::CommandBuffer *realMTLCommandBuffer,
                                                 ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandBuffer, objId, wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = AllocateObjCWrapper(this);
}

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = NULL;
}

void WrappedMTLCommandBuffer::SetWrappedMTLCommandQueue(WrappedMTLCommandQueue *wrappedMTLCommandQueue)
{
  m_WrappedMTLCommandQueue = wrappedMTLCommandQueue;
}

MTL::RenderCommandEncoder *WrappedMTLCommandBuffer::renderCommandEncoderWithDescriptor(
    MTL::RenderPassDescriptor *descriptor)
{
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder;
  SERIALISE_TIME_CALL(realMTLRenderCommandEncoder =
                          real_renderCommandEncoderWithDescriptor(descriptor));
  MetalResourceManager::UnwrapHelper<MTL::RenderCommandEncoder *>::Outer *wrappedMTLRenderCommandEncoder;
  ResourceId id = GetResourceManager()->WrapResource(realMTLRenderCommandEncoder,
                                                     wrappedMTLRenderCommandEncoder);
  wrappedMTLRenderCommandEncoder->SetWrappedMTLCommandBuffer(this);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_renderCommandEncoderWithDescriptor);
      Serialise_mtlCommandBuffer_renderCommandEncoderWithDescriptor(
          ser, this, wrappedMTLRenderCommandEncoder, descriptor);
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
    for(uint32_t i = 0; i < MAX_RENDER_PASS_COLOR_ATTACHMENTS; ++i)
    {
      MTL::RenderPassColorAttachmentDescriptor *colorAttachment =
          descriptor->colorAttachments()->object(i);
      MTL::Texture *texture = colorAttachment->texture();
      if(texture != NULL)
      {
        GetResourceManager()->MarkResourceFrameReferenced(GetResID(GetWrapped(texture)),
                                                          eFrameRef_Read);
      }
    }
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
  return UnwrapObjC<MTL::RenderCommandEncoder *>(wrappedMTLRenderCommandEncoder);
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_mtlCommandBuffer_renderCommandEncoderWithDescriptor(
    SerialiserType &ser, WrappedMTLCommandBuffer *buffer, WrappedMTLRenderCommandEncoder *encoder,
    MTL::RenderPassDescriptor *descriptor)
{
  RDCASSERT(m_WrappedMTLCommandQueue);
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT(descriptor);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    ResourceId liveID;

    WrappedMTLCommandBuffer *buffer =
        (WrappedMTLCommandBuffer *)GetResourceManager()->GetLiveResource(CommandBuffer);
    MTL::RenderCommandEncoder *realMTLRenderCommandEncoder =
        buffer->real_renderCommandEncoderWithDescriptor(descriptor);
    MetalResourceManager::UnwrapHelper<MTL::RenderCommandEncoder *>::Outer *wrappedMTLRenderCommandEncoder;
    liveID = GetResourceManager()->WrapResource(realMTLRenderCommandEncoder,
                                                wrappedMTLRenderCommandEncoder);
    wrappedMTLRenderCommandEncoder->SetWrappedMTLCommandBuffer(buffer);
    if(GetResourceManager()->HasLiveResource(RenderCommandEncoder))
    {
      // TODO: we are leaking the original WrappedMTLRenderCommandEncoder
      GetResourceManager()->EraseLiveResource(RenderCommandEncoder);
    }
    GetResourceManager()->AddLiveResource(RenderCommandEncoder, wrappedMTLRenderCommandEncoder);
    m_WrappedMTLDevice->AddResource(RenderCommandEncoder, ResourceType::RenderPass,
                                    "Render Encoder");
    m_WrappedMTLDevice->DerivedResource(buffer, RenderCommandEncoder);

    m_WrappedMTLDevice->SetRenderPassActiveState(WrappedMTLDevice::Primary, true);
    m_WrappedMTLDevice->SetActiveRenderCommandEnvoder(wrappedMTLRenderCommandEncoder);
    MetalRenderState &renderstate = m_WrappedMTLDevice->GetCmdRenderState();
    renderstate.Init();
    renderstate.renderPass = liveID;
    MetalCreationInfo &c = m_WrappedMTLDevice->GetCreationInfo();
    MetalCreationInfo::RenderPass &rp = c.m_RenderPass[liveID];
    rp.colorAttachments.resize(MAX_RENDER_PASS_COLOR_ATTACHMENTS);
    // Save render stage settings from MTLRenderPassDescriptor
    for(uint32_t i = 0; i < MAX_RENDER_PASS_COLOR_ATTACHMENTS; ++i)
    {
      MTL::RenderPassColorAttachmentDescriptor *colorAttachment =
          descriptor->colorAttachments()->object(i);
      // MTLRenderPassColorAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
      MTL::Texture *texture = colorAttachment->texture();
      rp.colorAttachments[i].texture = GetId(texture);
      rp.colorAttachments[i].level = colorAttachment->level();
      rp.colorAttachments[i].slice = colorAttachment->slice();
      rp.colorAttachments[i].depthPlane = colorAttachment->depthPlane();
      MTL::Texture *resolveTexture = colorAttachment->resolveTexture();
      rp.colorAttachments[i].resolveTexture = GetId(resolveTexture);
      rp.colorAttachments[i].resolveLevel = colorAttachment->resolveLevel();
      rp.colorAttachments[i].resolveSlice = colorAttachment->resolveSlice();
      rp.colorAttachments[i].resolveDepthPlane = colorAttachment->resolveDepthPlane();
      rp.colorAttachments[i].loadAction = colorAttachment->loadAction();
      rp.colorAttachments[i].storeAction = colorAttachment->storeAction();
      rp.colorAttachments[i].storeActionOptions = colorAttachment->storeActionOptions();
      rp.colorAttachments[i].clearColor = colorAttachment->clearColor();
    }

    // depthAttachment
    // stencilAttachment
    // visibilityResultBuffer
    // renderTargetArrayLength
    // imageblockSampleLength
    // threadgroupMemoryLength
    // tileWidth
    // tileHeight
    // defaultRasterSampleCount
    // renderTargetWidth
    // renderTargetHeight
    // samplePositions
    // rasterizationRateMap
    // sampleBufferAttachments

    // MTLRenderPassAttachmentDescriptor
    // texture
    // level
    // slice
    // depthPlane
    // resolveTexture
    // resolveLevel
    // resolveSlice
    // resolveDepthPlane
    // loadAction
    // storeAction
    // storeActionOptions

    // MTLRenderPassDepthAttachmentDescriptor
    // texture
    // level
    // slice
    // depthPlane
    // resolveTexture
    // resolveLevel
    // resolveSlice
    // resolveDepthPlane
    // loadAction
    // storeAction
    // storeActionOptions
    // clearDepth
    // depthResolveFilter

    // MTLRenderPassStencilAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
    // texture
    // level
    // slice
    // depthPlane
    // resolveTexture
    // resolveLevel
    // resolveSlice
    // resolveDepthPlane
    // loadAction
    // storeAction
    // storeActionOptions
    // clearStencil
    // stencilResolveFilter
  }
  return true;
}

void WrappedMTLCommandBuffer::presentDrawable(MTL::Drawable *drawable)
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
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_presentDrawable);
      Serialise_mtlCommandBuffer_presentDrawable(ser, this, drawable);
      chunk = scope.Get();
    }
    record->AddChunk(chunk);
    record->cmdInfo->present = true;
    record->cmdInfo->drawable = drawable;
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_mtlCommandBuffer_presentDrawable(
    SerialiserType &ser, WrappedMTLCommandBuffer *buffer, MTL::Drawable *drawable)
{
  RDCASSERT(m_WrappedMTLCommandQueue);
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

void WrappedMTLCommandBuffer::enqueue()
{
  SERIALISE_TIME_CALL(real_enqueue());
  if(IsCaptureMode(m_State))
  {
    METAL_NOT_IMPLEMENTED();
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

void WrappedMTLCommandBuffer::waitUntilCompleted()
{
  SERIALISE_TIME_CALL(real_waitUntilCompleted());
  if(IsCaptureMode(m_State))
  {
    METAL_NOT_IMPLEMENTED();
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

void WrappedMTLCommandBuffer::label(const char *label)
{
  SERIALISE_TIME_CALL(real_label(label));
  if(IsCaptureMode(m_State))
  {
    METAL_NOT_IMPLEMENTED();
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

void WrappedMTLCommandBuffer::commit()
{
  SERIALISE_TIME_CALL(real_commit());
  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *record = GetRecord(this);
    bool present = record->cmdInfo->present;
    MTL::Drawable *drawable = record->cmdInfo->drawable;

    if(Metal_Debug_VerboseCommandRecording())
    {
      RDCLOG("Commit CommandBuffer %s", ToStr(record->GetResourceID()).c_str());
    }

    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_commit);
      Serialise_mtlCommandBuffer_commit(ser, this);
      chunk = scope.Get();
    }
    record->AddChunk(chunk);

    bool capframe = IsActiveCapturing(m_State);
    if(capframe)
    {
      record->AddRef();
      m_WrappedMTLDevice->AddCommandBufferRecord(record);
    }
    if(present)
    {
      m_WrappedMTLDevice->AdvanceFrame();
      m_WrappedMTLDevice->Present(m_WrappedMTLDevice);
    }
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_mtlCommandBuffer_commit(SerialiserType &ser,
                                                                WrappedMTLCommandBuffer *buffer)
{
  RDCASSERT(m_WrappedMTLCommandQueue);
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    buffer = (WrappedMTLCommandBuffer *)GetResourceManager()->GetLiveResource(CommandBuffer);
    buffer->real_commit();
  }
  return true;
}

void WrappedMTLCommandBuffer::real_presentDrawable(MTL::Drawable *drawable)
{
  MTL::CommandBuffer *realMTLCommandBuffer = Unwrap<MTL::CommandBuffer *>(this);
  realMTLCommandBuffer->presentDrawable(drawable);
}

MTL::RenderCommandEncoder *WrappedMTLCommandBuffer::real_renderCommandEncoderWithDescriptor(
    MTL::RenderPassDescriptor *descriptor)
{
  MTL::CommandBuffer *realMTLCommandBuffer = Unwrap<MTL::CommandBuffer *>(this);

  MTL::RenderPassDescriptor *realDescriptor = descriptor->copy();

  // The source descriptor contains wrapped MTLTexture resources
  // Need to unwrap them to the real resource before calling real API
  for(uint32_t i = 0; i < MAX_RENDER_PASS_COLOR_ATTACHMENTS; ++i)
  {
    MTL::Texture *wrappedTexture = descriptor->colorAttachments()->object(i)->texture();
    if(wrappedTexture != NULL)
    {
      if(IsObjCWrapped(wrappedTexture))
      {
        realDescriptor->colorAttachments()->object(i)->setTexture(GetReal(wrappedTexture));
      }
    }
  }

  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder =
      realMTLCommandBuffer->renderCommandEncoder(realDescriptor);
  realDescriptor->release();

  return realMTLRenderCommandEncoder;
}

void WrappedMTLCommandBuffer::real_enqueue()
{
  MTL::CommandBuffer *realMTLCommandBuffer = Unwrap<MTL::CommandBuffer *>(this);
  realMTLCommandBuffer->enqueue();
}

void WrappedMTLCommandBuffer::real_label(const char *label)
{
  MTL::CommandBuffer *realMTLCommandBuffer = Unwrap<MTL::CommandBuffer *>(this);
  NS::String *nsLabel = NS::String::string(label, NS::UTF8StringEncoding);
  realMTLCommandBuffer->setLabel(nsLabel);
}

void WrappedMTLCommandBuffer::real_waitUntilCompleted()
{
  MTL::CommandBuffer *realMTLCommandBuffer = Unwrap<MTL::CommandBuffer *>(this);
  realMTLCommandBuffer->waitUntilCompleted();
}

void WrappedMTLCommandBuffer::real_commit()
{
  MTL::CommandBuffer *realMTLCommandBuffer = Unwrap<MTL::CommandBuffer *>(this);
  realMTLCommandBuffer->commit();
}

void WrappedMTLCommandBuffer::CopyTexture(MTL::Texture *source, MTL::Texture *destination)
{
  MTL::Texture *realSource = GetReal(source);
  MTL::Texture *realDestination = GetReal(destination);

  MTL::CommandBuffer *realMTLCommandBuffer = Unwrap<MTL::CommandBuffer *>(this);
  MTL::BlitCommandEncoder *blitCommandEncoder = realMTLCommandBuffer->blitCommandEncoder();
  blitCommandEncoder->copyFromTexture(realSource, realDestination);
  blitCommandEncoder->endEncoding();
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer, mtlCommandBuffer_commit,
                                WrappedMTLCommandBuffer *buffer);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer,
                                mtlCommandBuffer_renderCommandEncoderWithDescriptor,
                                WrappedMTLCommandBuffer *buffer,
                                WrappedMTLRenderCommandEncoder *encoder,
                                MTL::RenderPassDescriptor *descriptor);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer, mtlCommandBuffer_presentDrawable,
                                WrappedMTLCommandBuffer *buffer, MTL::Drawable *drawable);
