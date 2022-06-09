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

#include "metal_core.h"
#include "serialise/rdcfile.h"

#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_command_queue.h"
#include "metal_device.h"
#include "metal_helpers_bridge.h"
#include "metal_library.h"
#include "metal_render_command_encoder.h"
#include "metal_replay.h"
#include "metal_resources.h"
#include "metal_texture.h"

WriteSerialiser &WrappedMTLDevice::GetThreadSerialiser()
{
  WriteSerialiser *ser = (WriteSerialiser *)Threading::GetTLSValue(threadSerialiserTLSSlot);
  if(ser)
    return *ser;

  // slow path, but rare
  ser = new WriteSerialiser(new StreamWriter(1024), Ownership::Stream);

  uint32_t flags = WriteSerialiser::ChunkDuration | WriteSerialiser::ChunkTimestamp |
                   WriteSerialiser::ChunkThreadID;

  if(RenderDoc::Inst().GetCaptureOptions().captureCallstacks)
    flags |= WriteSerialiser::ChunkCallstack;

  ser->SetChunkMetadataRecording(flags);
  ser->SetUserData(GetResourceManager());
  ser->SetVersion(MetalInitParams::CurrentVersion);

  Threading::SetTLSValue(threadSerialiserTLSSlot, (void *)ser);

  {
    SCOPED_LOCK(m_ThreadSerialisersLock);
    m_ThreadSerialisers.push_back(ser);
  }

  return *ser;
}

const APIEvent &WrappedMTLDevice::GetEvent(uint32_t eventId)
{
  // start at where the requested eventId would be
  size_t idx = eventId;

  // find the next valid event (some may be skipped)
  while(idx < m_Events.size() - 1 && m_Events[idx].eventId == 0)
    idx++;

  return m_Events[RDCMIN(idx, m_Events.size() - 1)];
}

const ActionDescription *WrappedMTLDevice::GetAction(uint32_t eventId)
{
  if(eventId >= m_Actions.size())
    return NULL;

  return m_Actions[eventId];
}

void WrappedMTLDevice::AddAction(const ActionDescription &a)
{
  bool cmdBufferActive = m_ReplayCurrentCmdBufferID != ResourceId();
  m_AddedAction = true;

  ActionDescription action = a;
  action.eventId = cmdBufferActive ? m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].curEventID
                                   : m_RootEventID;
  action.actionId = cmdBufferActive ? m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].actionCount
                                    : m_RootActionID;

  for(uint32_t i = 0; i < 8; i++)
    action.outputs[i] = ResourceId();

  action.depthOut = ResourceId();

  /*
  if(cmdBufferActive)
   {
      ResourceId fb = m_ReplayCmdBufferInfos[cmdBufferId].state.GetFramebuffer();
      ResourceId rp = m_ReplayCmdBufferInfos[cmdBufferId].state.renderPass;
      uint32_t sp = m_ReplayCmdBufferInfos[cmdBufferId].state.subpass;

      if(fb != ResourceId() && rp != ResourceId())
      {
        const rdcarray<ResourceId> &atts =
            m_BakedCmdBufferInfo[m_ReplayCurrentCmdBufferID].state.GetFramebufferAttachments();

        RDCASSERT(sp < m_CreationInfo.m_RenderPass[rp].subpasses.size());

        rdcarray<uint32_t> &colAtt = m_CreationInfo.m_RenderPass[rp].subpasses[sp].colorAttachments;
        int32_t dsAtt = m_CreationInfo.m_RenderPass[rp].subpasses[sp].depthstencilAttachment;

        RDCASSERT(colAtt.size() <= ARRAY_COUNT(action.outputs));

        for(size_t i = 0; i < ARRAY_COUNT(action.outputs) && i < colAtt.size(); i++)
        {
          if(colAtt[i] == VK_ATTACHMENT_UNUSED)
            continue;

          RDCASSERT(colAtt[i] < atts.size());
          action.outputs[i] =
              GetResourceManager()->GetOriginalID(m_CreationInfo.m_ImageView[atts[colAtt[i]]].image);
        }

        if(dsAtt != -1)
        {
          RDCASSERT(dsAtt < (int32_t)atts.size());
          action.depthOut =
              GetResourceManager()->GetOriginalID(m_CreationInfo.m_ImageView[atts[dsAtt]].image);
        }
      }
    }
   */

  // markers don't increment action ID
  ActionFlags MarkerMask = ActionFlags::SetMarker | ActionFlags::PushMarker |
                           ActionFlags::PopMarker | ActionFlags::PassBoundary;
  if(!(action.flags & MarkerMask))
  {
    if(cmdBufferActive)
      m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].actionCount++;
    else
      m_RootActionID++;
  }

  action.events.swap(cmdBufferActive ? m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].curEvents
                                     : m_RootEvents);
  for(int i = 0; i < m_RootEvents.count(); ++i)
  {
    APIEvent &e = m_RootEvents[i];
    RDCLOG("R Root[%d] %d %d %lu", i, e.eventId, e.chunkIndex, e.fileOffset);
  }

  // should have at least the root action here, push this action
  // onto the back's children list.
  if(!GetActionStack().empty())
  {
    MetalActionTreeNode node(action);

    /*
    node.resourceUsage.swap(m_BakedCmdBufferInfo[m_ReplayCurrentCmdBufferID].resourceUsage);

    if(cmdBufferActive)
      AddUsage(node, m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].debugMessages);
    */
    node.children.reserve(action.children.size());
    for(const ActionDescription &child : action.children)
      node.children.push_back(MetalActionTreeNode(child));

    GetActionStack().back()->children.push_back(node);
  }
  else
    RDCERR("Somehow lost action stack!");
}

void WrappedMTLDevice::AddEvent()
{
  bool cmdBufferActive = m_ReplayCurrentCmdBufferID != ResourceId();
  APIEvent apievent;

  apievent.fileOffset = m_CurChunkOffset;
  apievent.eventId = cmdBufferActive ? m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].curEventID
                                     : m_RootEventID;

  apievent.chunkIndex = uint32_t(m_StructuredFile->chunks.size() - 1);

  for(size_t i = 0; i < m_EventMessages.size(); i++)
    m_EventMessages[i].eventId = apievent.eventId;

  if(cmdBufferActive)
  {
    m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].curEvents.push_back(apievent);
    //    m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].debugMessages.append(m_EventMessages);
  }
  else
  {
    m_RootEvents.push_back(apievent);
    m_Events.resize(apievent.eventId + 1);
    m_Events[apievent.eventId] = apievent;

    m_DebugMessages.append(m_EventMessages);
  }

  m_EventMessages.clear();
}

bool WrappedMTLDevice::ContextProcessChunk(ReadSerialiser &ser, MetalChunk chunk)
{
  m_AddedAction = false;

  bool success = ProcessChunk(ser, chunk);

  if(!success)
    return false;

  if(IsLoading(m_State))
  {
    if((chunk == MetalChunk::MTLCommandQueue_commandBuffer) ||
       (chunk == MetalChunk::MTLCommandBuffer_enqueue))
    {
      // don't add these events - they will be handled when inserted in-line into queue submit
    }
    else
    {
      if(!m_AddedAction)
        AddEvent();
    }
  }

  m_AddedAction = false;

  return true;
}

