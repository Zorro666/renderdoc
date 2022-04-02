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

#include "metal_core.h"
#include "serialise/rdcfile.h"
#include "metal_blit_command_encoder.h"
#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_command_queue.h"
#include "metal_device.h"
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

  if(cmdBufferActive)
  {
    ResourceId rp = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].renderState.renderPass;
    //      ResourceId fb = m_ReplayCmdBufferInfos[cmdBufferId].state.GetFramebuffer();

    //      if(fb != ResourceId() && rp != ResourceId())
    if(rp != ResourceId())
    {
      const MetalCreationInfo::RenderPass &rpInfo = m_CreationInfo.m_RenderPass[rp];
      rdcarray<MetalCreationInfo::RenderPass::ColorAttachment> &colAtt =
          m_CreationInfo.m_RenderPass[rp].colorAttachments;
      //        int32_t dsAtt =
      //        m_CreationInfo.m_RenderPass[rp].subpasses[sp].depthstencilAttachment;

      RDCASSERT(colAtt.size() <= ARRAY_COUNT(action.outputs));

      for(size_t i = 0; i < ARRAY_COUNT(action.outputs) && i < colAtt.size(); i++)
      {
        action.outputs[i] = GetResourceManager()->GetOriginalID(colAtt[i].texture);
      }

      //        if(dsAtt != -1)
      //        {
      //          RDCASSERT(dsAtt < (int32_t)atts.size());
      //          action.depthOut =
      //              GetResourceManager()->GetOriginalID(m_CreationInfo.m_ImageView[atts[dsAtt]].image);
      //        }
    }
  }

  // markers don't increment action ID
  ActionFlags MarkerMask = ActionFlags::SetMarker | ActionFlags::PushMarker |
                           ActionFlags::PopMarker | ActionFlags::PassBoundary;
  if(!(action.flags & MarkerMask))
  {
    if(cmdBufferActive)
    {
      m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].actionCount++;
    }
    else
    {
      m_RootActionID++;
    }
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

#define METAL_CHUNK_NOT_HANDLED()                               \
  {                                                             \
    RDCERR("MetalChunk::%s not handled", ToStr(chunk).c_str()); \
    return false;                                               \
  }

