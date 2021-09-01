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
#include "metal_metal.h"
#include "metal_resources.h"

RDOC_DEBUG_CONFIG(
    bool, Metal_Debug_VerboseCommandRecording, false,
    "Add verbose logging around recording and submission of command buffers in Metal.");

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(id_MTLCommandBuffer realMTLCommandBuffer,
                                                 ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLCommandBuffer, objId, wrappedMTLDevice),
      m_State(wrappedMTLDevice->GetStateRef())
{
  objc = CreateObjCWrappedMTLCommandBuffer();
}

WrappedMTLCommandBuffer::WrappedMTLCommandBuffer(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice), m_State(wrappedMTLDevice->GetStateRef())
{
  objc = CreateObjCWrappedMTLCommandBuffer();
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
      MTLRenderPassColorAttachmentDescriptor *colorAttachment =
          MTL_GET(descriptor, colorAttachment, i);
      id_MTLTexture texture = MTL_GET(colorAttachment, texture);
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
  return wrappedMTLRenderCommandEncoder->objc;
}

template <typename SerialiserType>
bool WrappedMTLCommandBuffer::Serialise_mtlCommandBuffer_renderCommandEncoderWithDescriptor(
    SerialiserType &ser, WrappedMTLCommandBuffer *buffer, WrappedMTLRenderCommandEncoder *encoder,
    MTLRenderPassDescriptor *descriptor)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, GetResID(buffer)).TypedAs("MTLCommandBuffer"_lit);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT(descriptor).TypedAs("MTLRenderPassDescriptor");

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    ResourceId liveID;

    WrappedMTLCommandBuffer *buffer =
        (WrappedMTLCommandBuffer *)GetResourceManager()->GetLiveResource(CommandBuffer);
    id_MTLRenderCommandEncoder realMTLRenderCommandEncoder =
        buffer->real_renderCommandEncoderWithDescriptor(descriptor);
    MetalResourceManager::UnwrapHelper<id_MTLRenderCommandEncoder>::Outer *wrappedMTLRenderCommandEncoder;
    liveID = GetResourceManager()->WrapResource(realMTLRenderCommandEncoder,
                                                wrappedMTLRenderCommandEncoder);
    if(GetResourceManager()->HasLiveResource(RenderCommandEncoder))
    {
      // TODO: we are leaking the original WrappedMTLRenderCommandEncoder
      GetResourceManager()->EraseLiveResource(RenderCommandEncoder);
    }
    GetResourceManager()->AddLiveResource(RenderCommandEncoder, wrappedMTLRenderCommandEncoder);
    m_WrappedMTLDevice->AddResource(RenderCommandEncoder, ResourceType::RenderPass,
                                    "Render Encoder");
    m_WrappedMTLDevice->DerivedResource(buffer, RenderCommandEncoder);

    MetalRenderState &renderstate = m_WrappedMTLDevice->GetCmdRenderState();
    renderstate.Init();
    renderstate.renderPass = liveID;
    MetalCreationInfo &c = m_WrappedMTLDevice->GetCreationInfo();
    MetalCreationInfo::RenderPass &rp = c.m_RenderPass[liveID];
    rp.colorAttachments.resize(MAX_RENDER_PASS_COLOR_ATTACHMENTS);
    // Save render stage settings from MTLRenderPassDescriptor
    for(uint32_t i = 0; i < MAX_RENDER_PASS_COLOR_ATTACHMENTS; ++i)
    {
      MTLRenderPassColorAttachmentDescriptor *colorAttachment =
          MTL_GET(descriptor, colorAttachment, i);
      // MTLRenderPassColorAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
      id_MTLTexture texture = MTL_GET(colorAttachment, texture);
      rp.colorAttachments[i].texture = GetId(texture);
      rp.colorAttachments[i].level = MTL_GET(colorAttachment, level);
      rp.colorAttachments[i].slice = MTL_GET(colorAttachment, slice);
      rp.colorAttachments[i].depthPlane = MTL_GET(colorAttachment, depthPlane);
      id_MTLTexture resolveTexture = MTL_GET(colorAttachment, resolveTexture);
      rp.colorAttachments[i].resolveTexture = GetId(resolveTexture);
      rp.colorAttachments[i].resolveLevel = MTL_GET(colorAttachment, resolveLevel);
      rp.colorAttachments[i].resolveSlice = MTL_GET(colorAttachment, resolveSlice);
      rp.colorAttachments[i].resolveDepthPlane = MTL_GET(colorAttachment, resolveDepthPlane);
      rp.colorAttachments[i].loadAction = MTL_GET(colorAttachment, loadAction);
      rp.colorAttachments[i].storeAction = MTL_GET(colorAttachment, storeAction);
      rp.colorAttachments[i].storeActionOptions = MTL_GET(colorAttachment, storeActionOptions);
      rp.colorAttachments[i].clearColor = MTL_GET(colorAttachment, clearColor);
    }

    // GETSET_PROPERTY(MTLRenderPassDescriptor, MTLRenderPassDepthAttachmentDescriptor *,
    // depthAttachment);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, MTLRenderPassStencilAttachmentDescriptor *,
    // stencilAttachment);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, id_MTLBuffer, visibilityResultBuffer);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, renderTargetArrayLength);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, imageblockSampleLength);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, threadgroupMemoryLength);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, tileWidth);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, tileHeight);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, defaultRasterSampleCount);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, renderTargetWidth);
    // GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, renderTargetHeight);
    // TODO: GETSET_PROPERTY(MTLRenderPassDescriptor, MTLSamplePosition*, samplePositions);
    // TODO: GETSET_PROPERTY(MTLRenderPassDescriptor, id_MTLRasterizationRateMap*,
    // rasterizationRateMap);
    // TODO: GETSET_PROPERTY(MTLRenderPassDescriptor,
    // MTLRenderPassSampleBufferAttachmentDescriptorArray*,
    // sampleBufferAttachments);

    // MTLRenderPassAttachmentDescriptor
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, id_MTLTexture, texture);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, level);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, slice);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, depthPlane);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, id_MTLTexture, resolveTexture);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveLevel);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveSlice);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveDepthPlane);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, MTLLoadAction, loadAction);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, MTLStoreAction, storeAction);
    // GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, MTLStoreActionOptions,
    // storeActionOptions);

    // MTLRenderPassDepthAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        id_MTLTexture, texture);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, level);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, slice);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, depthPlane);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        id_MTLTexture, resolveTexture);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, resolveLevel);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, resolveSlice);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, resolveDepthPlane);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        MTLLoadAction, loadAction);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        MTLStoreAction, storeAction);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        MTLStoreActionOptions, storeActionOptions);
    // GETSET_PROPERTY(MTLRenderPassDepthAttachmentDescriptor, double, clearDepth);
    // TODO: MTLMultisampleDepthResolveFilter depthResolveFilter;

    // MTLRenderPassStencilAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        id_MTLTexture, texture);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, level);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, slice);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, depthPlane);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        id_MTLTexture, resolveTexture);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, resolveLevel);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, resolveSlice);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        NSUInteger_objc, resolveDepthPlane);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        MTLLoadAction, loadAction);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        MTLStoreAction, storeAction);
    // GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
    // MTLRenderPassAttachmentDescriptor,
    //                        MTLStoreActionOptions, storeActionOptions);
    // GETSET_PROPERTY(MTLRenderPassStencilAttachmentDescriptor, uint32_t, clearStencil);
    // TODO: MTLMultisampleStencilResolveFilter stencilResolveFilter;
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
    id_MTLDrawable drawable = record->cmdInfo->drawable;

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

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer, mtlCommandBuffer_commit,
                                WrappedMTLCommandBuffer *buffer);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer,
                                mtlCommandBuffer_renderCommandEncoderWithDescriptor,
                                WrappedMTLCommandBuffer *buffer,
                                WrappedMTLRenderCommandEncoder *encoder,
                                MTLRenderPassDescriptor *descriptor);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLCommandBuffer, mtlCommandBuffer_presentDrawable,
                                WrappedMTLCommandBuffer *buffer, id_MTLDrawable drawable);