#define METAL_CHUNK_NOT_HANDLED(CHUNK)            \
  case MetalChunk::CHUNK:                         \
  {                                               \
    RDCERR("MetalChunk::" #CHUNK " not handled"); \
    return false;                                 \
  }

bool WrappedMTLDevice::ProcessChunk(ReadSerialiser &ser, MetalChunk chunk)
{
  switch(chunk)
  {
    case MetalChunk::MTLCreateSystemDefaultDevice:
      return Serialise_MTLCreateSystemDefaultDevice(ser);
    case MetalChunk::MTLDevice_newCommandQueue:
      return Serialise_newCommandQueue(ser, NULL);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newCommandQueueWithMaxCommandBufferCount);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newHeapWithDescriptor);
    case MetalChunk::MTLDevice_newBufferWithLength:
    case MetalChunk::MTLDevice_newBufferWithBytes:
      return Serialise_newBufferWithBytes(ser, NULL, NULL, 0, MTL::ResourceOptionCPUCacheModeDefault);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newBufferWithBytesNoCopy);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newDepthStencilStateWithDescriptor);
    case MetalChunk::MTLDevice_newTextureWithDescriptor:
    case MetalChunk::MTLDevice_newTextureWithDescriptor_iosurface:
    case MetalChunk::MTLDevice_newTextureWithDescriptor_nextDrawable:
    {
      RDMTL::TextureDescriptor descriptor;
      return Serialise_newTextureWithDescriptor(ser, NULL, descriptor);
    }
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newSharedTextureWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newSharedTextureWithHandle);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newSamplerStateWithDescriptor);
    case MetalChunk::MTLDevice_newDefaultLibrary:
      return Serialise_newDefaultLibrary(ser, NULL);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newDefaultLibraryWithBundle);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newLibraryWithFile);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newLibraryWithURL);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newLibraryWithData);
    case MetalChunk::MTLDevice_newLibraryWithSource:
      return Serialise_newLibraryWithSource(ser, NULL, NULL, NULL, NULL);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newLibraryWithStitchedDescriptor);
    case MetalChunk::MTLDevice_newRenderPipelineStateWithDescriptor:
    {
      RDMTL::RenderPipelineDescriptor descriptor;
      return Serialise_newRenderPipelineStateWithDescriptor(ser, NULL, descriptor, NULL);
    }
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newRenderPipelineStateWithDescriptor_options);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newComputePipelineStateWithFunction);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newComputePipelineStateWithFunction_options);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newComputePipelineStateWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newFence);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newRenderPipelineStateWithTileDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newArgumentEncoderWithArguments);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_supportsRasterizationRateMapWithLayerCount);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newRasterizationRateMapWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newIndirectCommandBufferWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newEvent);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newSharedEvent);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newSharedEventWithHandle);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newCounterSampleBufferWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newDynamicLibrary);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newDynamicLibraryWithURL);
      METAL_CHUNK_NOT_HANDLED(MTLDevice_newBinaryArchiveWithDescriptor);

    case MetalChunk::MTLLibrary_newFunctionWithName:
      return m_DummyReplayLibrary->Serialise_newFunctionWithName(ser, NULL, NULL);
      METAL_CHUNK_NOT_HANDLED(MTLLibrary_newFunctionWithName_constantValues);
      METAL_CHUNK_NOT_HANDLED(MTLLibrary_newFunctionWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLLibrary_newIntersectionFunctionWithDescriptor);

      METAL_CHUNK_NOT_HANDLED(MTLFunction_newArgumentEncoderWithBufferIndex);

    case MetalChunk::MTLCommandQueue_commandBuffer:
      return m_DummyReplayCommandQueue->Serialise_commandBuffer(ser, NULL);
      METAL_CHUNK_NOT_HANDLED(MTLCommandQueue_commandBufferWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLCommandQueue_commandBufferWithUnretainedReferences);
    case MetalChunk::MTLCommandBuffer_enqueue:
      return m_DummyReplayCommandBuffer->Serialise_enqueue(ser);
    case MetalChunk::MTLCommandBuffer_commit:
      return m_DummyReplayCommandBuffer->Serialise_commit(ser);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_addScheduledHandler);
    case MetalChunk::MTLCommandBuffer_presentDrawable:
      return m_DummyReplayCommandBuffer->Serialise_presentDrawable(ser, NULL);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_presentDrawable_atTime);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_presentDrawable_afterMinimumDuration);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_waitUntilScheduled);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_addCompletedHandler);
    case MetalChunk::MTLCommandBuffer_waitUntilCompleted:
      return m_DummyReplayCommandBuffer->Serialise_waitUntilCompleted(ser);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_blitCommandEncoder);
    case MetalChunk::MTLCommandBuffer_renderCommandEncoderWithDescriptor:
    {
      RDMTL::RenderPassDescriptor descriptor;
      return m_DummyReplayCommandBuffer->Serialise_renderCommandEncoderWithDescriptor(ser, NULL,
                                                                                      descriptor);
    }
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_computeCommandEncoderWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_blitCommandEncoderWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_computeCommandEncoder);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_computeCommandEncoderWithDispatchType);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_encodeWaitForEvent);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_encodeSignalEvent);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_parallelRenderCommandEncoderWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_resourceStateCommandEncoder);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_resourceStateCommandEncoderWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_accelerationStructureCommandEncoder);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_pushDebugGroup);
      METAL_CHUNK_NOT_HANDLED(MTLCommandBuffer_popDebugGroup);

      METAL_CHUNK_NOT_HANDLED(MTLTexture_setPurgeableState);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_makeAliasable);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_getBytes);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_getBytes_slice);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_replaceRegion);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_replaceRegion_slice);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_newTextureViewWithPixelFormat);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_newTextureViewWithPixelFormat_subset);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_newTextureViewWithPixelFormat_subset_swizzle);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_newSharedTextureHandle);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_remoteStorageTexture);
      METAL_CHUNK_NOT_HANDLED(MTLTexture_newRemoteTextureViewForDevice);

      METAL_CHUNK_NOT_HANDLED(MTLRenderPipelineState_functionHandleWithFunction);
      METAL_CHUNK_NOT_HANDLED(MTLRenderPipelineState_newVisibleFunctionTableWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLRenderPipelineState_newIntersectionFunctionTableWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(
          MTLRenderPipelineState_newRenderPipelineStateWithAdditionalBinaryFunctions);

    case MetalChunk::MTLRenderCommandEncoder_endEncoding:
      return m_DummyReplayRenderCommandEncoder->Serialise_endEncoding(ser);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_insertDebugSignpost);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_pushDebugGroup);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_popDebugGroup);
    case MetalChunk::MTLRenderCommandEncoder_setRenderPipelineState:
      return m_DummyReplayRenderCommandEncoder->Serialise_setRenderPipelineState(ser, NULL);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexBytes);
    case MetalChunk::MTLRenderCommandEncoder_setVertexBuffer:
      return m_DummyReplayRenderCommandEncoder->Serialise_setVertexBuffer(ser, NULL, 0, 0);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexBufferOffset);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexBuffers);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexTexture);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexTextures);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexSamplerState);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexSamplerState_lodclamp);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexSamplerStates);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexSamplerStates_lodclamp);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexVisibleFunctionTable);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexVisibleFunctionTables);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexIntersectionFunctionTable);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexIntersectionFunctionTables);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexAccelerationStructure);
    case MetalChunk::MTLRenderCommandEncoder_setViewport:
    {
      MTL::Viewport viewport;
      return m_DummyReplayRenderCommandEncoder->Serialise_setViewport(ser, viewport);
    }
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setViewports);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFrontFacingWinding);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVertexAmplificationCount);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setCullMode);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setDepthClipMode);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setDepthBias);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setScissorRect);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setScissorRects);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTriangleFillMode);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentBytes);
    case MetalChunk::MTLRenderCommandEncoder_setFragmentBuffer:
      return m_DummyReplayRenderCommandEncoder->Serialise_setFragmentBuffer(ser, NULL, 0, 0);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentBufferOffset);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentBuffers);
    case MetalChunk::MTLRenderCommandEncoder_setFragmentTexture:
      return m_DummyReplayRenderCommandEncoder->Serialise_setFragmentTexture(ser, NULL, 0);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentTextures);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentSamplerState);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentSamplerState_lodclamp);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentSamplerStates);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentSamplerStates_lodclamp);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentVisibleFunctionTable);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentVisibleFunctionTables);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentIntersectionFunctionTable);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentIntersectionFunctionTables);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setFragmentAccelerationStructure);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setBlendColor);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setDepthStencilState);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setStencilReferenceValue);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setStencilFrontReferenceValue);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setVisibilityResultMode);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setColorStoreAction);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setDepthStoreAction);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setStencilStoreAction);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setColorStoreActionOptions);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setDepthStoreActionOptions);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setStencilStoreActionOptions);
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives:
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives_instanced:
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives_instanced_base:
      return m_DummyReplayRenderCommandEncoder->Serialise_drawPrimitives(
          ser, MTL::PrimitiveTypePoint, 0, 0, 0, 0);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawPrimitives_indirect);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawIndexedPrimitives);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawIndexedPrimitives_instanced);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawIndexedPrimitives_instanced_base);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawIndexedPrimitives_indirect);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_textureBarrier);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_updateFence);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_waitForFence);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTessellationFactorBuffer);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTessellationFactorScale);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawPatches);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawPatches_indirect);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawIndexedPatches);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_drawIndexedPatches_indirect);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileBytes);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileBuffer);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileBufferOffset);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileBuffers);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileTexture);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileTextures);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileSamplerState);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileSamplerState_lodclamp);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileSamplerStates);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileSamplerStates_lodclamp);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileVisibleFunctionTable);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileVisibleFunctionTables);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileIntersectionFunctionTable);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileIntersectionFunctionTables);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setTileAccelerationStructure);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_dispatchThreadsPerTile);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_setThreadgroupMemoryLength);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useResource);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useResource_stages);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useResources);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useResources_stages);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useHeap);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useHeap_stages);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useHeaps);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_useHeaps_stages);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_executeCommandsInBuffer);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_executeCommandsInBuffer_indirect);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_memoryBarrierWithScope);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_memoryBarrierWithResources);
      METAL_CHUNK_NOT_HANDLED(MTLRenderCommandEncoder_sampleCountersInBuffer);

      METAL_CHUNK_NOT_HANDLED(MTLBuffer_setPurgeableState);
      METAL_CHUNK_NOT_HANDLED(MTLBuffer_makeAliasable);
      METAL_CHUNK_NOT_HANDLED(MTLBuffer_contents);
    case MetalChunk::MTLBuffer_didModifyRange:
    {
      NS::Range range = NS::Range::Make(0, 0);
      return m_DummyBuffer->Serialise_didModifyRange(ser, range);
    }
      METAL_CHUNK_NOT_HANDLED(MTLBuffer_newTextureWithDescriptor);
      METAL_CHUNK_NOT_HANDLED(MTLBuffer_addDebugMarker);
      METAL_CHUNK_NOT_HANDLED(MTLBuffer_removeAllDebugMarkers);
      METAL_CHUNK_NOT_HANDLED(MTLBuffer_remoteStorageBuffer);
      METAL_CHUNK_NOT_HANDLED(MTLBuffer_newRemoteBufferViewForDevice);
    case MetalChunk::MTLBuffer_InternalModifyCPUContents:
      return m_DummyBuffer->Serialise_InternalModifyCPUContents(ser, 0, 0);

    // no explicit default so that we have compiler warnings if a chunk isn't explicitly handled.
    case MetalChunk::Max: break;
  }

  {
    SystemChunk system = (SystemChunk)chunk;
    if(system == SystemChunk::DriverInit)
    {
      MetalInitParams InitParams;
      SERIALISE_ELEMENT(InitParams);

      SERIALISE_CHECK_READ_ERRORS();
    }
    else if(system == SystemChunk::InitialContentsList)
    {
      GetResourceManager()->CreateInitialContents(ser);

      if(initStateCurCmd != NULL)
      {
        CloseInitStateCmd();
        //        SubmitAndFlushImageStateBarriers(m_setupImageBarriers);
        SubmitCmds();
        FlushQ();
        //        SubmitAndFlushImageStateBarriers(m_cleanupImageBarriers);
      }

      SERIALISE_CHECK_READ_ERRORS();
    }
    else if(system == SystemChunk::InitialContents)
    {
      return Serialise_InitialState(ser, ResourceId(), NULL, NULL);
    }
    else if(system == SystemChunk::CaptureScope)
    {
      return Serialise_CaptureScope(ser);
    }
    else if(system == SystemChunk::CaptureEnd)
    {
      SERIALISE_ELEMENT_LOCAL(PresentedImage, ResourceId()).TypedAs("MTLTexture"_lit);

      SERIALISE_CHECK_READ_ERRORS();

      if(PresentedImage != ResourceId())
        m_LastPresentedImage = PresentedImage;

      if(IsLoading(m_State))
      {
        AddEvent();

        ActionDescription action;
        action.customName = "End of Capture";
        action.flags |= ActionFlags::Present;
        action.copyDestination = m_LastPresentedImage;
        AddAction(action);
      }
      return true;
    }
    else if(system < SystemChunk::FirstDriverChunk)
    {
      RDCERR("Unexpected system chunk in capture data: %u", system);
      ser.SkipCurrentChunk();

      SERIALISE_CHECK_READ_ERRORS();
    }
    else
    {
      RDCERR("Unrecognised Chunk type %d", chunk);
      return false;
    }
  }

  return true;
}