bool WrappedMTLDevice::ProcessChunk(ReadSerialiser &ser, MetalChunk chunk)
{
  switch(chunk)
  {
    case MetalChunk::MTLCreateSystemDefaultDevice:
      return Serialise_MTLCreateSystemDefaultDevice(ser);
    case MetalChunk::MTLDevice_newCommandQueue: return Serialise_newCommandQueue(ser, NULL);
    case MetalChunk::MTLDevice_newCommandQueueWithMaxCommandBufferCount: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newHeapWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newBufferWithLength:
    case MetalChunk::MTLDevice_newBufferWithBytes:
      return Serialise_newBufferWithBytes(ser, NULL, NULL, 0, MTL::ResourceOptionCPUCacheModeDefault);
    case MetalChunk::MTLDevice_newBufferWithBytesNoCopy: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newDepthStencilStateWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newTextureWithDescriptor:
    case MetalChunk::MTLDevice_newTextureWithDescriptor_iosurface:
    case MetalChunk::MTLDevice_newTextureWithDescriptor_nextDrawable:
    {
      RDMTL::TextureDescriptor descriptor;
      return Serialise_newTextureWithDescriptor(ser, NULL, descriptor);
    }
    case MetalChunk::MTLDevice_newSharedTextureWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newSharedTextureWithHandle: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newSamplerStateWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newDefaultLibrary: return Serialise_newDefaultLibrary(ser, NULL);
    case MetalChunk::MTLDevice_newDefaultLibraryWithBundle: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newLibraryWithFile: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newLibraryWithURL: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newLibraryWithData: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newLibraryWithSource:
      return Serialise_newLibraryWithSource(ser, NULL, NULL, NULL, NULL);
    case MetalChunk::MTLDevice_newLibraryWithStitchedDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newRenderPipelineStateWithDescriptor:
    {
      RDMTL::RenderPipelineDescriptor descriptor;
      return Serialise_newRenderPipelineStateWithDescriptor(ser, NULL, descriptor, NULL);
    }
    case MetalChunk::MTLDevice_newRenderPipelineStateWithDescriptor_options:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newComputePipelineStateWithFunction: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newComputePipelineStateWithFunction_options:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newComputePipelineStateWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newFence: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newRenderPipelineStateWithTileDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newArgumentEncoderWithArguments: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_supportsRasterizationRateMapWithLayerCount:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newRasterizationRateMapWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newIndirectCommandBufferWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newEvent: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newSharedEvent: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newSharedEventWithHandle: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newCounterSampleBufferWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newDynamicLibrary: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newDynamicLibraryWithURL: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLDevice_newBinaryArchiveWithDescriptor: METAL_CHUNK_NOT_HANDLED();

    case MetalChunk::MTLLibrary_newFunctionWithName:
      return m_DummyReplayLibrary->Serialise_newFunctionWithName(ser, NULL, NULL);
    case MetalChunk::MTLLibrary_newFunctionWithName_constantValues: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLLibrary_newFunctionWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLLibrary_newIntersectionFunctionWithDescriptor: METAL_CHUNK_NOT_HANDLED();

    case MetalChunk::MTLFunction_newArgumentEncoderWithBufferIndex: METAL_CHUNK_NOT_HANDLED();

    case MetalChunk::MTLCommandQueue_commandBuffer:
      return m_DummyReplayCommandQueue->Serialise_commandBuffer(ser, NULL);
    case MetalChunk::MTLCommandQueue_commandBufferWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandQueue_commandBufferWithUnretainedReferences:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_enqueue:
      return m_DummyReplayCommandBuffer->Serialise_enqueue(ser);
    case MetalChunk::MTLCommandBuffer_commit:
      return m_DummyReplayCommandBuffer->Serialise_commit(ser);
    case MetalChunk::MTLCommandBuffer_addScheduledHandler: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_presentDrawable:
      return m_DummyReplayCommandBuffer->Serialise_presentDrawable(ser, NULL);
    case MetalChunk::MTLCommandBuffer_presentDrawable_atTime: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_presentDrawable_afterMinimumDuration:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_waitUntilScheduled: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_addCompletedHandler: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_waitUntilCompleted:
      return m_DummyReplayCommandBuffer->Serialise_waitUntilCompleted(ser);
    case MetalChunk::MTLCommandBuffer_blitCommandEncoder:
      return m_DummyReplayCommandBuffer->Serialise_blitCommandEncoder(ser, NULL);
    case MetalChunk::MTLCommandBuffer_renderCommandEncoderWithDescriptor:
    {
      RDMTL::RenderPassDescriptor descriptor;
      return m_DummyReplayCommandBuffer->Serialise_renderCommandEncoderWithDescriptor(ser, NULL,
                                                                                      descriptor);
    }
    case MetalChunk::MTLCommandBuffer_computeCommandEncoderWithDescriptor:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_blitCommandEncoderWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_computeCommandEncoder: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_computeCommandEncoderWithDispatchType:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_encodeWaitForEvent: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_encodeSignalEvent: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_parallelRenderCommandEncoderWithDescriptor:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_resourceStateCommandEncoder: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_resourceStateCommandEncoderWithDescriptor:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_accelerationStructureCommandEncoder:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_pushDebugGroup: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLCommandBuffer_popDebugGroup: METAL_CHUNK_NOT_HANDLED();

    case MetalChunk::MTLTexture_setPurgeableState: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_makeAliasable: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_getBytes: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_getBytes_slice: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_replaceRegion: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_replaceRegion_slice: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_newTextureViewWithPixelFormat: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_newTextureViewWithPixelFormat_subset: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_newTextureViewWithPixelFormat_subset_swizzle:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_newSharedTextureHandle: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_remoteStorageTexture: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLTexture_newRemoteTextureViewForDevice: METAL_CHUNK_NOT_HANDLED();

    case MetalChunk::MTLRenderPipelineState_functionHandleWithFunction: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderPipelineState_newVisibleFunctionTableWithDescriptor:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderPipelineState_newIntersectionFunctionTableWithDescriptor:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderPipelineState_newRenderPipelineStateWithAdditionalBinaryFunctions:
      METAL_CHUNK_NOT_HANDLED();

    case MetalChunk::MTLRenderCommandEncoder_endEncoding:
      return m_DummyReplayRenderCommandEncoder->Serialise_endEncoding(ser);
    case MetalChunk::MTLRenderCommandEncoder_insertDebugSignpost: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_pushDebugGroup: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_popDebugGroup: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setRenderPipelineState:
      return m_DummyReplayRenderCommandEncoder->Serialise_setRenderPipelineState(ser, NULL);
    case MetalChunk::MTLRenderCommandEncoder_setVertexBytes: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexBuffer:
      return m_DummyReplayRenderCommandEncoder->Serialise_setVertexBuffer(ser, NULL, 0, 0);
    case MetalChunk::MTLRenderCommandEncoder_setVertexBufferOffset: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexBuffers: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexTexture: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexTextures: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexSamplerState: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexSamplerState_lodclamp:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexSamplerStates: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexSamplerStates_lodclamp:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexVisibleFunctionTable:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexVisibleFunctionTables:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexIntersectionFunctionTable:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexIntersectionFunctionTables:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexAccelerationStructure:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setViewport:
    {
      MTL::Viewport viewport;
      return m_DummyReplayRenderCommandEncoder->Serialise_setViewport(ser, viewport);
    }
    case MetalChunk::MTLRenderCommandEncoder_setViewports: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFrontFacingWinding: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVertexAmplificationCount: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setCullMode:
    {
      MTL::CullMode cullMode;
      return m_DummyReplayRenderCommandEncoder->Serialise_setCullMode(ser, cullMode);
    }
    case MetalChunk::MTLRenderCommandEncoder_setDepthClipMode: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setDepthBias: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setScissorRect: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setScissorRects: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTriangleFillMode: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentBytes: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentBuffer:
      return m_DummyReplayRenderCommandEncoder->Serialise_setFragmentBuffer(ser, NULL, 0, 0);
    case MetalChunk::MTLRenderCommandEncoder_setFragmentBufferOffset: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentBuffers: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentTexture:
      return m_DummyReplayRenderCommandEncoder->Serialise_setFragmentTexture(ser, NULL, 0);
    case MetalChunk::MTLRenderCommandEncoder_setFragmentTextures: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentSamplerState: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentSamplerState_lodclamp:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentSamplerStates: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentSamplerStates_lodclamp:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentVisibleFunctionTable:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentVisibleFunctionTables:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentIntersectionFunctionTable:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentIntersectionFunctionTables:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setFragmentAccelerationStructure:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setBlendColor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setDepthStencilState: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setStencilReferenceValue: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setStencilFrontReferenceValue:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setVisibilityResultMode: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setColorStoreAction: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setDepthStoreAction: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setStencilStoreAction: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setColorStoreActionOptions: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setDepthStoreActionOptions: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setStencilStoreActionOptions:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives:
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives_instanced:
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives_instanced_base:
      return m_DummyReplayRenderCommandEncoder->Serialise_drawPrimitives(
          ser, MTL::PrimitiveTypePoint, 0, 0, 0, 0);
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives_indirect: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_drawIndexedPrimitives:
    case MetalChunk::MTLRenderCommandEncoder_drawIndexedPrimitives_instanced:
    case MetalChunk::MTLRenderCommandEncoder_drawIndexedPrimitives_instanced_base:
      return m_DummyReplayRenderCommandEncoder->Serialise_drawIndexedPrimitives(
          ser, MTL::PrimitiveTypePoint, 0, MTL::IndexTypeUInt16, NULL, 0, 0, 0, 0);
    case MetalChunk::MTLRenderCommandEncoder_drawIndexedPrimitives_indirect:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_textureBarrier: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_updateFence: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_waitForFence: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTessellationFactorBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTessellationFactorScale: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_drawPatches: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_drawPatches_indirect: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_drawIndexedPatches: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_drawIndexedPatches_indirect: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileBytes: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileBufferOffset: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileBuffers: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileTexture: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileTextures: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileSamplerState: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileSamplerState_lodclamp:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileSamplerStates: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileSamplerStates_lodclamp:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileVisibleFunctionTable: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileVisibleFunctionTables:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileIntersectionFunctionTable:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileIntersectionFunctionTables:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setTileAccelerationStructure:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_dispatchThreadsPerTile: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_setThreadgroupMemoryLength: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useResource: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useResource_stages: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useResources: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useResources_stages: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useHeap: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useHeap_stages: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useHeaps: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_useHeaps_stages: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_executeCommandsInBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_executeCommandsInBuffer_indirect:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_memoryBarrierWithScope: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_memoryBarrierWithResources: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLRenderCommandEncoder_sampleCountersInBuffer: METAL_CHUNK_NOT_HANDLED();

    case MetalChunk::MTLBuffer_setPurgeableState: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_makeAliasable: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_contents: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_didModifyRange:
    {
      NS::Range range = NS::Range::Make(0, 0);
      return m_DummyBuffer->Serialise_didModifyRange(ser, range);
    }
    case MetalChunk::MTLBuffer_newTextureWithDescriptor: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_addDebugMarker: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_removeAllDebugMarkers: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_remoteStorageBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_newRemoteBufferViewForDevice: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBuffer_InternalModifyCPUContents:
      return m_DummyBuffer->Serialise_InternalModifyCPUContents(ser, 0, 0, NULL);

    case MetalChunk::MTLBlitCommandEncoder_setLabel: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_endEncoding:
      return m_DummyReplayBlitCommandEncoder->Serialise_endEncoding(ser);
    case MetalChunk::MTLBlitCommandEncoder_insertDebugSignpost: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_pushDebugGroup: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_popDebugGroup: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_synchronizeResource:
      return m_DummyReplayBlitCommandEncoder->Serialise_synchronizeResource(ser, NULL);
    case MetalChunk::MTLBlitCommandEncoder_synchronizeTexture: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromBuffer_toBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromBuffer_toTexture: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromBuffer_toTexture_options:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromTexture_toBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromTexture_toBuffer_options:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromTexture_toTexture: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromTexture_toTexture_slice_level_origin:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyFromTexture_toTexture_slice_level_count:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_generateMipmapsForTexture: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_fillBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_updateFence: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_waitForFence: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_getTextureAccessCounters: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_resetTextureAccessCounters: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_optimizeContentsForGPUAccess: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_optimizeContentsForGPUAccess_slice_level:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_optimizeContentsForCPUAccess: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_optimizeContentsForCPUAccess_slice_level:
      METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_resetCommandsInBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_copyIndirectCommandBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_optimizeIndirectCommandBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_sampleCountersInBuffer: METAL_CHUNK_NOT_HANDLED();
    case MetalChunk::MTLBlitCommandEncoder_resolveCounters: METAL_CHUNK_NOT_HANDLED();

    // no default to get compile error if a chunk is not handled
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
        return status;
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
  RDCLOG("M J Replay %d->%d Partial:%s", startEventID, endEventID, ToStr(partial).c_str());
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
  m_ReplayNextCmdBufferQueueIndex = 0;

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

    MetalRenderState::PipelineBinding partialEncoder = MetalRenderState::BindNone;

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
        partialEncoder = cmdBufInfo.activeEncoder;
        if(partialEncoder == MetalRenderState::BindRender)
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
              rpUnneeded ? MetalRenderState::BindNone : MetalRenderState::BindRender);
          RDCLOG(" S Begin Render %p", m_RenderState.renderCommandEncoder);
          RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
          RDCASSERT(!cmdBufInfo.renderState.blitCommandEncoder);
          cmdBufInfo.renderState.renderCommandEncoder = m_RenderState.renderCommandEncoder;
        }
        else if(partialEncoder == MetalRenderState::BindBlit)
        {
          m_RenderState.BeginBlitPass(this, cmdBuffer);
          RDCLOG(" S Begin Blit %p", m_RenderState.blitCommandEncoder);
          RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
          RDCASSERT(!cmdBufInfo.renderState.blitCommandEncoder);
          cmdBufInfo.renderState.blitCommandEncoder = m_RenderState.blitCommandEncoder;
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
      // even if it wasn't before (if the above event was a CmdBegin*Encoder).
      // If we began our own custom single-action loadrp, and it was ended by a CmdEnd*Encoder,
      // we need to reverse the virtual transitions we did above, as it won't happen otherwise
      MetalRenderState::PipelineBinding activeEncoder = MetalRenderState::BindNone;
      if(m_ReplayCurrentCmdBufferID != ResourceId())
      {
        ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
        activeEncoder = cmdBufInfo.activeEncoder;
        if(activeEncoder == MetalRenderState::BindRender)
        {
          RDCLOG(" S End Render %p", m_RenderState.renderCommandEncoder);
          m_RenderState.EndRenderPass();
          ClearActiveRenderCommandEncoder();
        }
        else if(activeEncoder == MetalRenderState::BindBlit)
        {
          RDCLOG(" S End Blit %p", m_RenderState.blitCommandEncoder);
          m_RenderState.EndBlitPass();
          ClearActiveBlitCommandEncoder();
        }
      }

      // we might have replayed a CmdBegin*Encoder or CmdEnd*Encoder,
      // but we want to keep the partial replay data state intact, so restore
      // whether or not a render pass was active.
      activeEncoder = partialEncoder;

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
  ResourceId parentId = GetResourceManager()->GetUnreplacedOriginalID(parentLive);

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

void WrappedMTLDevice::WaitForGPU()
{
  MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
  mtlCommandBuffer->commit();
  mtlCommandBuffer->waitUntilCompleted();
}

MetalTextureState *WrappedMTLDevice::FindTextureState(ResourceId id)
{
  SCOPED_LOCK(m_TextureStatesLock);
  auto it = m_TextureStates.find(id);
  if(it != m_TextureStates.end())
    return &it->second;
  else
    return NULL;
}

const MetalTextureState *WrappedMTLDevice::FindConstTextureState(ResourceId id)
{
  SCOPED_LOCK(m_TextureStatesLock);
  auto it = m_TextureStates.find(id);
  if(it != m_TextureStates.end())
    return &it->second;
  else
    return NULL;
}

MetalTextureState *WrappedMTLDevice::InsertTextureState(WrappedMTLTexture *wrappedHandle,
                                                        ResourceId id, const MetalTextureInfo &info,
                                                        FrameRefType refType, bool *inserted)
{
  SCOPED_LOCK(m_TextureStatesLock);
  auto it = m_TextureStates.find(id);
  if(it != m_TextureStates.end())
  {
    if(inserted != NULL)
      *inserted = false;
    return &it->second;
  }
  else
  {
    if(inserted != NULL)
      *inserted = true;
    it = m_TextureStates.insert({id, MetalTextureState(wrappedHandle, info, refType)}).first;
    return &it->second;
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

  //  GetResourceManager()->SerialiseTextureStates(ser, m_TextureStates);

  SERIALISE_CHECK_READ_ERRORS();

  return true;
}

void WrappedMTLDevice::StartFrameCapture(DeviceOwnedWindow devWnd)
{
  if(!IsBackgroundCapturing(m_State))
    return;

  RDCLOG("Starting capture");
  {
    SCOPED_LOCK(m_CaptureCommandBuffersLock);
    RDCASSERT(m_CaptureCommandBuffersSubmitted.empty());
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
        it->second.BeginCapture();
      }
    }
    m_State = CaptureState::ActiveCapturing;
  }

  GetResourceManager()->MarkResourceFrameReferenced(GetResID(this), eFrameRef_Read);

  // TODO: are there other resources that need to be marked as frame referenced
}

