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
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::PassBoundary;
      action.flags |= ActionFlags::BeginPass;

      AddAction(action);
    }
    ResourceId liveID;

    MTL::RenderPassDescriptor *mtlDescriptor(descriptor);
    MTL::RenderCommandEncoder *realMTLRenderCommandEncoder =
        Unwrap(CommandBuffer)->renderCommandEncoder(mtlDescriptor);
    mtlDescriptor->release();
    WrappedMTLRenderCommandEncoder *wrappedMTLRenderCommandEncoder;
    liveID = GetResourceManager()->WrapResource(realMTLRenderCommandEncoder,
                                                wrappedMTLRenderCommandEncoder);
    wrappedMTLRenderCommandEncoder->SetCommandBuffer(CommandBuffer);
    if(GetResourceManager()->HasLiveResource(RenderCommandEncoder))
    {
      // TODO: we are leaking the original WrappedMTLRenderCommandEncoder
      GetResourceManager()->EraseLiveResource(RenderCommandEncoder);
    }
    GetResourceManager()->AddLiveResource(RenderCommandEncoder, wrappedMTLRenderCommandEncoder);
    m_Device->AddResource(RenderCommandEncoder, ResourceType::RenderPass, "Render Encoder");
    m_Device->DerivedResource(CommandBuffer, RenderCommandEncoder);

    m_Device->SetRenderPassActiveState(WrappedMTLDevice::Primary, true);
    m_Device->SetActiveRenderCommandEncoder(wrappedMTLRenderCommandEncoder);
    MetalRenderState &renderstate = m_Device->GetCmdRenderState();
    renderstate.Init();
    renderstate.renderPass = liveID;
    MetalCreationInfo &c = m_Device->GetCreationInfo();
    MetalCreationInfo::RenderPass &rp = c.m_RenderPass[liveID];
    rp.colorAttachments.resize(descriptor.colorAttachments.count());
    // Save render stage settings from MTLRenderPassDescriptor
    for(int i = 0; i < descriptor.colorAttachments.count(); ++i)
    {
      RDMTL::RenderPassColorAttachmentDescriptor &mtlColorAttachment = descriptor.colorAttachments[i];
      MetalCreationInfo::RenderPass::ColorAttachment &rdColorAttachment = rp.colorAttachments[i];
      WrappedMTLTexture *texture = mtlColorAttachment.texture;
      rdColorAttachment.texture = GetResID(texture);
      rdColorAttachment.level = mtlColorAttachment.level;
      rdColorAttachment.slice = mtlColorAttachment.slice;
      rdColorAttachment.depthPlane = mtlColorAttachment.depthPlane;
      WrappedMTLTexture *resolveTexture = mtlColorAttachment.resolveTexture;
      rdColorAttachment.resolveTexture = GetResID(resolveTexture);
      rdColorAttachment.resolveLevel = mtlColorAttachment.resolveLevel;
      rdColorAttachment.resolveSlice = mtlColorAttachment.resolveSlice;
      rdColorAttachment.resolveDepthPlane = mtlColorAttachment.resolveDepthPlane;
      rdColorAttachment.loadAction = mtlColorAttachment.loadAction;
      rdColorAttachment.storeAction = mtlColorAttachment.storeAction;
      rdColorAttachment.storeActionOptions = mtlColorAttachment.storeActionOptions;
      rdColorAttachment.clearColor = mtlColorAttachment.clearColor;
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

void WrappedMTLCommandBuffer::enqueue()
{
  SERIALISE_TIME_CALL(Unwrap(this)->enqueue());
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
bool WrappedMTLCommandBuffer::Serialise_waitUntilCompleted(SerialiserType &ser)
{
  SERIALISE_ELEMENT_LOCAL(CommandBuffer, this);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //
    //      ActionDescription action;
    //      action.flags |= ActionFlags::NoFlags;
    //
    //      AddAction(action);
    //    }
    Unwrap(CommandBuffer)->waitUntilCompleted();
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
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::NoFlags;
      action.flags |= ActionFlags::CommandBufferBoundary;

      AddAction(action);
    }
    m_Device->RemovePendingCommandBuffer(CommandBuffer);
    CommandBuffer->commit();
  }
  return true;
}

void WrappedMTLCommandBuffer::commit()
{
  SERIALISE_TIME_CALL(Unwrap(this)->commit());
  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *bufferRecord = GetRecord(this);
    bool capframe = IsActiveCapturing(m_State);
    if(capframe)
    {
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
      m_Device->AddCommandBufferRecord(bufferRecord);
    }
    bool present = bufferRecord->cmdInfo->present;
    if(present)
    {
      m_Device->AdvanceFrame();
      m_Device->Present(bufferRecord->cmdInfo->backBuffer, bufferRecord->cmdInfo->outputLayer);
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
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, commit);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLCommandBuffer, void, waitUntilCompleted);