RDResult WrappedMTLDevice::ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers)
{
  int sectionIdx = rdc->SectionIndex(SectionType::FrameCapture);

  GetResourceManager()->SetState(m_State);

  if(sectionIdx < 0)
    return ResultCode::FileCorrupted;

  StreamReader *reader = rdc->ReadSection(sectionIdx);

  if(IsStructuredExporting(m_State))
  {
    // when structured exporting don't do any timebase conversion
    m_TimeBase = 0;
    m_TimeFrequency = 1.0;
  }
  else
  {
    m_TimeBase = rdc->GetTimestampBase();
    m_TimeFrequency = rdc->GetTimestampFrequency();
  }

  if(reader->IsErrored())
  {
    delete reader;
    return ResultCode::FileIOFailed;
  }

  ReadSerialiser ser(reader, Ownership::Stream);

  ser.SetStringDatabase(&m_StringDB);
  ser.SetUserData(GetResourceManager());

  ser.ConfigureStructuredExport(&GetChunkName, storeStructuredBuffers, m_TimeBase, m_TimeFrequency);

  m_StructuredFile = &ser.GetStructuredFile();

  m_StoredStructuredData.version = m_StructuredFile->version = m_SectionVersion;

  ser.SetVersion(m_SectionVersion);

  uint64_t chunkIdx = 0;

  struct chunkinfo
  {
    chunkinfo() : count(0), totalsize(0), total(0.0) {}
    uint64_t count;
    uint64_t totalsize;
    double total;
  };

  std::map<MetalChunk, chunkinfo> chunkInfos;

  SCOPED_TIMER("chunk initialisation");

  uint64_t frameDataSize = 0;

  for(;;)
  {
    PerformanceTimer timer;

    uint64_t offsetStart = reader->GetOffset();

    MetalChunk context = ser.ReadChunk<MetalChunk>();

    chunkIdx++;

    if(reader->IsErrored())
    {
      return RDResult(ResultCode::APIDataCorrupted, ser.GetError().message);
    }

    bool success = ProcessChunk(ser, context);

    ser.EndChunk();

    if(reader->IsErrored())
    {
      return RDResult(ResultCode::APIDataCorrupted, ser.GetError().message);
    }

    // if there wasn't a serialisation error, but the chunk didn't succeed, then it's an API replay
    // failure.
    if(!success)
    {
      return m_FailedReplayResult;
    }

    uint64_t offsetEnd = reader->GetOffset();

    // TODO: m_DebugManager support
    if(IsStructuredExporting(m_State))
    {
      RenderDoc::Inst().SetProgress(LoadProgress::FileInitialRead,
                                    float(offsetEnd) / float(reader->GetSize()));
    }

    if((SystemChunk)context == SystemChunk::CaptureScope)
    {
      GetReplay()->WriteFrameRecord().frameInfo.fileOffset = offsetStart;

      // read the remaining data into memory and pass to immediate context
      frameDataSize = reader->GetSize() - reader->GetOffset();

      m_FrameReader = new StreamReader(reader, frameDataSize);

      RDResult status = ContextReplayLog(m_State, 0, 0, false);
      if(status != ResultCode::Succeeded)
      {
        return status;
      }
    }

    chunkInfos[context].total += timer.GetMilliseconds();
    chunkInfos[context].totalsize += offsetEnd - offsetStart;
    chunkInfos[context].count++;

    if((SystemChunk)context == SystemChunk::CaptureScope || reader->IsErrored() || reader->AtEnd())
      break;
  }

#if ENABLED(RDOC_DEVEL)
  for(auto it = chunkInfos.begin(); it != chunkInfos.end(); ++it)
  {
    double dcount = double(it->second.count);

    RDCDEBUG(
        "% 5d chunks - Time: %9.3fms total/%9.3fms avg - Size: %8.3fMB total/%7.3fMB avg - %s (%u)",
        it->second.count, it->second.total, it->second.total / dcount,
        double(it->second.totalsize) / (1024.0 * 1024.0),
        double(it->second.totalsize) / (dcount * 1024.0 * 1024.0),
        GetChunkName((uint32_t)it->first).c_str(), uint32_t(it->first));
  }
#endif

  // steal the structured data for ourselves
  m_StructuredFile->Swap(m_StoredStructuredData);

  // and in future use this file.
  m_StructuredFile = &m_StoredStructuredData;

  GetReplay()->WriteFrameRecord().frameInfo.uncompressedFileSize =
      rdc->GetSectionProperties(sectionIdx).uncompressedSize;
  GetReplay()->WriteFrameRecord().frameInfo.compressedFileSize =
      rdc->GetSectionProperties(sectionIdx).compressedSize;
  GetReplay()->WriteFrameRecord().frameInfo.persistentSize = frameDataSize;
  GetReplay()->WriteFrameRecord().frameInfo.initDataSize =
      chunkInfos[(MetalChunk)SystemChunk::InitialContents].totalsize;

  RDCDEBUG("Allocating %llu persistant bytes of memory for the log.",
           GetReplay()->WriteFrameRecord().frameInfo.persistentSize);

  // ensure the capture at least created a device
  if(!IsStructuredExporting(m_State))
  {
    // TODO: implement RD MTL replay
    RDCASSERT(m_Real != NULL);
  }

  // TODO: FreeAllMemory(MemoryScope::IndirectReadback);

  return ResultCode::Succeeded;
}

void WrappedMTLDevice::ApplyInitialContents()
{
  RENDERDOC_PROFILEFUNCTION();
  if(HasFatalError())
    return;

  //  VkMarkerRegion region("ApplyInitialContents");

  initStateCurBatch = 0;
  initStateCurCmd = NULL;

  // check that we have all external queues necessary
  //  for(size_t i = 0; i < m_ExternalQueues.size(); i++)
  //  {
  //    // if we created a pool (so this is a queue family we're using) but
  //    // didn't get a queue at all, fetch our own queue for this family
  //    if(m_ExternalQueues[i].queue != VK_NULL_HANDLE || m_ExternalQueues[i].pool ==
  //    VK_NULL_HANDLE)
  //      continue;
  //
  //    VkQueue queue;
  //
  //    ObjDisp(m_Device)->GetDeviceQueue(Unwrap(m_Device), (uint32_t)i, 0, &queue);
  //
  //    GetResourceManager()->WrapResource(Unwrap(m_Device), queue);
  //    GetResourceManager()->AddLiveResource(ResourceIDGen::GetNewUniqueID(), queue);
  //
  //    m_ExternalQueues[i].queue = queue;
  //  }

  // add a global memory barrier to ensure all writes have finished and are synchronised
  // add memory barrier to ensure this copy completes before any subsequent work
  // this is a very blunt instrument but it ensures we don't get random artifacts around
  // frame restart where we may be skipping a lot of important synchronisation
  //  VkMemoryBarrier memBarrier = {
  //      VK_STRUCTURE_TYPE_MEMORY_BARRIER, NULL, VK_ACCESS_ALL_WRITE_BITS, VK_ACCESS_ALL_READ_BITS,
  //  };

  WrappedMTLCommandBuffer *cmd = GetNextCommandBuffer();
  if(cmd == NULL)
    return;

  // DoPipelineBarrier(cmd, 1, &memBarrier);

  // sync all GPU work so we can also apply descriptor set initial contents
  SubmitCmds();
  FlushQ();

  // actually apply the initial contents here
  GetResourceManager()->ApplyInitialContents();

  // close the final command buffer
  if(initStateCurCmd != NULL)
  {
    CloseInitStateCmd();
  }

  initStateCurBatch = 0;
  initStateCurCmd = NULL;

  for(auto it = m_TextureStates.begin(); it != m_TextureStates.end(); ++it)
  {
    if(GetResourceManager()->HasCurrentResource(it->first))
    {
      //      it->second.LockWrite()->ResetToOldState(m_cleanupImageBarriers,
      //      GetImageTransitionInfo());
    }
    else
    {
      it = m_TextureStates.erase(it);
      --it;
    }
  }

  // likewise again to make sure the initial states are all applied
  cmd = GetNextCommandBuffer();
  // DoPipelineBarrier(cmd, 1, &memBarrier);

  //  SubmitAndFlushImageStateBarriers(m_setupImageBarriers);
  SubmitCmds();
  FlushQ();
  //  SubmitAndFlushImageStateBarriers(m_cleanupImageBarriers);

  SubmitCmds();
  FlushQ();
}