void WrappedMTLDevice::EndCaptureFrame(ResourceId backbuffer)
{
  CACHE_THREAD_SERIALISER();
  ser.SetActionChunk();
  SCOPED_SERIALISE_CHUNK(SystemChunk::CaptureEnd);

  SERIALISE_ELEMENT_LOCAL(PresentedImage, backbuffer).TypedAs("MTLTexture"_lit);

  m_FrameCaptureRecord->AddChunk(scope.Get());
}

bool WrappedMTLDevice::EndFrameCapture(DeviceOwnedWindow devWnd)
{
  if(!IsActiveCapturing(m_State))
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

  // atomically transition to IDLE
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);
    EndCaptureFrame(bbId);
    m_State = CaptureState::BackgroundCapturing;
  }

  {
    SCOPED_LOCK(m_CaptureCommandBuffersLock);
    // wait for the GPU to be idle
    for(MetalResourceRecord *record : m_CaptureCommandBuffersSubmitted)
    {
      WrappedMTLCommandBuffer *commandBuffer = (WrappedMTLCommandBuffer *)(record->m_Resource);
      Unwrap(commandBuffer)->waitUntilCompleted();
      // Remove the reference on the real resource added during commit()
      Unwrap(commandBuffer)->release();
    }

    if(m_CaptureCommandBuffersSubmitted.empty())
      WaitForGPU();
  }

  RenderDoc::FramePixels fp;

  MTL::Texture *mtlBackBuffer = Unwrap(backBuffer);

  // The backbuffer has to be a non-framebufferOnly texture
  // to be able to copy the pixels for the thumbnail
  if(!mtlBackBuffer->framebufferOnly())
  {
    const uint32_t maxSize = 2048;

    MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
    MTL::BlitCommandEncoder *mtlBlitEncoder = mtlCommandBuffer->blitCommandEncoder();

    uint32_t sourceWidth = (uint32_t)mtlBackBuffer->width();
    uint32_t sourceHeight = (uint32_t)mtlBackBuffer->height();
    MTL::Origin sourceOrigin(0, 0, 0);
    MTL::Size sourceSize(sourceWidth, sourceHeight, 1);

    MTL::PixelFormat format = mtlBackBuffer->pixelFormat();
    uint32_t bytesPerRow = GetByteSize(sourceWidth, 1, 1, format, 0);
    NS::UInteger bytesPerImage = sourceHeight * bytesPerRow;

    MTL::Buffer *mtlCpuPixelBuffer =
        Unwrap(this)->newBuffer(bytesPerImage, MTL::ResourceStorageModeShared);

    mtlBlitEncoder->copyFromTexture(mtlBackBuffer, 0, 0, sourceOrigin, sourceSize,
                                    mtlCpuPixelBuffer, 0, bytesPerRow, bytesPerImage);
    mtlBlitEncoder->endEncoding();

    mtlCommandBuffer->commit();
    mtlCommandBuffer->waitUntilCompleted();

    fp.len = (uint32_t)mtlCpuPixelBuffer->length();
    fp.data = new uint8_t[fp.len];
    memcpy(fp.data, mtlCpuPixelBuffer->contents(), fp.len);

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
  }

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
    // TODO: memory references

    // need over estimate of chunk size when writing directly to file
    {
      SCOPED_SERIALISE_CHUNK(SystemChunk::CaptureScope, 16);
      Serialise_CaptureScope(ser);
    }

    {
      uint64_t maxCaptureBeginChunkSizeInBytes = 16 + m_TextureStates.size() * 128;
      SCOPED_SERIALISE_CHUNK(SystemChunk::CaptureBegin, maxCaptureBeginChunkSizeInBytes);
      Serialise_BeginCaptureFrame(ser);
    }

    // don't need to lock access to m_CaptureCommandBuffersSubmitted as
    // no longer in active capture (the transition is thread-protected)
    // nothing will be pushed to the vector

    {
      std::map<int64_t, Chunk *> recordlist;
      size_t countCmdBuffers = m_CaptureCommandBuffersSubmitted.size();
      // ensure all command buffer records within the frame even if recorded before
      // serialised order must be preserved
      for(MetalResourceRecord *record : m_CaptureCommandBuffersSubmitted)
      {
        size_t prevSize = recordlist.size();
        (void)prevSize;
        record->Insert(recordlist);
      }

      size_t prevSize = recordlist.size();
      (void)prevSize;
      m_FrameCaptureRecord->Insert(recordlist);
      RDCDEBUG("Adding %zu/%zu frame capture chunks to file serialiser",
               recordlist.size() - prevSize, recordlist.size());

      float num = float(recordlist.size());
      float idx = 0.0f;

      for(auto it = recordlist.begin(); it != recordlist.end(); ++it)
      {
        RenderDoc::Inst().SetProgress(CaptureProgress::SerialiseFrameContents, idx / num);
        idx += 1.0f;
        it->second->Write(ser);
      }
    }
    captureSectionSize = captureWriter->GetOffset();
  }

  RDCLOG("Captured Metal frame with %f MB capture section in %f seconds",
         double(captureSectionSize) / (1024.0 * 1024.0), m_CaptureTimer.GetMilliseconds() / 1000.0);

  RenderDoc::Inst().FinishCaptureWriting(rdc, m_CapturedFrames.back().frameNumber);

  // delete tracked cmd buffers - had to keep them alive until after serialiser flush.
  CaptureClearSubmittedCmdBuffers();

  GetResourceManager()->ResetLastWriteTimes();
  GetResourceManager()->MarkUnwrittenResources();

  // TODO: handle memory resources in the resource manager
  // GetResourceManager()->ClearReferencedMemory();

  GetResourceManager()->ClearReferencedResources();
  GetResourceManager()->FreeInitialContents();

  // TODO: handle memory resources in the initial contents

  return true;
}

bool WrappedMTLDevice::DiscardFrameCapture(DeviceOwnedWindow devWnd)
{
  if(!IsActiveCapturing(m_State))
    return true;

  RDCLOG("Discarding frame capture.");

  RenderDoc::Inst().FinishCaptureWriting(NULL, m_CapturedFrames.back().frameNumber);

  m_CapturedFrames.pop_back();

  // atomically transition to IDLE
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);
    m_State = CaptureState::BackgroundCapturing;
  }

  CaptureClearSubmittedCmdBuffers();

  GetResourceManager()->MarkUnwrittenResources();

  // TODO: handle memory resources in the resource manager

  GetResourceManager()->ClearReferencedResources();
  GetResourceManager()->FreeInitialContents();

  // TODO: handle memory resources in the initial contents

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

void WrappedMTLDevice::CaptureCmdBufSubmit(MetalResourceRecord *record)
{
  RDCASSERTEQUAL(record->cmdInfo->status, MetalCmdBufferStatus::Submitted);
  RDCASSERT(IsCaptureMode(m_State));
  WrappedMTLCommandBuffer *commandBuffer = (WrappedMTLCommandBuffer *)(record->m_Resource);
  if(IsActiveCapturing(m_State))
  {
    std::unordered_set<ResourceId> refIDs;
    // The record will get deleted at the end of active frame capture
    record->AddRef();
    record->AddReferencedIDs(refIDs);
    // snapshot/detect any CPU modifications to the contents
    // of referenced MTLBuffer with shared storage mode
    for(auto it = refIDs.begin(); it != refIDs.end(); ++it)
    {
      ResourceId id = *it;
      MetalResourceRecord *refRecord = GetResourceManager()->GetResourceRecord(id);
      if(refRecord->m_Type == eResBuffer)
      {
        MetalBufferInfo *bufInfo = refRecord->bufInfo;
        if(bufInfo->storageMode == MTL::StorageModeShared)
        {
          size_t diffStart = 0;
          size_t diffEnd = bufInfo->length;
          bool foundDifference = true;
          if(!bufInfo->baseSnapshot.isEmpty())
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
              RDCERR("Writing buffer memory %s that is NULL", ToStr(id).c_str());
              continue;
            }
            Chunk *chunk = NULL;
            {
              CACHE_THREAD_SERIALISER();
              SCOPED_SERIALISE_CHUNK(MetalChunk::MTLBuffer_InternalModifyCPUContents);
              ((WrappedMTLBuffer *)refRecord->m_Resource)
                  ->Serialise_InternalModifyCPUContents(ser, diffStart, diffEnd, bufInfo);
              chunk = scope.Get();
            }
            record->AddChunk(chunk);
          }
        }
      }
    }
    record->MarkResourceFrameReferenced(GetResID(commandBuffer->GetCommandQueue()), eFrameRef_Read);
    // pull in frame refs from this command buffer
    record->AddResourceReferences(GetResourceManager());
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCommandBuffer_commit);
      commandBuffer->Serialise_commit(ser);
      chunk = scope.Get();
    }
    record->AddChunk(chunk);
    m_CaptureCommandBuffersSubmitted.push_back(record);
  }
  else
  {
    // Remove the reference on the real resource added during commit()
    Unwrap(commandBuffer)->release();
  }
  if(record->cmdInfo->presented)
  {
    AdvanceFrame();
    Present(record);
  }
  // In background or active capture mode the record reference is incremented in
  // CaptureCmdBufEnqueue
  record->Delete(GetResourceManager());
}