RDResult WrappedMTLDevice::ContextReplayLog(CaptureState readType, uint32_t startEventID,
                                            uint32_t endEventID, bool partial)
{
  RDCLOG("M J Replay %d->%d %s", startEventID, endEventID, ToStr(partial).c_str());
  m_FrameReader->SetOffset(0);

  ReadSerialiser ser(m_FrameReader, Ownership::Nothing);

  ser.SetStringDatabase(&m_StringDB);
  ser.SetUserData(GetResourceManager());
  ser.SetVersion(m_SectionVersion);

  SDFile *prevFile = m_StructuredFile;

  if(IsLoading(m_State) || IsStructuredExporting(m_State))
  {
    ser.ConfigureStructuredExport(&GetChunkName, IsStructuredExporting(m_State), m_TimeBase,
                                  m_TimeFrequency);

    ser.GetStructuredFile().Swap(*m_StructuredFile);

    m_StructuredFile = &ser.GetStructuredFile();
  }

  SystemChunk header = ser.ReadChunk<SystemChunk>();
  RDCASSERTEQUAL(header, SystemChunk::CaptureBegin);

  if(partial)
  {
    ser.SkipCurrentChunk();
  }
  else
  {
#if ENABLED(RDOC_RELEASE)
    if(IsLoading(m_State))
      Serialise_BeginCaptureFrame(ser);
    else
      ser.SkipCurrentChunk();
#else
    Serialise_BeginCaptureFrame(ser);
#endif
  }

  ser.EndChunk();

  if(!IsStructuredExporting(m_State))
    WaitForGPU();

  // apply initial contents here so that textures are in the right layout
  // (not undefined)
  if(IsLoading(m_State))
  {
    ApplyInitialContents();
  }

  m_RootEvents.clear();

  if(IsActiveReplaying(m_State))
  {
    APIEvent ev = GetEvent(startEventID);
    m_RootEventID = ev.eventId;

    // if not partial, we need to be sure to replay
    // past the command buffer records, so can't
    // skip to the file offset of the first event
    if(partial)
      ser.GetReader()->SetOffset(ev.fileOffset);
    else
      m_ReplayPartialCmdBufferID = ResourceId();

    m_FirstEventID = startEventID;
    m_LastEventID = endEventID;

    // when selecting a marker we can get into an inconsistent state -
    // make sure that we make things consistent again here, replay the event
    // that we ended up selecting (the one that was closest)
    if(startEventID == endEventID && m_RootEventID != m_FirstEventID)
      m_FirstEventID = m_LastEventID = m_RootEventID;
  }
  else
  {
    m_RootEventID = 1;
    m_RootActionID = 1;
    m_FirstEventID = 0;
    m_LastEventID = ~0U;
  }

  uint64_t startOffset = ser.GetReader()->GetOffset();

  for(;;)
  {
    if(IsActiveReplaying(m_State) && m_RootEventID > endEventID)
    {
      // we can just break out if we've done all the events desired.
      // note that the command buffer events aren't 'real' and we just blaze through them
      break;
    }

    m_CurChunkOffset = ser.GetReader()->GetOffset();

    MetalChunk chunk = ser.ReadChunk<MetalChunk>();

    if(ser.GetReader()->IsErrored())
      return ResultCode::APIDataCorrupted;

    m_ChunkMetadata = ser.ChunkMetadata();

    m_ReplayCurrentCmdBufferID = ResourceId();

    RDCLOG("J %d '%s' %lu", m_RootEventID, ToStr(chunk).c_str(), m_CurChunkOffset);
    bool success = ContextProcessChunk(ser, chunk);

    ser.EndChunk();

    if(ser.GetReader()->IsErrored())
      return ResultCode::APIDataCorrupted;

    // if there wasn't a serialisation error, but the chunk didn't succeed, then it's an API replay
    // failure.
    if(!success)
    {
      return m_FailedReplayResult;
    }

    RDCLOG("J Cmd %s %d", ToStr(m_ReplayCurrentCmdBufferID).c_str(),
           m_ReplayCurrentCmdBufferID == ResourceId()
               ? -1
               : m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].curEventID);

    RenderDoc::Inst().SetProgress(
        LoadProgress::FrameEventsRead,
        float(m_CurChunkOffset - startOffset) / float(ser.GetReader()->GetSize()));

    if((SystemChunk)chunk == SystemChunk::CaptureEnd || ser.GetReader()->AtEnd())
      break;

    // break out if we were only executing one event
    if(IsActiveReplaying(m_State) && startEventID == endEventID)
      break;

    m_LastChunk = chunk;

    // increment root event ID either if we didn't just replay a cmd
    // buffer event, OR if we are doing a frame sub-section replay,
    if(m_ReplayCurrentCmdBufferID == ResourceId() || startEventID > 1)
    {
      m_RootEventID++;

      if(startEventID > 1)
        ser.GetReader()->SetOffset(GetEvent(m_RootEventID).fileOffset);
    }
    else
    {
      // these events are completely omitted, so don't increment the curEventID
      if((chunk != MetalChunk::MTLCommandQueue_commandBuffer) &&
         (chunk != MetalChunk::MTLCommandBuffer_enqueue))
        m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].curEventID++;
    }
  }

  // Save the current render state of the partial command buffer.
  if(m_ReplayPartialCmdBufferID != ResourceId())
  {
    m_RenderState = m_ReplayCmdBufferInfos[m_ReplayPartialCmdBufferID].renderState;
  }

  // swap the structure back now that we've accumulated the frame as well.
  if(IsLoading(m_State) || IsStructuredExporting(m_State))
    ser.GetStructuredFile().Swap(*prevFile);

  m_StructuredFile = prevFile;

  if(IsLoading(m_State))
  {
    GetReplay()->WriteFrameRecord().actionList = m_ParentAction.Bake();

    SetupActionPointers(m_Actions, GetReplay()->WriteFrameRecord().actionList);

    m_ParentAction.children.clear();
  }
  if(!IsStructuredExporting(m_State))
  {
    WaitForGPU();

    /*
      // destroy any events we created for waiting on
      for(size_t i = 0; i < m_CleanupEvents.size(); i++)
        ObjDisp(GetDev())->DestroyEvent(Unwrap(GetDev()), m_CleanupEvents[i], NULL);

      for(const rdcpair<VkCommandPool, VkCommandBuffer> &rerecord : m_RerecordCmdList)
        vkFreeCommandBuffers(GetDev(), rerecord.first, 1, &rerecord.second);
    */
  }

  /*
   // submit the indirect preparation command buffer, if we need to
    if(m_IndirectDraw)
    {
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,
          m_SubmitChain,
          0,
          NULL,
          NULL,    // wait semaphores
          1,
          UnwrapPtr(m_IndirectCommandBuffer),    // command buffers
          0,
          NULL,    // signal semaphores
      };

      VkResult vkr = ObjDisp(m_Queue)->QueueSubmit(Unwrap(m_Queue), 1, &submitInfo, VK_NULL_HANDLE);
      RDCASSERTEQUAL(vkr, VK_SUCCESS);
    }

    m_IndirectDraw = false;

    m_CleanupEvents.clear();

    m_RerecordCmds.clear();
    m_RerecordCmdList.clear();
   */

  return ResultCode::Succeeded;
}

void WrappedMTLDevice::ReplayLog(uint32_t startEventID, uint32_t endEventID, ReplayLogType replayType)
{
  bool partial = true;

  if(startEventID == 0 && (replayType == eReplay_WithoutDraw || replayType == eReplay_Full))
  {
    startEventID = 1;
    partial = false;
  }

  if(!partial)
  {
    ApplyInitialContents();
  }

  m_State = CaptureState::ActiveReplaying;

  {
    if(!partial)
    {
      m_RenderState = MetalRenderState();
      for(auto it = m_ReplayCmdBufferInfos.begin(); it != m_ReplayCmdBufferInfos.end(); it++)
        it->second.renderState = MetalRenderState();
    }
    else
    {
      // Copy the state in case m_RenderState was modified externally for the partial replay.
      if(m_ReplayPartialCmdBufferID != ResourceId())
        m_ReplayCmdBufferInfos[m_ReplayPartialCmdBufferID].renderState = m_RenderState;
    }

    bool rpWasActive = false;

    // we'll need our own command buffer if we're replaying just a subsection
    // of events within a single command buffer record - always if it's only
    // one action, or if start event ID is > 0 we assume the outside code
    // has chosen a subsection that lies within a command buffer
    if(partial)
    {
      WrappedMTLCommandBuffer *cmdBuffer = m_ReplayPartialCmdBuffer = GetNextCommandBuffer();

      if(cmdBuffer == NULL)
        return;

      if(m_ReplayPartialCmdBufferID != ResourceId())
      {
        ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayPartialCmdBufferID];
        rpWasActive = cmdBufInfo.renderPassOpen;
        if(rpWasActive)
        {
          const ActionDescription *action = GetAction(endEventID);

          bool rpUnneeded = false;

          // if we're only replaying an action, and it's not an draw or dispatch, don't try and bind
          // all the replay state as we don't know if it will be valid.
          if(replayType == eReplay_OnlyDraw)
          {
            if(!action)
            {
              rpUnneeded = true;
            }
            else if(!(action->flags & (ActionFlags::Drawcall | ActionFlags::Dispatch)))
            {
              rpUnneeded = true;
            }
          }

          // if a render pass was active, begin it and set up the partial replay state
          m_RenderState.BeginRenderPassAndApplyState(
              this, cmdBuffer,
              rpUnneeded ? MetalRenderState::BindNone : MetalRenderState::BindGraphics);
          RDCLOG(" S Begin %p", m_RenderState.renderCommandEncoder);
          RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
          cmdBufInfo.renderState.renderCommandEncoder = m_RenderState.renderCommandEncoder;
        }
        else
        {
          // even outside of render passes, we need to restore the state
          // m_RenderState.BindPipeline(this, cmdBuffer, MetalRenderState::BindInitial);
        }
      }
    }

    RDResult status = ResultCode::Succeeded;

    if(replayType == eReplay_Full)
      status = ContextReplayLog(m_State, startEventID, endEventID, partial);
    else if(replayType == eReplay_WithoutDraw)
      status = ContextReplayLog(m_State, startEventID, RDCMAX(1U, endEventID) - 1, partial);
    else if(replayType == eReplay_OnlyDraw)
      status = ContextReplayLog(m_State, endEventID, endEventID, partial);
    else
      RDCFATAL("Unexpected replay type");

    RDCASSERTEQUAL(status.code, ResultCode::Succeeded);
    if(m_ReplayPartialCmdBuffer != NULL)
    {
      WrappedMTLCommandBuffer *cmdBuffer = m_ReplayPartialCmdBuffer;

      // end any active XFB
      // end any active conditional rendering

      // check if the render pass is active - it could have become active
      // even if it wasn't before (if the above event was a CmdBeginRenderEncoder).
      // If we began our own custom single-action loadrp, and it was ended by a CmdEndRenderEncoder,
      // we need to reverse the virtual transitions we did above, as it won't happen otherwise
      bool rpActiveNow = false;
      if(m_ReplayCurrentCmdBufferID != ResourceId())
      {
        ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
        rpActiveNow = cmdBufInfo.renderPassOpen;
        if(rpActiveNow)
        {
          RDCLOG(" S End %p", m_RenderState.renderCommandEncoder);
          m_RenderState.EndRenderPass();
          ClearActiveRenderCommandEncoder();
        }
      }

      // we might have replayed a CmdBeginRenderEncoder or CmdEndRenderEncoder,
      // but we want to keep the partial replay data state intact, so restore
      // whether or not a render pass was active.
      rpActiveNow = rpWasActive;

      SubmitCmds();

      m_ReplayPartialCmdBuffer = NULL;
    }
  }
}

rdcstr WrappedMTLDevice::GetChunkName(uint32_t idx)
{
  if((SystemChunk)idx == SystemChunk::DriverInit)
    return "MTLCreateInstance"_lit;

  if((SystemChunk)idx < SystemChunk::FirstDriverChunk)
    return ToStr((SystemChunk)idx);

  return ToStr((MetalChunk)idx);
}

void WrappedMTLDevice::AddResource(ResourceId id, ResourceType type, const char *defaultNamePrefix)
{
  ResourceDescription &descr = GetReplay()->GetResourceDesc(id);

  uint64_t num;
  memcpy(&num, &id, sizeof(uint64_t));
  descr.name = defaultNamePrefix + (" " + ToStr(num));
  descr.autogeneratedName = true;
  descr.type = type;
  AddResourceCurChunk(descr);
}