void WrappedMTLDevice::CaptureCmdBufCommit(MetalResourceRecord *cbRecord)
{
  SCOPED_LOCK(m_CaptureCommandBuffersLock);
  if(cbRecord->cmdInfo->status != MetalCmdBufferStatus::Enqueued)
    CaptureCmdBufEnqueue(cbRecord);

  RDCASSERTEQUAL(cbRecord->cmdInfo->status, MetalCmdBufferStatus::Enqueued);
  cbRecord->cmdInfo->status = MetalCmdBufferStatus::Committed;

  size_t countSubmitted = 0;
  for(MetalResourceRecord *record : m_CaptureCommandBuffersEnqueued)
  {
    if(record->cmdInfo->status == MetalCmdBufferStatus::Committed)
    {
      record->cmdInfo->status = MetalCmdBufferStatus::Submitted;
      ++countSubmitted;
      CaptureCmdBufSubmit(record);
      continue;
    }
    break;
  };
  m_CaptureCommandBuffersEnqueued.erase(0, countSubmitted);
}

void WrappedMTLDevice::CaptureCmdBufEnqueue(MetalResourceRecord *cbRecord)
{
  SCOPED_LOCK(m_CaptureCommandBuffersLock);
  RDCASSERTEQUAL(cbRecord->cmdInfo->status, MetalCmdBufferStatus::Unknown);
  cbRecord->cmdInfo->status = MetalCmdBufferStatus::Enqueued;
  cbRecord->AddRef();
  m_CaptureCommandBuffersEnqueued.push_back(cbRecord);

  RDCDEBUG("Enqueing CommandBufferRecord %s %d", ToStr(cbRecord->GetResourceID()).c_str(),
           m_CaptureCommandBuffersEnqueued.count());
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
    RenderDoc::Inst().StartFrameCapture(DeviceOwnedWindow(this, NULL));

    m_AppControlledCapture = false;
    m_CapturedFrames.back().frameNumber = 0;
  }
}