void WrappedMTLDevice::DerivedResource(ResourceId parentLive, ResourceId child)
{
  ResourceId parentId = GetResourceManager()->GetOriginalID(parentLive);

  GetReplay()->GetResourceDesc(parentId).derivedResources.push_back(child);
  GetReplay()->GetResourceDesc(child).parentResources.push_back(parentId);
}

void WrappedMTLDevice::AddResourceCurChunk(ResourceDescription &descr)
{
  descr.initialisationChunks.push_back((uint32_t)m_StructuredFile->chunks.size() - 1);
}

void WrappedMTLDevice::AddResourceCurChunk(ResourceId id)
{
  AddResourceCurChunk(GetReplay()->GetResourceDesc(id));
}

WrappedMTLCommandBuffer *WrappedMTLDevice::GetInitStateCmd()
{
  if(initStateCurBatch >= initialStateMaxBatch)
  {
    CloseInitStateCmd();
  }

  if(initStateCurCmd == NULL)
  {
    initStateCurCmd = GetNextCommandBuffer();

    if(initStateCurCmd == NULL)
      return NULL;

    if(IsReplayMode(m_State))
    {
      //      VkMarkerRegion::Begin("!!!!RenderDoc Internal: ApplyInitialContents batched list",
      //                            initStateCurCmd);
    }
  }

  initStateCurBatch++;

  return initStateCurCmd;
}

void WrappedMTLDevice::CloseInitStateCmd()
{
  if(initStateCurCmd == NULL)
    return;

  //  VkMarkerRegion::End(initStateCurCmd);

  initStateCurCmd->commit();

  initStateCurCmd = NULL;
  initStateCurBatch = 0;
}

WrappedMTLCommandBuffer *WrappedMTLDevice::GetNextCommandBuffer()
{
  WrappedMTLCommandBuffer *cmdBuffer = m_ReplayCommandQueue->commandBuffer();
  // cmd->enqueue();
  AddPendingCommandBuffer(cmdBuffer);
  return cmdBuffer;
}

void WrappedMTLDevice::RemovePendingCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  m_InternalCmds.m_PendingCmds.removeOne(cmdBuffer);
}

void WrappedMTLDevice::AddPendingCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  m_InternalCmds.m_PendingCmds.push_back(cmdBuffer);
}

void WrappedMTLDevice::SubmitCmds()
{
  RENDERDOC_PROFILEFUNCTION();
  if(HasFatalError())
    return;

  // nothing to do
  if(m_InternalCmds.m_PendingCmds.empty())
    return;

  for(size_t i = 0; i < m_InternalCmds.m_PendingCmds.size(); i++)
  {
    m_InternalCmds.m_PendingCmds[i]->commit();
  }

  m_InternalCmds.m_SubmittedCmds.append(m_InternalCmds.m_PendingCmds);
  m_InternalCmds.m_PendingCmds.clear();
}

void WrappedMTLDevice::FlushQ()
{
  RENDERDOC_PROFILEFUNCTION();

  if(HasFatalError())
    return;

  for(size_t i = 0; i < m_InternalCmds.m_SubmittedCmds.size(); i++)
  {
    WrappedMTLCommandBuffer *cmd = m_InternalCmds.m_SubmittedCmds[i];
    cmd->waitUntilCompleted();
  }
  if(!m_InternalCmds.m_SubmittedCmds.empty())
  {
    m_InternalCmds.m_SubmittedCmds.clear();
  }
  else
  {
    WaitForGPU();
  }
}

void WrappedMTLDevice::WaitForGPU()
{
  MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
  mtlCommandBuffer->commit();
  mtlCommandBuffer->waitUntilCompleted();
}

MetalLockedTextureStateRef WrappedMTLDevice::FindTextureState(ResourceId id)
{
  SCOPED_LOCK(m_TextureStatesLock);
  auto it = m_TextureStates.find(id);
  if(it != m_TextureStates.end())
    return it->second.LockWrite();
  else
    return MetalLockedTextureStateRef();
}

MetalLockedConstTextureStateRef WrappedMTLDevice::FindConstTextureState(ResourceId id)
{
  SCOPED_LOCK(m_TextureStatesLock);
  auto it = m_TextureStates.find(id);
  if(it != m_TextureStates.end())
    return it->second.LockRead();
  else
    return MetalLockedConstTextureStateRef();
}

MetalLockedTextureStateRef WrappedMTLDevice::InsertTextureState(WrappedMTLTexture *wrappedHandle,
                                                                ResourceId id,
                                                                const MetalTextureInfo &info,
                                                                FrameRefType refType, bool *inserted)
{
  SCOPED_LOCK(m_TextureStatesLock);
  auto it = m_TextureStates.find(id);
  if(it != m_TextureStates.end())
  {
    if(inserted != NULL)
      *inserted = false;
    return it->second.LockWrite();
  }
  else
  {
    if(inserted != NULL)
      *inserted = true;
    it = m_TextureStates.insert({id, MetalLockingTextureState(wrappedHandle, info, refType)}).first;
    return it->second.LockWrite();
  }
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_BeginCaptureFrame(SerialiserType &ser)
{
  SCOPED_LOCK(m_TextureStatesLock);

  //  for(auto it = m_ImageStates.begin(); it != m_ImageStates.end(); ++it)
  //  {
  //    it->second.LockWrite()->FixupStorageReferences();
  //  }

  GetResourceManager()->SerialiseTextureStates(ser, m_TextureStates);

  SERIALISE_CHECK_READ_ERRORS();

  return true;
}

void WrappedMTLDevice::StartFrameCapture(void *dev, void *wnd)
{
  if(!IsBackgroundCapturing(m_State))
    return;

  RDCLOG("Starting capture");
  {
    SCOPED_LOCK(m_CommandBuffersLock);
    m_CaptureCommandBufferStartSubmitIndex = m_CommandBufferNextSubmitIndex;
    m_CaptureCommandBufferEndSubmitIndex = LONG_MAX;
    RDCASSERT(m_CommandBuffersQueuedPendingPresent.empty());
  }

  m_CaptureTimer.Restart();

  GetResourceManager()->ResetCaptureStartTime();

  m_AppControlledCapture = true;

  FrameDescription frame;
  frame.frameNumber = ~0U;
  frame.captureTime = Timing::GetUnixTimestamp();
  m_CapturedFrames.push_back(frame);

  GetResourceManager()->ClearReferencedResources();
  // TODO: handle tracked memory

  // need to do all this atomically so that no other commands
  // will check to see if they need to mark dirty or
  // mark pending dirty and go into the frame record.
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);
    // TODO: sync all active (enqueued and committed) command buffers

    GetResourceManager()->PrepareInitialContents();
    /*
    SubmitCmds();
    FlushQ();

    {
      SCOPED_LOCK(m_CapDescriptorsLock);
      m_CapDescriptors.clear();
    }
     */

    RDCDEBUG("Attempting capture");
    m_FrameCaptureRecord->DeleteChunks();
    {
      SCOPED_LOCK(m_TextureStatesLock);
      for(auto it = m_TextureStates.begin(); it != m_TextureStates.end(); ++it)
      {
        it->second.LockWrite()->BeginCapture();
      }
    }
    m_State = CaptureState::ActiveCapturing;
  }

  GetResourceManager()->MarkResourceFrameReferenced(GetResID(this), eFrameRef_Read);
  // TODO: is there any other type of resource that needs to be marked as frame referenced
  // MTLCommandQueue's, MTLLibrary's
  // GetResourceManager()->MarkResourceFrameReferenced(GetResID(m_Queue), eFrameRef_Read);

  /*
rdcarray<MetalResourceRecord *> forced = GetForcedReferences();

// Note we force read-before-write because this resource is implicitly untracked so we have no
// way of knowing how it's used
for(auto it = forced.begin(); it != forced.end(); ++it)
{
  // reference the buffer
  GetResourceManager()->MarkResourceFrameReferenced((*it)->GetResourceID(), eFrameRef_Read);
  // and its backing memory
  GetResourceManager()->MarkMemoryFrameReferenced((*it)->baseResource, (*it)->memOffset,
                                                  (*it)->memSize, eFrameRef_ReadBeforeWrite);
}
   */
}

void WrappedMTLDevice::EndCaptureFrame(ResourceId backbuffer)
{
  CACHE_THREAD_SERIALISER();
  ser.SetActionChunk();
  SCOPED_SERIALISE_CHUNK(SystemChunk::CaptureEnd);

  SERIALISE_ELEMENT_LOCAL(PresentedImage, backbuffer).TypedAs("MTLTexture"_lit);

  m_FrameCaptureRecord->AddChunk(scope.Get());
}

bool WrappedMTLDevice::EndFrameCapture(void *dev, void *wnd)
{
  if(!IsActiveCapturing(m_State) && !m_DeferredCapture)
    return true;

  RDCLOG("Finished capture, Frame %u", m_CapturedFrames.back().frameNumber);

  ResourceId bbId;
  WrappedMTLTexture *backBuffer = m_CapturedBackbuffer;
  m_CapturedBackbuffer = NULL;
  if(backBuffer)
  {
    bbId = GetResID(backBuffer);
  }
  if(bbId == ResourceId())
  {
    RDCERR("Invalid Capture backbuffer");
    return false;
  }
  GetResourceManager()->MarkResourceFrameReferenced(bbId, eFrameRef_Read);

  // atomically transition back to IDLE
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);
    EndCaptureFrame(bbId);

    m_State = CaptureState::BackgroundCapturing;
    m_DeferredCapture = false;
  }

  // wait for the GPU to be idle
  for(auto it = m_CommandBuffersCommittedRecords.begin();
      it != m_CommandBuffersCommittedRecords.end(); ++it)
  {
    WrappedMTLCommandBuffer *commandBuffer = (WrappedMTLCommandBuffer *)((*it)->m_Resource);
    Unwrap(commandBuffer)->waitUntilCompleted();
  }

  if(m_CommandBuffersCommittedRecords.empty())
  {
    WaitForGPU();
  }

  // get the backbuffer to generate the thumbnail image
  const uint32_t maxSize = 2048;
  RenderDoc::FramePixels fp;

  MTL::Texture *mtlBackBuffer = Unwrap(backBuffer);

  MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
  MTL::BlitCommandEncoder *mtlBlitEncoder = mtlCommandBuffer->blitCommandEncoder();

  NS::UInteger sourceWidth = mtlBackBuffer->width();
  NS::UInteger sourceHeight = mtlBackBuffer->height();
  MTL::Origin sourceOrigin(0, 0, 0);
  MTL::Size sourceSize(sourceWidth, sourceHeight, 1);

  MTL::PixelFormat format = mtlBackBuffer->pixelFormat();
  uint32_t bytesPerRow = GetByteSize(sourceWidth, 1, 1, format, 0);
  NS::UInteger bytesPerImage = sourceHeight * bytesPerRow;

  MTL::Buffer *mtlCpuPixelBuffer =
      Unwrap(this)->newBuffer(bytesPerImage, MTL::ResourceStorageModeShared);

  mtlBlitEncoder->copyFromTexture(mtlBackBuffer, 0, 0, sourceOrigin, sourceSize, mtlCpuPixelBuffer,
                                  0, bytesPerRow, bytesPerImage);
  mtlBlitEncoder->endEncoding();

  mtlCommandBuffer->commit();
  mtlCommandBuffer->waitUntilCompleted();
  uint32_t *pixels = (uint32_t *)mtlCpuPixelBuffer->contents();

  fp.len = (uint32_t)mtlCpuPixelBuffer->length();
  fp.data = new uint8_t[fp.len];
  memcpy(fp.data, pixels, fp.len);

  mtlCpuPixelBuffer->release();

  ResourceFormat fmt = MakeResourceFormat(format);
  fp.width = sourceWidth;
  fp.height = sourceHeight;
  fp.pitch = bytesPerRow;
  fp.stride = fmt.compByteWidth * fmt.compCount;
  fp.bpc = fmt.compByteWidth;
  fp.bgra = fmt.BGRAOrder();
  fp.max_width = maxSize;
  fp.pitch_requirement = 8;
  // TODO: handle different resource formats
  /*
  switch(fmt.type)
  {
    case ResourceFormatType::R10G10B10A2:
      fp.stride = 4;
      fp.buf1010102 = true;
      break;
    case ResourceFormatType::R5G6B5:
      fp.stride = 2;
      fp.buf565 = true;
      break;
    case ResourceFormatType::R5G5B5A1:
      fp.stride = 2;
      fp.buf5551 = true;
      break;
    default: break;
  }
   */

  RDCFile *rdc =
      RenderDoc::Inst().CreateRDC(RDCDriver::Metal, m_CapturedFrames.back().frameNumber, fp);

  StreamWriter *captureWriter = NULL;

  if(rdc)
  {
    SectionProperties props;

    // Compress with LZ4 so that it's fast
    props.flags = SectionFlags::LZ4Compressed;
    props.version = m_SectionVersion;
    props.type = SectionType::FrameCapture;

    captureWriter = rdc->WriteSection(props);
  }
  else
  {
    captureWriter = new StreamWriter(StreamWriter::InvalidStream);
  }

  uint64_t captureSectionSize = 0;

  {
    WriteSerialiser ser(captureWriter, Ownership::Stream);

    ser.SetChunkMetadataRecording(GetThreadSerialiser().GetChunkMetadataRecording());

    ser.SetUserData(GetResourceManager());

    {
      m_InitParams.Set(Unwrap(this), m_ID);
      SCOPED_SERIALISE_CHUNK(SystemChunk::DriverInit, m_InitParams.GetSerialiseSize());

      SERIALISE_ELEMENT(m_InitParams);
    }

    RDCDEBUG("Inserting Resource Serialisers");

    GetResourceManager()->InsertReferencedChunks(ser);

    GetResourceManager()->InsertInitialContentsChunks(ser);

    RDCDEBUG("Creating Capture Scope");

    GetResourceManager()->Serialise_InitialContentsNeeded(ser);
    // GetResourceManager()->InsertDeviceMemoryRefs(ser);

    {
      SCOPED_SERIALISE_CHUNK(SystemChunk::CaptureScope, 16);
      Serialise_CaptureScope(ser);
    }

    {
      WriteSerialiser &captureBeginSer = GetThreadSerialiser();
      ScopedChunk scope(captureBeginSer, SystemChunk::CaptureBegin);

      Serialise_BeginCaptureFrame(captureBeginSer);
      m_HeaderChunk = scope.Get();
    }

    m_HeaderChunk->Write(ser);

    // don't need to lock access to m_CommandBufferRecords as we are no longer
    // in capframe (the transition is thread-protected) so nothing will be
    // pushed to the vector

    {
      std::map<int64_t, Chunk *> recordlist;
      size_t countCmdBuffers = m_CommandBuffersCommittedRecords.size();
      RDCDEBUG("Flushing %zu command buffer records to file serialiser", countCmdBuffers);
      // ensure all command buffer records within the frame even if recorded before, but
      // otherwise order must be preserved (vs. queue submits and desc set updates)
      for(auto it = m_CommandBuffersCommittedRecords.begin();
          it != m_CommandBuffersCommittedRecords.end(); ++it)
      {
        MetalResourceRecord *record = *it;
        RDCDEBUG("Adding chunks from command buffer %s", ToStr(record->GetResourceID()).c_str());

        size_t prevSize = recordlist.size();
        (void)prevSize;

        record->Insert(recordlist);

        RDCDEBUG("Added %zu chunks to file serialiser", recordlist.size() - prevSize);
      }

      size_t prevSize = recordlist.size();
      (void)prevSize;
      m_FrameCaptureRecord->Insert(recordlist);
      RDCDEBUG("Adding %zu frame capture chunks to file serialiser", recordlist.size() - prevSize);
      RDCDEBUG("Flushing %zu chunks to file serialiser from context record", recordlist.size());

      float num = float(recordlist.size());
      float idx = 0.0f;

      for(auto it = recordlist.begin(); it != recordlist.end(); ++it)
      {
        RenderDoc::Inst().SetProgress(CaptureProgress::SerialiseFrameContents, idx / num);
        idx += 1.0f;
        it->second->Write(ser);
        RDCLOG("Writing Chunk %d", (int)it->second->GetChunkType<MetalChunk>());
      }

      RDCDEBUG("Done");
    }

    captureSectionSize = captureWriter->GetOffset();
  }

  RDCLOG("Captured Metal frame with %f MB capture section in %f seconds",
         double(captureSectionSize) / (1024.0 * 1024.0), m_CaptureTimer.GetMilliseconds() / 1000.0);

  RenderDoc::Inst().FinishCaptureWriting(rdc, m_CapturedFrames.back().frameNumber);

  m_HeaderChunk->Delete();
  m_HeaderChunk = NULL;

  // delete cmd buffers now - had to keep them alive until after serialiser flush.
  ClearTrackedCmdBuffers();

  GetResourceManager()->ResetLastWriteTimes();

  GetResourceManager()->MarkUnwrittenResources();

  // TODO: handle memory resources in the resource manager
  // GetResourceManager()->ClearReferencedMemory();

  GetResourceManager()->ClearReferencedResources();

  GetResourceManager()->FreeInitialContents();

  // TODO: handle memory resources in the initial contents
  // FreeAllMemory(MemoryScope::InitialContents);

  return true;
}

bool WrappedMTLDevice::DiscardFrameCapture(void *dev, void *wnd)
{
  if(!IsActiveCapturing(m_State))
    return true;

  RDCLOG("Discarding frame capture.");

  RenderDoc::Inst().FinishCaptureWriting(NULL, m_CapturedFrames.back().frameNumber);

  m_CapturedFrames.pop_back();

  // transition back to IDLE atomically
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);

    m_State = CaptureState::BackgroundCapturing;
    m_DeferredCapture = false;

    /*
    ObjDisp(GetDev())->DeviceWaitIdle(Unwrap(GetDev()));

    {
      SCOPED_LOCK(m_CoherentMapsLock);
      for(auto it = m_CoherentMaps.begin(); it != m_CoherentMaps.end(); ++it)
      {
        FreeAlignedBuffer((*it)->memMapState->refData);
        (*it)->memMapState->refData = NULL;
        (*it)->memMapState->needRefData = false;
      }
    }
     */
  }

  m_HeaderChunk->Delete();
  m_HeaderChunk = NULL;

  // delete cmd buffers now - had to keep them alive until after serialiser flush.
  ClearTrackedCmdBuffers();

  GetResourceManager()->MarkUnwrittenResources();

  GetResourceManager()->ClearReferencedResources();

  GetResourceManager()->FreeInitialContents();

  //  FreeAllMemory(MemoryScope::InitialContents);

  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_CaptureScope(SerialiserType &ser)
{
  SERIALISE_ELEMENT_LOCAL(frameNumber, m_CapturedFrames.back().frameNumber);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    GetReplay()->WriteFrameRecord().frameInfo.frameNumber = frameNumber;
    RDCEraseEl(GetReplay()->WriteFrameRecord().frameInfo.stats);
  }
  return true;
}

void WrappedMTLDevice::CommitCommandBufferRecord(MetalResourceRecord *record)
{
  SCOPED_LOCK(m_CommandBuffersLock);
  int64_t submitIndex = record->cmdInfo->submitIndex;
  RDCASSERTNOTEQUAL(0, submitIndex);
  m_CommandBuffersEnqueued.erase(record);
  if(ShouldCaptureEnqueuedCommandBuffer(submitIndex))
  {
    int64_t presentSubmitIndex = m_CommandBuffersPresentRecord->cmdInfo->submitIndex;
    m_CommandBuffersQueuedPendingPresent.erase(record);
    if(m_CommandBuffersQueuedPendingPresent.empty())
    {
      RDCLOG("Capture triggering deferred capture %ld %ld", submitIndex, presentSubmitIndex);
      CA::MetalLayer *outputLayer = m_CommandBuffersPresentRecord->cmdInfo->outputLayer;
      RenderDoc::Inst().EndFrameCapture(this, outputLayer);
    }
  }
}

void WrappedMTLDevice::StartAddCommandBufferRecord(MetalResourceRecord *record)
{
  if(m_DeferredCapture)
  {
    RDCASSERTEQUAL(CaptureState::BackgroundCapturing, m_State);
    m_CapTransitionLock.WriteLock();
    m_State = CaptureState::ActiveCapturing;
  }
}