void WrappedMTLDevice::Present(MetalResourceRecord *record)
{
  WrappedMTLTexture *backBuffer = record->cmdInfo->backBuffer;
  {
    SCOPED_LOCK(m_CapturePotentialBackBuffersLock);
    if(m_CapturePotentialBackBuffers.count(backBuffer) == 0)
    {
      RDCERR("Capture ignoring Present called on unknown backbuffer");
      return;
    }
  }

  CA::MetalLayer *outputLayer = record->cmdInfo->outputLayer;
  DeviceOwnedWindow devWnd(this, outputLayer);

  bool activeWindow = RenderDoc::Inst().IsActiveWindow(devWnd);

  RenderDoc::Inst().AddActiveDriver(RDCDriver::Metal, true);

  if(!activeWindow)
    return;

  if(IsActiveCapturing(m_State) && !m_AppControlledCapture)
  {
    RDCASSERT(m_CapturedBackbuffer == NULL);
    m_CapturedBackbuffer = backBuffer;
    RenderDoc::Inst().EndFrameCapture(devWnd);
  }

  if(RenderDoc::Inst().ShouldTriggerCapture(m_FrameCounter) && IsBackgroundCapturing(m_State))
  {
    RenderDoc::Inst().StartFrameCapture(devWnd);

    m_AppControlledCapture = false;
    m_CapturedFrames.back().frameNumber = m_FrameCounter;
  }
}

void WrappedMTLDevice::CaptureClearSubmittedCmdBuffers()
{
  SCOPED_LOCK(m_CaptureCommandBuffersLock);
  for(MetalResourceRecord *record : m_CaptureCommandBuffersSubmitted)
  {
    record->Delete(GetResourceManager());
  }

  m_CaptureCommandBuffersSubmitted.clear();
}

void WrappedMTLDevice::RegisterMetalLayer(CA::MetalLayer *mtlLayer)
{
  SCOPED_LOCK(m_CaptureOutputLayersLock);
  if(m_CaptureOutputLayers.count(mtlLayer) == 0)
  {
    m_CaptureOutputLayers.insert(mtlLayer);
    TrackedCAMetalLayer::Track(mtlLayer, this);

    DeviceOwnedWindow devWnd(this, mtlLayer);
    RenderDoc::Inst().AddFrameCapturer(devWnd, &m_Capturer);
  }
}

void WrappedMTLDevice::UnregisterMetalLayer(CA::MetalLayer *mtlLayer)
{
  SCOPED_LOCK(m_CaptureOutputLayersLock);
  RDCASSERT(m_CaptureOutputLayers.count(mtlLayer));
  m_CaptureOutputLayers.erase(mtlLayer);

  DeviceOwnedWindow devWnd(this, mtlLayer);
  RenderDoc::Inst().RemoveFrameCapturer(devWnd);
}

void WrappedMTLDevice::RegisterDrawableInfo(CA::MetalDrawable *caMtlDrawable)
{
  MetalDrawableInfo drawableInfo;
  drawableInfo.mtlLayer = caMtlDrawable->layer();
  drawableInfo.texture = GetWrapped(caMtlDrawable->texture());
  drawableInfo.drawableID = caMtlDrawable->drawableID();
  SCOPED_LOCK(m_CaptureDrawablesLock);
  RDCASSERTEQUAL(m_CaptureDrawableInfos.find(caMtlDrawable), m_CaptureDrawableInfos.end());
  m_CaptureDrawableInfos[caMtlDrawable] = drawableInfo;
}

MetalDrawableInfo WrappedMTLDevice::UnregisterDrawableInfo(MTL::Drawable *mtlDrawable)
{
  MetalDrawableInfo drawableInfo;
  {
    SCOPED_LOCK(m_CaptureDrawablesLock);
    auto it = m_CaptureDrawableInfos.find(mtlDrawable);
    if(it != m_CaptureDrawableInfos.end())
    {
      drawableInfo = it->second;
      m_CaptureDrawableInfos.erase(it);
      return drawableInfo;
    }
  }
  // Not found by pointer fall back and check by drawableID
  NS::UInteger drawableID = mtlDrawable->drawableID();
  for(auto it = m_CaptureDrawableInfos.begin(); it != m_CaptureDrawableInfos.end(); ++it)
  {
    drawableInfo = it->second;
    if(drawableInfo.drawableID == drawableID)
    {
      m_CaptureDrawableInfos.erase(it);
      return drawableInfo;
    }
  }
  drawableInfo.mtlLayer = NULL;
  drawableInfo.texture = NULL;
  return drawableInfo;
}