void WrappedMTLDevice::EndAddCommandBufferRecord(MetalResourceRecord *record)
{
  SCOPED_LOCK(m_CommandBuffersLock);
  if(record->cmdInfo->submitIndex == 0)
    EnqueueCommandBufferRecord(record);

  int64_t submitIndex = record->cmdInfo->submitIndex;
  m_CommandBuffersCommittedRecords.insert(record);

  RDCDEBUG("Adding CommandBufferRecord %s Count %zu %ld", ToStr(record->GetResourceID()).c_str(),
           m_CommandBuffersCommittedRecords.size(), submitIndex);

  CommitCommandBufferRecord(record);

  if(m_DeferredCapture)
  {
    RDCASSERTEQUAL(CaptureState::ActiveCapturing, m_State);
    m_State = CaptureState::BackgroundCapturing;
    m_CapTransitionLock.WriteUnlock();
  }
}

void WrappedMTLDevice::EnqueueCommandBufferRecord(MetalResourceRecord *record)
{
  SCOPED_LOCK(m_CommandBuffersLock);
  RDCASSERTEQUAL(0, record->cmdInfo->submitIndex);
  int64_t submitIndex = Atomic::Inc64(&m_CommandBufferNextSubmitIndex) - 1;
  record->cmdInfo->submitIndex = submitIndex;
  m_CommandBuffersEnqueued.insert(record);

  RDCDEBUG("Enqueing CommandBufferRecord %s %ld", ToStr(record->GetResourceID()).c_str(),
           submitIndex);
}

void WrappedMTLDevice::AdvanceFrame()
{
  if(IsBackgroundCapturing(m_State))
    RenderDoc::Inst().Tick();

  m_FrameCounter++;    // first present becomes frame #1, this function is at the end of the frame
}

void WrappedMTLDevice::FirstFrame()
{
  // if we have to capture the first frame, begin capturing immediately
  if(IsBackgroundCapturing(m_State) && RenderDoc::Inst().ShouldTriggerCapture(0))
  {
    RenderDoc::Inst().StartFrameCapture(this, NULL);

    m_AppControlledCapture = false;
    m_CapturedFrames.back().frameNumber = 0;
  }
}

void WrappedMTLDevice::Present(MetalResourceRecord *record)
{
  WrappedMTLTexture *backBuffer = record->cmdInfo->backBuffer;
  {
    SCOPED_LOCK(m_PotentialBackBuffersLock);
    if(m_PotentialBackBuffers.count(backBuffer) == 0)
    {
      RDCERR("Capture ignoring Present called on unknown backbuffer");
      return;
    }
  }

  CA::MetalLayer *outputLayer = record->cmdInfo->outputLayer;
  {
    SCOPED_LOCK(m_OutputLayersLock);
    if(m_OutputLayers.count(outputLayer) == 0)
    {
      m_OutputLayers.insert(outputLayer);
      RenderDoc::Inst().AddFrameCapturer(this, outputLayer, this->m_Capturer);
      ObjC::Set_FramebufferOnly(outputLayer, false);
    }
  }

  bool activeWindow = RenderDoc::Inst().IsActiveWindow(this, outputLayer);

  RenderDoc::Inst().AddActiveDriver(RDCDriver::Metal, true);

  if(!activeWindow)
    return;

  if(IsActiveCapturing(m_State) && !m_AppControlledCapture)
  {
    m_CommandBuffersPresentRecord = record;
    int64_t presentSubmitIndex = record->cmdInfo->submitIndex;
    bool captureNow;
    {
      SCOPED_LOCK(m_CommandBuffersLock);
      m_CaptureCommandBufferEndSubmitIndex = presentSubmitIndex;
      m_CommandBuffersQueuedPendingPresent.swap(m_CommandBuffersEnqueued);
      captureNow = m_CommandBuffersQueuedPendingPresent.empty();
      RDCASSERT(m_CommandBuffersEnqueued.empty());
    }

    RDCASSERT(m_CapturedBackbuffer == NULL);
    m_CapturedBackbuffer = backBuffer;
    if(captureNow)
    {
      m_DeferredCapture = false;
      RenderDoc::Inst().EndFrameCapture(this, outputLayer);
    }
    else
    {
      SCOPED_WRITELOCK(m_CapTransitionLock);
      m_State = CaptureState::BackgroundCapturing;
      m_DeferredCapture = true;
      RDCERR("Capture Present deferring end capture %ld %zu", presentSubmitIndex,
             m_CommandBuffersQueuedPendingPresent.size());
    }
  }

  if(RenderDoc::Inst().ShouldTriggerCapture(m_FrameCounter) && IsBackgroundCapturing(m_State))
  {
    RenderDoc::Inst().StartFrameCapture(this, outputLayer);

    m_AppControlledCapture = false;
    m_CapturedFrames.back().frameNumber = m_FrameCounter;
  }
}

void WrappedMTLDevice::ClearTrackedCmdBuffers()
{
  SCOPED_LOCK(m_CommandBuffersLock);
  for(auto it = m_CommandBuffersCommittedRecords.begin();
      it != m_CommandBuffersCommittedRecords.end(); ++it)
  {
    (*it)->Delete(GetResourceManager());
  }

  m_CommandBuffersCommittedRecords.clear();
  m_CommandBuffersPresentRecord = NULL;
  m_CaptureCommandBufferStartSubmitIndex = LONG_MAX;
  m_CaptureCommandBufferEndSubmitIndex = LONG_MAX;
  m_CommandBuffersEnqueued.clear();
  m_CommandBuffersQueuedPendingPresent.clear();
}

void WrappedMTLDevice::ClearActiveRenderCommandEncoder()
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  // RDCASSERT(cmdBufInfo.renderCommandEncoder);
  RDCASSERT(cmdBufInfo.renderPassOpen);
  cmdBufInfo.renderState.renderCommandEncoder = NULL;
  cmdBufInfo.renderPassOpen = false;
}

void WrappedMTLDevice::SetActiveRenderCommandEncoder(WrappedMTLRenderCommandEncoder *renderCommandEncoder)
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  RDCASSERT(renderCommandEncoder);
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
  RDCASSERT(!cmdBufInfo.renderPassOpen);
  cmdBufInfo.renderState.renderCommandEncoder = renderCommandEncoder;
  cmdBufInfo.renderPassOpen = true;
}

WrappedMTLRenderCommandEncoder *WrappedMTLDevice::GetCurrentReplayRenderEncoder()
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  return cmdBufInfo.renderState.renderCommandEncoder;
}

WrappedMTLCommandBuffer *WrappedMTLDevice::GetCurrentReplayCommandBuffer()
{
  if(m_ReplayPartialCmdBufferID != ResourceId())
    return m_ReplayPartialCmdBuffer;

  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(cmdBufInfo.cmdBuffer);
  return cmdBufInfo.cmdBuffer;
}

bool WrappedMTLDevice::IsCurrentCommandBufferEventInReplayRange()
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  if(m_ReplayCurrentCmdBufferID == m_ReplayPartialCmdBufferID)
    return true;
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  uint32_t startEID = cmdBufInfo.baseRootEvent;
  uint32_t cmdBufCurEID = cmdBufInfo.curEventID;
  uint32_t rebasedEID = startEID + cmdBufCurEID;
  return rebasedEID <= m_LastEventID;
}

void WrappedMTLDevice::NewReplayCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  m_ReplayCurrentCmdBufferID = GetResourceManager()->GetOriginalID(GetResID(cmdBuffer));
  MetalActionTreeNode *action = new MetalActionTreeNode;

  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  cmdBufInfo.action = action;
  cmdBufInfo.actionCount = 0;
  cmdBufInfo.baseRootEvent = 0;
  cmdBufInfo.eventCount = 0;
  cmdBufInfo.actionStack.push_back(action);
  cmdBufInfo.beginChunk = uint32_t(m_StructuredFile->chunks.size() - 1);
  cmdBufInfo.endChunk = 0;
  ResetReplayCommandBuffer(cmdBuffer);
  m_ReplayCurrentCmdBufferID = ResourceId();
}