void WrappedMTLDevice::ClearActiveRenderCommandEncoder()
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(cmdBufInfo.activeEncoder == MetalRenderState::BindRender);
  RDCASSERT(cmdBufInfo.renderState.renderCommandEncoder);
  RDCASSERT(!cmdBufInfo.renderState.blitCommandEncoder);
  cmdBufInfo.renderState.renderCommandEncoder = NULL;
  cmdBufInfo.activeEncoder = MetalRenderState::BindNone;
}

void WrappedMTLDevice::SetActiveRenderCommandEncoder(WrappedMTLRenderCommandEncoder *renderCommandEncoder)
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  RDCASSERT(renderCommandEncoder);
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
  RDCASSERT(cmdBufInfo.activeEncoder == MetalRenderState::BindNone);
  RDCASSERT(!cmdBufInfo.renderState.blitCommandEncoder);
  cmdBufInfo.renderState.renderCommandEncoder = renderCommandEncoder;
  cmdBufInfo.activeEncoder = MetalRenderState::BindRender;
}

WrappedMTLRenderCommandEncoder *WrappedMTLDevice::GetCurrentReplayRenderEncoder()
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(cmdBufInfo.activeEncoder == MetalRenderState::BindRender);
  RDCASSERT(cmdBufInfo.renderState.renderCommandEncoder);
  RDCASSERT(!cmdBufInfo.renderState.blitCommandEncoder);
  return cmdBufInfo.renderState.renderCommandEncoder;
}

void WrappedMTLDevice::ClearActiveBlitCommandEncoder()
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(cmdBufInfo.activeEncoder == MetalRenderState::BindBlit);
  RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
  RDCASSERT(cmdBufInfo.renderState.blitCommandEncoder);
  cmdBufInfo.renderState.blitCommandEncoder = NULL;
  cmdBufInfo.activeEncoder = MetalRenderState::BindNone;
}

void WrappedMTLDevice::SetActiveBlitCommandEncoder(WrappedMTLBlitCommandEncoder *blitCommandEncoder)
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  RDCASSERT(blitCommandEncoder);
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(cmdBufInfo.activeEncoder == MetalRenderState::BindNone);
  RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
  RDCASSERT(!cmdBufInfo.renderState.blitCommandEncoder);
  cmdBufInfo.renderState.blitCommandEncoder = blitCommandEncoder;
  cmdBufInfo.activeEncoder = MetalRenderState::BindBlit;
}

WrappedMTLBlitCommandEncoder *WrappedMTLDevice::GetCurrentReplayBlitEncoder()
{
  RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  RDCASSERT(cmdBufInfo.activeEncoder == MetalRenderState::BindBlit);
  RDCASSERT(!cmdBufInfo.renderState.renderCommandEncoder);
  RDCASSERT(cmdBufInfo.renderState.blitCommandEncoder);
  return cmdBufInfo.renderState.blitCommandEncoder;
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
  uint32_t startEID = cmdBufInfo.baseRootEventID;
  uint32_t cmdBufCurEID = cmdBufInfo.curEventID;
  uint32_t rebasedEID = startEID + cmdBufCurEID;
  return rebasedEID <= m_LastEventID;
}

void WrappedMTLDevice::NewReplayCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  ResourceId cmdBufferID = GetResourceManager()->GetUnreplacedOriginalID(GetResID(cmdBuffer));
  RDCASSERT(cmdBufferID != ResourceId());

  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[cmdBufferID];
  RDCASSERT(!cmdBufInfo.action);
  RDCASSERT(cmdBufInfo.actionStack.empty());

  MetalActionTreeNode *action = new MetalActionTreeNode;
  cmdBufInfo.action = action;
  cmdBufInfo.actionCount = 0;
  cmdBufInfo.baseRootActionID = 0;
  cmdBufInfo.baseRootEventID = 0;
  cmdBufInfo.eventCount = 0;
  cmdBufInfo.actionStack.push_back(action);
  cmdBufInfo.beginChunk = uint32_t(m_StructuredFile->chunks.size() - 1);
  cmdBufInfo.endChunk = 0;
  cmdBufInfo.queueIndex = 0;
  ResetReplayCommandBuffer(cmdBuffer);
}

void WrappedMTLDevice::ResetReplayCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  ResourceId cmdID = GetResourceManager()->GetUnreplacedOriginalID(GetResID(cmdBuffer));
  RDCASSERT(cmdID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[cmdID];
  cmdBufInfo.curEventID = 0;
  cmdBufInfo.activeEncoder = MetalRenderState::BindNone;
  cmdBufInfo.renderState.renderCommandEncoder = NULL;
  cmdBufInfo.renderState.blitCommandEncoder = NULL;
  cmdBufInfo.cmdBuffer = cmdBuffer;
}