void WrappedMTLDevice::ReplayCommandBufferCommit(WrappedMTLCommandBuffer *cmdBuffer)
{
  SetCurrentCommandBuffer(NULL);
  ResourceId cmdID = GetResourceManager()->GetOriginalID(GetResID(cmdBuffer));
  RDCASSERT(cmdID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[cmdID];
  if(IsLoading(m_State))
  {
    uint32_t cmdBufStart = m_RootEventID;
    AddEvent();
    m_RootEventID++;

    cmdBufInfo.eventCount = cmdBufInfo.curEventID;
    cmdBufInfo.curEventID = 0;
    cmdBufInfo.endChunk = uint32_t(m_StructuredFile->chunks.size() - 1);
    rdcstr name = StringFormat::Fmt("=> Commit : StartCommandBuffer(%s)", ToStr(cmdID).c_str());

    ActionDescription action;
    {
      // add a fake marker
      action.customName = name;
      action.flags |= ActionFlags::CommandBufferBoundary;
      AddEvent();

      m_RootEvents.back().chunkIndex = cmdBufInfo.beginChunk;
      m_Events.back().chunkIndex = cmdBufInfo.beginChunk;

      AddAction(action);
      m_RootEventID++;
    }

    cmdBufInfo.baseRootEvent = m_RootEventID;

    // insert the actions with new event and action IDs
    InsertCommandBufferActionsAndRefreshIDs(cmdBufInfo);

    m_RootEventID += cmdBufInfo.eventCount;
    m_RootActionID += cmdBufInfo.actionCount;

    // pull in any remaining events on the command buffer that weren't added to an action
    uint32_t i = 0;
    for(APIEvent &apievent : cmdBufInfo.curEvents)
    {
      apievent.eventId = m_RootEventID - cmdBufInfo.curEvents.count() + i;

      m_RootEvents.push_back(apievent);
      m_Events.resize(apievent.eventId + 1);
      m_Events[apievent.eventId] = apievent;

      i++;
    }

    name = StringFormat::Fmt("=> Commit : EndCommandBuffer(%s)", ToStr(cmdID).c_str());
    action.customName = name;
    action.flags = ActionFlags::CommandBufferBoundary;
    AddEvent();

    m_RootEvents.back().chunkIndex = cmdBufInfo.endChunk;
    m_Events.back().chunkIndex = cmdBufInfo.endChunk;

    AddAction(action);
    uint32_t cmdBufEnd = m_RootEventID;
    for(uint32_t eid = cmdBufStart; eid <= cmdBufEnd; ++eid)
    {
      APIEvent &e = m_Events[eid];
      RDCLOG("Z Events[%d] %d %d %lu", eid, e.eventId, e.chunkIndex, e.fileOffset);
    }
  }
  else
  {
    uint32_t startEID = cmdBufInfo.baseRootEvent;
    uint32_t cmdBufCurEID = cmdBufInfo.curEventID;

    // skip past the CommandBuffer commit event
    m_RootEventID++;

    m_RootEventID += cmdBufInfo.eventCount;
    m_RootActionID += cmdBufInfo.actionCount;

    // 2 extra for the virtual StartCommandBuffer & EndCommandBuffer markers
    m_RootEventID += 2;
    m_RootActionID += 2;

    uint32_t rebasedEID = startEID + cmdBufCurEID;
    if(startEID > m_LastEventID)
    {
      RDCLOG("M %s commit Before Start %d >= %d", ToStr(cmdID).c_str(), startEID, m_LastEventID);
    }
    else if(rebasedEID > m_LastEventID)
    {
      RDCLOG("M %s commit Partial %d > %d", ToStr(cmdID).c_str(), rebasedEID, m_LastEventID);
      RDCASSERT(m_ReplayPartialCmdBufferID == ResourceId() || m_ReplayPartialCmdBufferID == cmdID);
      m_ReplayPartialCmdBufferID = cmdID;
      if(cmdBufInfo.renderPassOpen)
      {
        RDCASSERT(cmdBufInfo.renderState.renderCommandEncoder);
        cmdBufInfo.renderState.renderCommandEncoder->endEncoding();
        cmdBufInfo.renderState.renderCommandEncoder = NULL;
      }
      m_Device->RemovePendingCommandBuffer(cmdBuffer);
      Unwrap(cmdBuffer)->commit();
    }
    else
    {
      m_Device->RemovePendingCommandBuffer(cmdBuffer);
      Unwrap(cmdBuffer)->commit();
      RDCLOG("M %s commit", ToStr(cmdID).c_str());
    }
  }
}

void WrappedMTLDevice::ResetReplayCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  ResourceId cmdID = GetResourceManager()->GetOriginalID(GetResID(cmdBuffer));
  RDCASSERT(cmdID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[cmdID];
  cmdBufInfo.curEventID = 0;
  cmdBufInfo.renderPassOpen = false;
  cmdBufInfo.renderState.renderCommandEncoder = NULL;
  cmdBufInfo.cmdBuffer = cmdBuffer;
}

void WrappedMTLDevice::SetCurrentCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  m_ReplayCurrentCmdBufferID = GetResourceManager()->GetOriginalID(GetResID(cmdBuffer));
  if(cmdBuffer)
  {
    RDCASSERT(GetResourceManager()->HasLiveResource(m_ReplayCurrentCmdBufferID));
    RDCASSERTEQUAL(m_ReplayCurrentCmdBufferID,
                   GetResourceManager()->GetUnreplacedOriginalID(GetResID(cmdBuffer)));
  }
}

void WrappedMTLDevice::InsertCommandBufferActionsAndRefreshIDs(ReplayCmdBufferInfo &cmdBufInfo)
{
  rdcarray<MetalActionTreeNode> &cmdBufNodes = cmdBufInfo.action->children;

  // assign new action IDs
  for(size_t i = 0; i < cmdBufNodes.size(); i++)
  {
    MetalActionTreeNode n = cmdBufNodes[i];
    n.action.eventId += m_RootEventID;
    n.action.actionId += m_RootActionID;

    for(APIEvent &ev : n.action.events)
    {
      ev.eventId += m_RootEventID;
      m_Events.resize(ev.eventId + 1);
      m_Events[ev.eventId] = ev;
    }

    RDCASSERT(n.children.empty());

    GetActionStack().back()->children.push_back(n);
  }
}

MetalInitParams::MetalInitParams()
{
  memset(this, 0, sizeof(MetalInitParams));
}

uint64_t MetalInitParams::GetSerialiseSize()
{
  size_t ret = 0;

  // device information
  ret += sizeof(uint32_t);
  ret += sizeof(char) * name->length();

  ret += sizeof(uint64_t) * 4 + sizeof(uint32_t) * 2;
  ret += sizeof(MTL::DeviceLocation);
  ret += sizeof(NS::UInteger);
  ret += sizeof(bool) * 4;

  // device capabilities
  ret += sizeof(bool) * 15;
  ret += sizeof(MTL::ArgumentBuffersTier);

  ret += sizeof(ResourceId);

  return (uint64_t)ret;
}

bool MetalInitParams::IsSupportedVersion(uint64_t ver)
{
  if(ver == CurrentVersion)
    return true;

  return false;
}

void MetalInitParams::Set(MTL::Device *pRealDevice, ResourceId inst)
{
  // device information
  name = pRealDevice->name();
  recommendedMaxWorkingSetSize = pRealDevice->recommendedMaxWorkingSetSize();
  maxTransferRate = pRealDevice->maxTransferRate();
  registryID = pRealDevice->registryID();
  peerGroupID = pRealDevice->peerGroupID();
  peerCount = pRealDevice->peerCount();
  peerIndex = pRealDevice->peerIndex();
  location = pRealDevice->location();
  locationNumber = pRealDevice->locationNumber();
  hasUnifiedMemory = pRealDevice->hasUnifiedMemory();
  headless = pRealDevice->headless();
  lowPower = pRealDevice->lowPower();
  removable = pRealDevice->removable();

  // device capabilities
  supportsMTLGPUFamilyCommon1 = pRealDevice->supportsFamily(MTL::GPUFamilyCommon1);
  supportsMTLGPUFamilyCommon2 = pRealDevice->supportsFamily(MTL::GPUFamilyCommon2);
  supportsMTLGPUFamilyCommon3 = pRealDevice->supportsFamily(MTL::GPUFamilyCommon3);

  supportsMTLGPUFamilyApple1 = pRealDevice->supportsFamily(MTL::GPUFamilyApple1);
  supportsMTLGPUFamilyApple2 = pRealDevice->supportsFamily(MTL::GPUFamilyApple2);
  supportsMTLGPUFamilyApple3 = pRealDevice->supportsFamily(MTL::GPUFamilyApple3);
  supportsMTLGPUFamilyApple4 = pRealDevice->supportsFamily(MTL::GPUFamilyApple4);
  supportsMTLGPUFamilyApple5 = pRealDevice->supportsFamily(MTL::GPUFamilyApple5);
  supportsMTLGPUFamilyApple6 = pRealDevice->supportsFamily(MTL::GPUFamilyApple6);
  supportsMTLGPUFamilyApple7 = pRealDevice->supportsFamily(MTL::GPUFamilyApple7);
  supportsMTLGPUFamilyApple8 = pRealDevice->supportsFamily(MTL::GPUFamilyApple8);

  supportsMTLGPUFamilyMac1 = pRealDevice->supportsFamily(MTL::GPUFamilyMac1);
  supportsMTLGPUFamilyMac2 = pRealDevice->supportsFamily(MTL::GPUFamilyMac2);

  supportsMTLGPUFamilyMacCatalyst1 = pRealDevice->supportsFamily(MTL::GPUFamilyMacCatalyst1);
  supportsMTLGPUFamilyMacCatalyst2 = pRealDevice->supportsFamily(MTL::GPUFamilyMacCatalyst2);

  argumentBuffersSupport = pRealDevice->argumentBuffersSupport();

  InstanceID = inst;
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalInitParams &el)
{
  SERIALISE_MEMBER(name);
  SERIALISE_MEMBER(recommendedMaxWorkingSetSize);
  SERIALISE_MEMBER(maxTransferRate);
  SERIALISE_MEMBER(registryID);
  SERIALISE_MEMBER(peerGroupID);
  SERIALISE_MEMBER(peerCount);
  SERIALISE_MEMBER(peerIndex);
  SERIALISE_MEMBER(location);
  SERIALISE_MEMBER(locationNumber);
  SERIALISE_MEMBER(hasUnifiedMemory);
  SERIALISE_MEMBER(headless);
  SERIALISE_MEMBER(lowPower);
  SERIALISE_MEMBER(removable);

  // device capabilities
  SERIALISE_MEMBER(supportsMTLGPUFamilyCommon1);
  SERIALISE_MEMBER(supportsMTLGPUFamilyCommon2);
  SERIALISE_MEMBER(supportsMTLGPUFamilyCommon3);

  SERIALISE_MEMBER(supportsMTLGPUFamilyApple1);
  SERIALISE_MEMBER(supportsMTLGPUFamilyApple2);
  SERIALISE_MEMBER(supportsMTLGPUFamilyApple3);
  SERIALISE_MEMBER(supportsMTLGPUFamilyApple4);
  SERIALISE_MEMBER(supportsMTLGPUFamilyApple5);
  SERIALISE_MEMBER(supportsMTLGPUFamilyApple6);
  SERIALISE_MEMBER(supportsMTLGPUFamilyApple7);
  SERIALISE_MEMBER(supportsMTLGPUFamilyApple8);

  SERIALISE_MEMBER(supportsMTLGPUFamilyMac1);
  SERIALISE_MEMBER(supportsMTLGPUFamilyMac2);

  SERIALISE_MEMBER(supportsMTLGPUFamilyMacCatalyst1);
  SERIALISE_MEMBER(supportsMTLGPUFamilyMacCatalyst2);

  SERIALISE_MEMBER(argumentBuffersSupport);
}

RDResult Metal_ProcessStructured(RDCFile *rdc, SDFile &output)
{
  WrappedMTLDevice wrappedMTLDevice;

  int sectionIdx = rdc->SectionIndex(SectionType::FrameCapture);

  if(sectionIdx < 0)
    RETURN_ERROR_RESULT(ResultCode::FileCorrupted, "File does not contain captured API data");

  wrappedMTLDevice.SetStructuredExport(rdc->GetSectionProperties(sectionIdx).version);
  RDResult status = wrappedMTLDevice.ReadLogInitialisation(rdc, true);

  if(status == ResultCode::Succeeded)
    wrappedMTLDevice.GetStructuredFile()->Swap(output);

  return status;
}

static StructuredProcessRegistration MetalProcessRegistration(RDCDriver::Metal,
                                                              &Metal_ProcessStructured);

INSTANTIATE_SERIALISE_TYPE(MetalInitParams);