void WrappedMTLDevice::ReplayCommandBufferEnqueue(WrappedMTLCommandBuffer *cmdBuffer)
{
  SetCurrentCommandBuffer(NULL);
  ResourceId cmdID = GetResourceManager()->GetUnreplacedOriginalID(GetResID(cmdBuffer));
  RDCASSERT(cmdID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[cmdID];
  if(IsLoading(m_State))
    cmdBufInfo.queueIndex = Atomic::Inc32(&m_ReplayNextCmdBufferQueueIndex);
}

void WrappedMTLDevice::ReplayCommandBufferCommit(WrappedMTLCommandBuffer *cmdBuffer)
{
  SetCurrentCommandBuffer(NULL);
  if(cmdBuffer == m_ReplayPartialCmdBuffer)
    return;
  ResourceId cmdID = GetResourceManager()->GetUnreplacedOriginalID(GetResID(cmdBuffer));
  RDCASSERT(cmdID != ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[cmdID];
  if(IsLoading(m_State))
  {
    if(cmdBufInfo.queueIndex == 0)
      ReplayCommandBufferEnqueue(cmdBuffer);

    uint32_t currentRootEventID = m_RootEventID;
    uint32_t currentRootActionID = m_RootActionID;
    uint32_t cmdBufStart = currentRootEventID;

    AddEvent();
    m_RootEventID++;

    cmdBufInfo.eventCount = cmdBufInfo.curEventID;
    cmdBufInfo.curEventID = 0;
    cmdBufInfo.endChunk = uint32_t(m_StructuredFile->chunks.size() - 1);

    // add a fake marker
    ActionDescription action;
    action.customName = StringFormat::Fmt("=> %d Commit : StartCommandBuffer(%s)",
                                          cmdBufInfo.queueIndex, ToStr(cmdID).c_str());
    action.flags |= ActionFlags::CommandBufferBoundary;
    AddEvent();

    m_RootEvents.back().chunkIndex = cmdBufInfo.beginChunk;
    m_Events.back().chunkIndex = cmdBufInfo.beginChunk;

    AddAction(action);
    m_RootEventID++;

    cmdBufInfo.baseRootEventID = m_RootEventID;
    cmdBufInfo.baseRootActionID = m_RootActionID;

    uint32_t countEvents = cmdBufInfo.eventCount;
    uint32_t countActions = 1 + cmdBufInfo.actionCount;

    m_RootEventID = cmdBufInfo.baseRootEventID;
    m_RootActionID = cmdBufInfo.baseRootActionID;

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

    action.customName = StringFormat::Fmt("=> %d Commit : EndCommandBuffer(%s)",
                                          cmdBufInfo.queueIndex, ToStr(cmdID).c_str());
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
    RDCASSERTEQUAL(countEvents, m_RootEventID - cmdBufInfo.baseRootEventID);
    RDCASSERTEQUAL(countActions, m_RootActionID - cmdBufInfo.baseRootActionID);
  }
  else
  {
    int32_t nextIndex = Atomic::Inc32(&m_ReplayNextCmdBufferQueueIndex);
    uint32_t startEID = cmdBufInfo.baseRootEventID;
    uint32_t cmdBufCurEID = cmdBufInfo.curEventID;

    // skip past the CommandBuffer commit event
    m_RootEventID++;

    m_RootEventID += cmdBufInfo.eventCount;
    m_RootActionID += cmdBufInfo.actionCount;

    // 2 extra for the StartCommandBuffer & EndCommandBuffer markers
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
      if(cmdBufInfo.activeEncoder == MetalRenderState::BindRender)
      {
        RDCASSERT(cmdBufInfo.renderState.renderCommandEncoder);
        RDCLOG(" S Commit End Render %p", cmdBufInfo.renderState.renderCommandEncoder);
        cmdBufInfo.renderState.renderCommandEncoder->endEncoding();
        cmdBufInfo.renderState.renderCommandEncoder = NULL;
      }
      else if(cmdBufInfo.activeEncoder == MetalRenderState::BindBlit)
      {
        RDCASSERT(cmdBufInfo.renderState.blitCommandEncoder);
        RDCLOG(" S Commit End Blit %p", cmdBufInfo.renderState.blitCommandEncoder);
        cmdBufInfo.renderState.blitCommandEncoder->endEncoding();
        cmdBufInfo.renderState.blitCommandEncoder = NULL;
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

void WrappedMTLDevice::SetCurrentCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer)
{
  m_ReplayCurrentCmdBufferID = GetResourceManager()->GetUnreplacedOriginalID(GetResID(cmdBuffer));
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

void WrappedMTLDevice::SetCurrentCommandBufferRenderPassDescriptor(
    const RDMTL::RenderPassDescriptor &rpDesc)
{
  RDCASSERTNOTEQUAL(m_ReplayCurrentCmdBufferID, ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  cmdBufInfo.rpDesc = rpDesc;
}

rdcstr WrappedMTLDevice::MakeRenderPassOpString(bool startPass)
{
  RDCASSERTNOTEQUAL(m_ReplayCurrentCmdBufferID, ResourceId());
  ReplayCmdBufferInfo &cmdBufInfo = m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID];
  const RDMTL::RenderPassDescriptor &rpDesc = cmdBufInfo.rpDesc;
  rdcstr message = "";
  if(rpDesc.colorAttachments.count())
  {
    message += "C=";
    if(startPass)
    {
      if(rpDesc.colorAttachments[0].loadAction == MTL::LoadActionLoad)
        message += "L";
      else if(rpDesc.colorAttachments[0].loadAction == MTL::LoadActionClear)
        message += "C";
      else if(rpDesc.colorAttachments[0].loadAction == MTL::LoadActionDontCare)
        message += "DC";
    }
    else
    {
      if(rpDesc.colorAttachments[0].storeAction == MTL::StoreActionStore)
        message += "S";
      else if(rpDesc.colorAttachments[0].storeAction == MTL::StoreActionDontCare)
        message += "DC";
    }
  }
  if(rpDesc.depthAttachment.texture != NULL)
  {
    message += " D=";
    if(startPass)
    {
      if(rpDesc.depthAttachment.loadAction == MTL::LoadActionLoad)
        message += "L";
      else if(rpDesc.depthAttachment.loadAction == MTL::LoadActionClear)
        message += "C";
      else if(rpDesc.depthAttachment.loadAction == MTL::LoadActionDontCare)
        message += "DC";
    }
    else
    {
      if(rpDesc.depthAttachment.storeAction == MTL::StoreActionStore)
        message += "S";
      else if(rpDesc.depthAttachment.storeAction == MTL::StoreActionDontCare)
        message += "DC";
    }
  }
  return message;
}

MetalInitParams::MetalInitParams()
{
  memset(this, 0, sizeof(MetalInitParams));
}

uint64_t MetalInitParams::GetSerialiseSize()
{
  size_t ret = sizeof(*this);
  return (uint64_t)ret;
}

bool MetalInitParams::IsSupportedVersion(uint64_t ver)
{
  if(ver == CurrentVersion)
    return true;

  return false;
}

void MetalInitParams::Set(MTL::Device *pRealDevice, ResourceId device)
{
  DeviceID = device;
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalInitParams &el)
{
  SERIALISE_MEMBER(DeviceID).TypedAs("MTLDevice"_lit);
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
