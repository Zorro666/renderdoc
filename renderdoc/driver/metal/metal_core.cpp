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

#include "metal_core.h"
#include "core/core.h"
#include "core/settings.h"
#include "serialise/rdcfile.h"
#include "metal_manager.h"
#include "metal_metal.h"
#include "metal_replay.h"

RDOC_EXTERN_CONFIG(bool, Metal_Debug_VerboseCommandRecording);

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

void WrappedMTLDevice::Present(void *wnd)
{
  bool activeWindow = wnd == NULL || RenderDoc::Inst().IsActiveWindow(this, wnd);

  RenderDoc::Inst().AddActiveDriver(RDCDriver::Metal, true);

  if(!activeWindow)
    return;

  if(IsActiveCapturing(m_State) && !m_AppControlledCapture)
    RenderDoc::Inst().EndFrameCapture(this, wnd);

  if(RenderDoc::Inst().ShouldTriggerCapture(m_FrameCounter) && IsBackgroundCapturing(m_State))
  {
    RenderDoc::Inst().StartFrameCapture(this, wnd);

    m_AppControlledCapture = false;

    FrameDescription frame;
    frame.frameNumber = m_AppControlledCapture ? ~0U : m_FrameCounter;
    frame.captureTime = Timing::GetUnixTimestamp();
    m_CapturedFrames.push_back(frame);

    m_CapturedFrames.back().frameNumber = m_FrameCounter;
  }
}

void WrappedMTLDevice::EndCaptureFrame()
{
  CACHE_THREAD_SERIALISER();
  ser.SetActionChunk();
  SCOPED_SERIALISE_CHUNK(SystemChunk::CaptureEnd);

  // SERIALISE_ELEMENT_LOCAL(PresentedImage, GetResID(presentImage)).TypedAs("VkImage"_lit);

  m_FrameCaptureRecord->AddChunk(scope.Get());
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

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_BeginCaptureFrame(SerialiserType &ser)
{
  SCOPED_LOCK(m_ImageStatesLock);

  //  for(auto it = m_ImageStates.begin(); it != m_ImageStates.end(); ++it)
  //  {
  //    it->second.LockWrite()->FixupStorageReferences();
  //  }

  GetResourceManager()->SerialiseImageStates(ser, m_ImageStates);

  SERIALISE_CHECK_READ_ERRORS();

  return true;
}

void WrappedMTLDevice::StartFrameCapture(void *dev, void *wnd)
{
  if(!IsBackgroundCapturing(m_State))
    return;

  RDCLOG("Starting capture");

  Atomic::Dec32(&m_ReuseEnabled);

  m_CaptureTimer.Restart();

  GetResourceManager()->ResetCaptureStartTime();

  m_AppControlledCapture = true;

  m_SubmitCounter = 0;

  FrameDescription frame;
  frame.frameNumber = ~0U;
  frame.captureTime = Timing::GetUnixTimestamp();
  m_CapturedFrames.push_back(frame);

  GetResourceManager()->ClearReferencedResources();
  // GetResourceManager()->ClearReferencedMemory();

  // need to do all this atomically so that no other commands
  // will check to see if they need to markdirty or markpendingdirty
  // and go into the frame record.
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);
    /*
      // wait for all work to finish and apply a memory barrier to ensure all memory is visible
      for(size_t i = 0; i < m_QueueFamilies.size(); i++)
      {
        for(uint32_t q = 0; q < m_QueueFamilyCounts[i]; q++)
        {
          if(m_QueueFamilies[i][q] != VK_NULL_HANDLE)
            ObjDisp(m_QueueFamilies[i][q])->QueueWaitIdle(Unwrap(m_QueueFamilies[i][q]));
        }
      }

      {
        VkMemoryBarrier memBarrier = {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER, NULL, VK_ACCESS_ALL_WRITE_BITS,
      VK_ACCESS_ALL_READ_BITS,
        };

        VkCommandBuffer cmd = GetNextCmd();

        VkResult vkr = VK_SUCCESS;

        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
                                              VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        vkr = ObjDisp(cmd)->BeginCommandBuffer(Unwrap(cmd), &beginInfo);
        RDCASSERTEQUAL(vkr, VK_SUCCESS);

        DoPipelineBarrier(cmd, 1, &memBarrier);

        vkr = ObjDisp(cmd)->EndCommandBuffer(Unwrap(cmd));
        RDCASSERTEQUAL(vkr, VK_SUCCESS);
      }
      */

    GetResourceManager()->PrepareInitialContents();
    /*
     SubmitAndFlushImageStateBarriers(m_setupImageBarriers);
    SubmitCmds();
    FlushQ();
    SubmitAndFlushImageStateBarriers(m_cleanupImageBarriers);

    {
      SCOPED_LOCK(m_CapDescriptorsLock);
      m_CapDescriptors.clear();
    }
     */

    RDCDEBUG("Attempting capture");
    m_FrameCaptureRecord->DeleteChunks();
    {
      SCOPED_LOCK(m_ImageStatesLock);
      for(auto it = m_ImageStates.begin(); it != m_ImageStates.end(); ++it)
      {
        it->second.LockWrite()->BeginCapture();
      }
    }
    m_State = CaptureState::ActiveCapturing;
  }

  GetResourceManager()->MarkResourceFrameReferenced(GetResID(this), eFrameRef_Read);
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

bool WrappedMTLDevice::EndFrameCapture(void *dev, void *wnd)
{
  if(!IsActiveCapturing(m_State))
    return true;
  /*
  VkSwapchainKHR swap = VK_NULL_HANDLE;

  if(wnd)
  {
    {
      SCOPED_LOCK(m_SwapLookupLock);
      auto it = m_SwapLookup.find(wnd);
      if(it != m_SwapLookup.end())
        swap = it->second;
    }

    if(swap == VK_NULL_HANDLE)
    {
      RDCERR("Output window %p provided for frame capture corresponds with no known swap chain",
  wnd);
      return false;
    }
  }
   */

  RDCLOG("Finished capture, Frame %u", m_CapturedFrames.back().frameNumber);

  /*
VkImage backbuffer = VK_NULL_HANDLE;
const PresentInfo *presentInfo = NULL;
VkResourceRecord *swaprecord = NULL;

if(swap != VK_NULL_HANDLE)
{
  GetResourceManager()->MarkResourceFrameReferenced(GetResID(swap), eFrameRef_Read);

  swaprecord = GetRecord(swap);
  RDCASSERT(swaprecord->swapInfo);

  const SwapchainInfo &swapInfo = *swaprecord->swapInfo;

  presentInfo = &swapInfo.lastPresent;
  backbuffer = swapInfo.images[presentInfo->imageIndex].im;

  // mark all images referenced as well
  for(size_t i = 0; i < swapInfo.images.size(); i++)
    GetResourceManager()->MarkResourceFrameReferenced(GetResID(swapInfo.images[i].im),
                                                      eFrameRef_Read);
}
else
{
  // if a swapchain wasn't specified or found, use the last one presented
  swaprecord = GetResourceManager()->GetResourceRecord(m_LastSwap);

  if(swaprecord)
  {
    GetResourceManager()->MarkResourceFrameReferenced(swaprecord->GetResourceID(), eFrameRef_Read);
    RDCASSERT(swaprecord->swapInfo);

    const SwapchainInfo &swapInfo = *swaprecord->swapInfo;

    presentInfo = &swapInfo.lastPresent;
    backbuffer = swapInfo.images[presentInfo->imageIndex].im;

    // mark all images referenced as well
    for(size_t i = 0; i < swapInfo.images.size(); i++)
      GetResourceManager()->MarkResourceFrameReferenced(GetResID(swapInfo.images[i].im),
                                                        eFrameRef_Read);
  }
}
   */

  // rdcarray<VkDeviceMemory> DeadMemories;
  // rdcarray<VkBuffer> DeadBuffers;

  // transition back to IDLE atomically
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);
    EndCaptureFrame();

    m_State = CaptureState::BackgroundCapturing;

    // m_SuccessfulCapture = false;

    // ObjDisp(GetDev())->DeviceWaitIdle(Unwrap(GetDev()));

    /*
     {
      SCOPED_LOCK(m_CoherentMapsLock);
      for(auto it = m_CoherentMaps.begin(); it != m_CoherentMaps.end(); ++it)
      {
        FreeAlignedBuffer((*it)->memMapState->refData);
        (*it)->memMapState->refData = NULL;
        (*it)->memMapState->needRefData = false;
      }
    }

    DeadMemories.swap(m_DeviceAddressResources.DeadMemories);
    DeadBuffers.swap(m_DeviceAddressResources.DeadBuffers);
  */
  }

  RenderDoc::FramePixels fp;
  /*
  for(VkDeviceMemory m : DeadMemories)
    vkFreeMemory(m_Device, m, NULL);

  for(VkBuffer b : DeadBuffers)
    vkDestroyBuffer(m_Device, b, NULL);

  // gather backbuffer screenshot
  const uint32_t maxSize = 2048;
  RenderDoc::FramePixels fp;

  if(swaprecord != NULL)
  {
    VkDevice device = GetDev();
    VkCommandBuffer cmd = GetNextCmd();

    const VkDevDispatchTable *vt = ObjDisp(device);

    vt->DeviceWaitIdle(Unwrap(device));

    const SwapchainInfo &swapInfo = *swaprecord->swapInfo;

    // since this happens during capture, we don't want to start serialising extra buffer creates,
    // so we manually create & then just wrap.
    VkBuffer readbackBuf = VK_NULL_HANDLE;

    VkResult vkr = VK_SUCCESS;

    // create readback buffer
    VkBufferCreateInfo bufInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0,
        GetByteSize(swapInfo.imageInfo.extent.width, swapInfo.imageInfo.extent.height, 1,
                    swapInfo.imageInfo.format, 0),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    vt->CreateBuffer(Unwrap(device), &bufInfo, NULL, &readbackBuf);
    RDCASSERTEQUAL(vkr, VK_SUCCESS);

    GetResourceManager()->WrapResource(Unwrap(device), readbackBuf);

    MemoryAllocation readbackMem =
        AllocateMemoryForResource(readbackBuf, MemoryScope::InitialContents, MemoryType::Readback);

    vkr = vt->BindBufferMemory(Unwrap(device), Unwrap(readbackBuf), Unwrap(readbackMem.mem),
                               readbackMem.offs);
    RDCASSERTEQUAL(vkr, VK_SUCCESS);

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
                                          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    // do image copy
    vkr = vt->BeginCommandBuffer(Unwrap(cmd), &beginInfo);
    RDCASSERTEQUAL(vkr, VK_SUCCESS);

    uint32_t rowPitch =
        GetByteSize(swapInfo.imageInfo.extent.width, 1, 1, swapInfo.imageInfo.format, 0);

    VkBufferImageCopy cpy = {
        0,
        0,
        0,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {
            0, 0, 0,
        },
        {swapInfo.imageInfo.extent.width, swapInfo.imageInfo.extent.height, 1},
    };

    VkResourceRecord *queueRecord = GetRecord(swapInfo.lastPresent.presentQueue);
    uint32_t swapQueueIndex = queueRecord->queueFamilyIndex;

    VkImageMemoryBarrier bbBarrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapQueueIndex,
        m_QueueFamilyIdx,
        Unwrap(backbuffer),
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };

    if(swapInfo.shared)
      bbBarrier.oldLayout = VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;

    DoPipelineBarrier(cmd, 1, &bbBarrier);

    if(swapQueueIndex != m_QueueFamilyIdx)
    {
      VkCommandBuffer extQCmd = GetExtQueueCmd(swapQueueIndex);

      vkr = vt->BeginCommandBuffer(Unwrap(extQCmd), &beginInfo);
      RDCASSERTEQUAL(vkr, VK_SUCCESS);

      DoPipelineBarrier(extQCmd, 1, &bbBarrier);

      ObjDisp(extQCmd)->EndCommandBuffer(Unwrap(extQCmd));

      SubmitAndFlushExtQueue(swapQueueIndex);
    }

    vt->CmdCopyImageToBuffer(Unwrap(cmd), Unwrap(backbuffer), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             Unwrap(readbackBuf), 1, &cpy);

    // barrier to switch backbuffer back to present layout
    std::swap(bbBarrier.oldLayout, bbBarrier.newLayout);
    std::swap(bbBarrier.srcAccessMask, bbBarrier.dstAccessMask);
    std::swap(bbBarrier.srcQueueFamilyIndex, bbBarrier.dstQueueFamilyIndex);

    VkBufferMemoryBarrier bufBarrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        NULL,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_HOST_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        Unwrap(readbackBuf),
        0,
        bufInfo.size,
    };

    DoPipelineBarrier(cmd, 1, &bbBarrier);
    DoPipelineBarrier(cmd, 1, &bufBarrier);

    vkr = vt->EndCommandBuffer(Unwrap(cmd));
    RDCASSERTEQUAL(vkr, VK_SUCCESS);

    SubmitCmds();
    FlushQ();    // need to wait so we can readback

    if(swapQueueIndex != m_QueueFamilyIdx)
    {
      VkCommandBuffer extQCmd = GetExtQueueCmd(swapQueueIndex);

      vkr = vt->BeginCommandBuffer(Unwrap(extQCmd), &beginInfo);
      RDCASSERTEQUAL(vkr, VK_SUCCESS);

      DoPipelineBarrier(extQCmd, 1, &bbBarrier);

      ObjDisp(extQCmd)->EndCommandBuffer(Unwrap(extQCmd));

      SubmitAndFlushExtQueue(swapQueueIndex);
    }

    // map memory and readback
    byte *pData = NULL;
    vkr = vt->MapMemory(Unwrap(device), Unwrap(readbackMem.mem), readbackMem.offs, readbackMem.size,
                        0, (void **)&pData);
    RDCASSERTEQUAL(vkr, VK_SUCCESS);
    RDCASSERT(pData != NULL);

    fp.len = (uint32_t)readbackMem.size;
    fp.data = new uint8_t[fp.len];
    memcpy(fp.data, pData, fp.len);

    VkMappedMemoryRange range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        NULL,
        Unwrap(readbackMem.mem),
        readbackMem.offs,
        readbackMem.size,
    };

    vkr = vt->InvalidateMappedMemoryRanges(Unwrap(device), 1, &range);
    RDCASSERTEQUAL(vkr, VK_SUCCESS);

    vt->UnmapMemory(Unwrap(device), Unwrap(readbackMem.mem));

    // delete all
    vt->DestroyBuffer(Unwrap(device), Unwrap(readbackBuf), NULL);
    GetResourceManager()->ReleaseWrappedResource(readbackBuf);

    ResourceFormat fmt = MakeResourceFormat(swapInfo.imageInfo.format);
    fp.width = swapInfo.imageInfo.extent.width;
    fp.height = swapInfo.imageInfo.extent.height;
    fp.pitch = rowPitch;
    fp.stride = fmt.compByteWidth * fmt.compCount;
    fp.bpc = fmt.compByteWidth;
    fp.bgra = fmt.BGRAOrder();
    fp.max_width = maxSize;
    fp.pitch_requirement = 8;
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

    // don't need to lock access to m_CmdBufferRecords as we are no longer
    // in capframe (the transition is thread-protected) so nothing will be
    // pushed to the vector

    {
      std::map<int64_t, Chunk *> recordlist;
      RDCDEBUG("Flushing %zu command buffer records to file serialiser", m_CmdBufferRecords.size());
      // ensure all command buffer records within the frame even if recorded before, but
      // otherwise order must be preserved (vs. queue submits and desc set updates)
      for(size_t i = 0; i < m_CmdBufferRecords.size(); i++)
      {
        if(Metal_Debug_VerboseCommandRecording())
        {
          RDCLOG("Adding chunks from command buffer %s",
                 ToStr(m_CmdBufferRecords[i]->GetResourceID()).c_str());
        }
        else
        {
          RDCDEBUG("Adding chunks from command buffer %s",
                   ToStr(m_CmdBufferRecords[i]->GetResourceID()).c_str());
        }

        size_t prevSize = recordlist.size();
        (void)prevSize;

        m_CmdBufferRecords[i]->Insert(recordlist);

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

  m_State = CaptureState::BackgroundCapturing;

  if(Metal_Debug_VerboseCommandRecording())
  {
    RDCLOG("Deleting %zu command buffer records", m_CmdBufferRecords.size());
  }
  // delete cmd buffers now - had to keep them alive until after serialiser flush.
  for(size_t i = 0; i < m_CmdBufferRecords.size(); i++)
    m_CmdBufferRecords[i]->Delete(GetResourceManager());

  m_CmdBufferRecords.clear();
  Atomic::Inc32(&m_ReuseEnabled);

  GetResourceManager()->ResetLastWriteTimes();

  GetResourceManager()->MarkUnwrittenResources();

  // GetResourceManager()->ClearReferencedMemory();

  GetResourceManager()->ClearReferencedResources();

  GetResourceManager()->FreeInitialContents();

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

    // m_SuccessfulCapture = false;

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

  Atomic::Inc32(&m_ReuseEnabled);

  m_HeaderChunk->Delete();
  m_HeaderChunk = NULL;

  if(Metal_Debug_VerboseCommandRecording())
  {
    RDCLOG("Deleting %zu command buffer records", m_CmdBufferRecords.size());
  }

  // delete cmd buffers now - had to keep them alive until after serialiser flush.
  for(size_t i = 0; i < m_CmdBufferRecords.size(); i++)
    m_CmdBufferRecords[i]->Delete(GetResourceManager());

  m_CmdBufferRecords.clear();

  GetResourceManager()->MarkUnwrittenResources();

  GetResourceManager()->ClearReferencedResources();

  GetResourceManager()->FreeInitialContents();

  //  FreeAllMemory(MemoryScope::InitialContents);

  return true;
}

void WrappedMTLDevice::AdvanceFrame()
{
  if(IsBackgroundCapturing(m_State))
    RenderDoc::Inst().Tick();

  m_FrameCounter++;    // first present becomes frame #1, this function is at the end of the frame
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
  m_AddedAction = true;

  ActionDescription action = a;
  /*
  action.eventId = m_LastCmdBufferID != ResourceId()
                       ? m_BakedCmdBufferInfo[m_LastCmdBufferID].curEventID
                       : m_RootEventID;
  action.actionId = m_LastCmdBufferID != ResourceId()
                        ? m_BakedCmdBufferInfo[m_LastCmdBufferID].actionCount
                        : m_RootActionID;
*/
  action.eventId = m_RootEventID;
  action.actionId = m_RootActionID;
  for(uint32_t i = 0; i < 8; i++)
    action.outputs[i] = ResourceId();

  action.depthOut = ResourceId();

  /*
   if(m_LastCmdBufferID != ResourceId())
    {
      ResourceId fb = m_BakedCmdBufferInfo[m_LastCmdBufferID].state.GetFramebuffer();
      ResourceId rp = m_BakedCmdBufferInfo[m_LastCmdBufferID].state.renderPass;
      uint32_t sp = m_BakedCmdBufferInfo[m_LastCmdBufferID].state.subpass;

      if(fb != ResourceId() && rp != ResourceId())
      {
        const rdcarray<ResourceId> &atts =
            m_BakedCmdBufferInfo[m_LastCmdBufferID].state.GetFramebufferAttachments();

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
    /*
        if(m_LastCmdBufferID != ResourceId())
          m_BakedCmdBufferInfo[m_LastCmdBufferID].actionCount++;
        else
    */
    m_RootActionID++;
  }

  /*
     action.events.swap(m_LastCmdBufferID != ResourceId()
                           ? m_BakedCmdBufferInfo[m_LastCmdBufferID].curEvents
                           : m_RootEvents);
  */
  action.events.swap(m_RootEvents);

  // should have at least the root action here, push this action
  // onto the back's children list.
  if(!GetActionStack().empty())
  {
    /*
    ActionDescription node(action);

    node.resourceUsage.swap(m_BakedCmdBufferInfo[m_LastCmdBufferID].resourceUsage);

    if(m_LastCmdBufferID != ResourceId())
      AddUsage(node, m_BakedCmdBufferInfo[m_LastCmdBufferID].debugMessages);
    node.children.reserve(action.children.size());
    for(const ActionDescription &child : action.children)
      node.children.push_back(ActionDescription(child));
    GetActionStack().back()->children.push_back(node);
     */
    GetActionStack().back()->children.push_back(action);
  }
  else
    RDCERR("Somehow lost action stack!");
}

void WrappedMTLDevice::AddEvent()
{
  APIEvent apievent;

  apievent.fileOffset = m_CurChunkOffset;
  /*
  apievent.eventId = m_LastCmdBufferID != ResourceId()
                         ? m_BakedCmdBufferInfo[m_LastCmdBufferID].curEventID
                         : m_RootEventID;
  */
  apievent.eventId = m_RootEventID;

  apievent.chunkIndex = uint32_t(m_StructuredFile->chunks.size() - 1);

  for(size_t i = 0; i < m_EventMessages.size(); i++)
    m_EventMessages[i].eventId = apievent.eventId;

  /*
  if(m_LastCmdBufferID != ResourceId())
  {
    m_BakedCmdBufferInfo[m_LastCmdBufferID].curEvents.push_back(apievent);
    m_BakedCmdBufferInfo[m_LastCmdBufferID].debugMessages.append(m_EventMessages);
  }
  else
   */
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
    /*
    if(chunk == MetalChunk::vkBeginCommandBuffer || chunk == MetalChunk::vkEndCommandBuffer)
    {
      // don't add these events - they will be handled when inserted in-line into queue submit
    }
    else
     */
    {
      if(!m_AddedAction)
        AddEvent();
    }
  }

  m_AddedAction = false;

  return true;
}

bool WrappedMTLDevice::ProcessChunk(ReadSerialiser &ser, MetalChunk chunk)
{
  switch(chunk)
  {
    case MetalChunk::MTLCreateSystemDefaultDevice:
      return Serialise_MTLCreateSystemDefaultDevice(ser, NULL);
    case MetalChunk::MTLBuffer_contents:
      return m_DummyBuffer->Serialise_mtlBuffer_contents(ser, NULL);
    case MetalChunk::MTLDevice_newBuffer:
    case MetalChunk::MTLDevice_newBufferWithLength:
      return Serialise_mtlDevice_newBuffer(ser, NULL, NULL, NULL, NSUInteger_Zero,
                                           MTLResourceOptions_Zero);
    case MetalChunk::MTLDevice_newCommandQueue:
      return Serialise_mtlDevice_newCommandQueue(ser, NULL, NULL);
    case MetalChunk::MTLDevice_newDefaultLibrary:
      return Serialise_mtlDevice_newDefaultLibrary(ser, NULL, NULL, NULL, 0);
    case MetalChunk::MTLDevice_newLibraryWithSource:
      return Serialise_mtlDevice_newLibraryWithSource(ser, NULL, NULL, NULL, NULL);
    case MetalChunk::MTLDevice_newRenderPipelineStateWithDescriptor:
      return Serialise_mtlDevice_newRenderPipelineStateWithDescriptor(ser, NULL, NULL, NULL);
    case MetalChunk::MTLDevice_newTextureWithDescriptor:
      return Serialise_mtlDevice_newTextureWithDescriptor(ser, NULL, NULL, NULL, NULL,
                                                          NSUInteger_Zero);
    case MetalChunk::MTLCommandBuffer_commit:
      return m_DummyReplayCommandBuffer->Serialise_mtlCommandBuffer_commit(ser, NULL);
    case MetalChunk::MTLCommandBuffer_presentDrawable:
      return m_DummyReplayCommandBuffer->Serialise_mtlCommandBuffer_presentDrawable(
          ser, NULL, id_MTLDrawable());
    case MetalChunk::MTLCommandBuffer_renderCommandEncoderWithDescriptor:
      return m_DummyReplayCommandBuffer->Serialise_mtlCommandBuffer_renderCommandEncoderWithDescriptor(
          ser, NULL, NULL, NULL);
    case MetalChunk::MTLCommandQueue_commandBuffer:
      return m_DummyReplayCommandQueue->Serialise_mtlCommandQueue_commandBuffer(ser, NULL, NULL);
    case MetalChunk::MTLLibrary_newFunctionWithName:
      return m_DummyReplayLibrary->Serialise_mtlLibrary_newFunctionWithName(ser, NULL, NULL, NULL);
    case MetalChunk::MTLRenderCommandEncoder_drawPrimitives:
      return m_DummyReplayRenderCommandEncoder->Serialise_mtlRenderCommandEncoder_drawPrimitives(
          ser, NULL, MTLPrimitiveType_Zero, NSUInteger_Zero, NSUInteger_Zero, NSUInteger_Zero);
    case MetalChunk::MTLRenderCommandEncoder_endEncoding:
      return m_DummyReplayRenderCommandEncoder->Serialise_mtlRenderCommandEncoder_endEncoding(ser,
                                                                                              NULL);
    case MetalChunk::MTLRenderCommandEncoder_setFragmentBuffer:
      return m_DummyReplayRenderCommandEncoder->Serialise_mtlRenderCommandEncoder_setFragmentBuffer(
          ser, NULL, NULL, NSUInteger_Zero, NSUInteger_Zero);
    case MetalChunk::MTLRenderCommandEncoder_setFragmentTexture:
      return m_DummyReplayRenderCommandEncoder->Serialise_mtlRenderCommandEncoder_setFragmentTexture(
          ser, NULL, NULL, NSUInteger_Zero);
    case MetalChunk::MTLRenderCommandEncoder_setVertexBuffer:
      return m_DummyReplayRenderCommandEncoder->Serialise_mtlRenderCommandEncoder_setVertexBuffer(
          ser, NULL, NULL, NSUInteger_Zero, NSUInteger_Zero);
    case MetalChunk::MTLRenderCommandEncoder_setRenderPipelineState:
      return m_DummyReplayRenderCommandEncoder->Serialise_mtlRenderCommandEncoder_setRenderPipelineState(
          ser, NULL, NULL);

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
      SERIALISE_ELEMENT_LOCAL(PresentedImage, ResourceId()).TypedAs("MTLImage"_lit);

      SERIALISE_CHECK_READ_ERRORS();

      if(IsLoading(m_State))
      {
        AddEvent();

        ActionDescription action;
        action.customName = "End of Capture";
        action.flags |= ActionFlags::Present;
        action.copyDestination = ResourceId();
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

ReplayStatus WrappedMTLDevice::ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers)
{
  int sectionIdx = rdc->SectionIndex(SectionType::FrameCapture);

  GetResourceManager()->SetState(m_State);

  if(sectionIdx < 0)
    return ReplayStatus::FileCorrupted;

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
    return ReplayStatus::FileIOFailed;
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
      return ReplayStatus::APIDataCorrupted;
    }

    bool success = ProcessChunk(ser, context);

    ser.EndChunk();

    if(reader->IsErrored())
    {
      return ReplayStatus::APIDataCorrupted;
    }

    // if there wasn't a serialisation error, but the chunk didn't succeed, then it's an API replay
    // failure.
    if(!success)
    {
      return m_FailedReplayStatus;
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

      ReplayStatus status = ContextReplayLog(m_State, 0, 0, false);
      if(status != ReplayStatus::Succeeded)
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
    RDCASSERT(real != NULL);
  }

  // TODO: FreeAllMemory(MemoryScope::IndirectReadback);

  return ReplayStatus::Succeeded;
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

  WrappedMTLCommandBuffer *cmd = GetNextCmd();
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

  for(auto it = m_ImageStates.begin(); it != m_ImageStates.end(); ++it)
  {
    if(GetResourceManager()->HasCurrentResource(it->first))
    {
      //      it->second.LockWrite()->ResetToOldState(m_cleanupImageBarriers,
      //      GetImageTransitionInfo());
    }
    else
    {
      it = m_ImageStates.erase(it);
      --it;
    }
  }

  // likewise again to make sure the initial states are all applied
  cmd = GetNextCmd();
  // DoPipelineBarrier(cmd, 1, &memBarrier);

  //  SubmitAndFlushImageStateBarriers(m_setupImageBarriers);
  SubmitCmds();
  FlushQ();
  //  SubmitAndFlushImageStateBarriers(m_cleanupImageBarriers);

  // reset any queries to a valid copy-able state if they need to be copied.
  //  if(!m_ResetQueries.empty())
  //  {
  //    // sort all pools together
  //    std::sort(m_ResetQueries.begin(), m_ResetQueries.end(),
  //              [](const ResetQuery &a, const ResetQuery &b) { return a.pool < b.pool; });
  //
  //    cmd = GetNextCmd();
  //
  //    if(cmd == VK_NULL_HANDLE)
  //      return;
  //
  //    vkr = ObjDisp(cmd)->BeginCommandBuffer(Unwrap(cmd), &beginInfo);
  //    CheckVkResult(vkr);
  //
  //    uint32_t i = 0;
  //    for(const ResetQuery &r : m_ResetQueries)
  //    {
  //      ObjDisp(cmd)->CmdResetQueryPool(Unwrap(cmd), Unwrap(r.pool), r.firstQuery, r.queryCount);
  //
  //      for(uint32_t q = 0; q < r.queryCount; q++)
  //      {
  //        // Timestamps are easy - we can do these without needing to render
  //        if(m_CreationInfo.m_QueryPool[GetResID(r.pool)].queryType == VK_QUERY_TYPE_TIMESTAMP)
  //        {
  //          ObjDisp(cmd)->CmdWriteTimestamp(Unwrap(cmd), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
  //                                          Unwrap(r.pool), r.firstQuery + q);
  //        }
  //        else
  //        {
  //          ObjDisp(cmd)->CmdBeginQuery(Unwrap(cmd), Unwrap(r.pool), r.firstQuery + q, 0);
  //          ObjDisp(cmd)->CmdEndQuery(Unwrap(cmd), Unwrap(r.pool), r.firstQuery + q);
  //        }
  //
  //        i++;
  //
  //        // split the command buffer and flush if the number of queries is massive
  //        if(i > 0 && (i % (128 * 1024)) == 0)
  //        {
  //          vkr = ObjDisp(cmd)->EndCommandBuffer(Unwrap(cmd));
  //          CheckVkResult(vkr);
  //
  //          SubmitCmds();
  //          FlushQ();
  //
  //          cmd = GetNextCmd();
  //
  //          if(cmd == VK_NULL_HANDLE)
  //            return;
  //
  //          vkr = ObjDisp(cmd)->BeginCommandBuffer(Unwrap(cmd), &beginInfo);
  //          CheckVkResult(vkr);
  //        }
  //      }
  //    }

  //  m_ResetQueries.clear();

  SubmitCmds();
  FlushQ();
}

ReplayStatus WrappedMTLDevice::ContextReplayLog(CaptureState readType, uint32_t startEventID,
                                                uint32_t endEventID, bool partial)
{
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

  /*
   // TODO: implement RD MTL replay
   // Need to track each command buffer and wait for all of them to be completed
    if(!IsStructuredExporting(m_State))
      ObjDisp(GetDev())->DeviceWaitIdle(Unwrap(GetDev()));
  */

  // apply initial contents here so that images are in the right layout
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

    MetalChunk chunktype = ser.ReadChunk<MetalChunk>();

    if(ser.GetReader()->IsErrored())
      return ReplayStatus::APIDataCorrupted;

    m_ChunkMetadata = ser.ChunkMetadata();

    m_LastCmdBufferID = ResourceId();

    bool success = ContextProcessChunk(ser, chunktype);

    ser.EndChunk();

    if(ser.GetReader()->IsErrored())
      return ReplayStatus::APIDataCorrupted;

    // if there wasn't a serialisation error, but the chunk didn't succeed, then it's an API replay
    // failure.
    if(!success)
      return m_FailedReplayStatus;

    RenderDoc::Inst().SetProgress(
        LoadProgress::FrameEventsRead,
        float(m_CurChunkOffset - startOffset) / float(ser.GetReader()->GetSize()));

    if((SystemChunk)chunktype == SystemChunk::CaptureEnd || ser.GetReader()->AtEnd())
      break;

    // break out if we were only executing one event
    if(IsActiveReplaying(m_State) && startEventID == endEventID)
      break;

    m_LastChunk = chunktype;

    // increment root event ID either if we didn't just replay a cmd
    // buffer event, OR if we are doing a frame sub-section replay,
    // in which case it's up to the calling code to make sure we only
    // replay inside a command buffer (if we crossed command buffer
    // boundaries, the event IDs would no longer match up).
    if(m_LastCmdBufferID == ResourceId() || startEventID > 1)
    {
      m_RootEventID++;

      if(startEventID > 1)
        ser.GetReader()->SetOffset(GetEvent(m_RootEventID).fileOffset);
    }
    else
    {
      /*
            // these events are completely omitted, so don't increment the curEventID
            if(chunktype != MetalChunk::vkBeginCommandBuffer &&
               chunktype != MetalChunk::vkEndCommandBuffer)
              m_BakedCmdBufferInfo[m_LastCmdBufferID].curEventID++;
      */
      m_RootEventID++;
    }
  }

  // Save the current render state in the partial command buffer.
  // m_RenderState = m_BakedCmdBufferInfo[GetPartialCommandBuffer()].state;

  // swap the structure back now that we've accumulated the frame as well.
  if(IsLoading(m_State) || IsStructuredExporting(m_State))
    ser.GetStructuredFile().Swap(*prevFile);

  m_StructuredFile = prevFile;

  if(IsLoading(m_State))
  {
    GetReplay()->WriteFrameRecord().actionList = m_ParentAction.children;

    SetupActionPointers(m_Actions, GetReplay()->WriteFrameRecord().actionList);

    // m_ParentAction.children.clear();
  }
  /*
  if(!IsStructuredExporting(m_State))
  {
    ObjDisp(GetDev())->DeviceWaitIdle(Unwrap(GetDev()));

    // destroy any events we created for waiting on
    for(size_t i = 0; i < m_CleanupEvents.size(); i++)
      ObjDisp(GetDev())->DestroyEvent(Unwrap(GetDev()), m_CleanupEvents[i], NULL);

    for(const rdcpair<VkCommandPool, VkCommandBuffer> &rerecord : m_RerecordCmdList)
      vkFreeCommandBuffers(GetDev(), rerecord.first, 1, &rerecord.second);
  }
  */

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

  return ReplayStatus::Succeeded;
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
    // VkMarkerRegion::Begin("!!!!RenderDoc Internal: ApplyInitialContents");
    ApplyInitialContents();
    // VkMarkerRegion::End();
  }

  m_State = CaptureState::ActiveReplaying;

  //  VkMarkerRegion::Set(StringFormat::Fmt("!!!!RenderDoc Internal: RenderDoc Replay %d (%d):
  //  %u->%u",
  //                                        (int)replayType, (int)partial, startEventID,
  //                                        endEventID));

  {
    if(!partial)
    {
      m_Partial[Primary].Reset();
      m_Partial[Secondary].Reset();
      m_RenderState = MetalRenderState();
      //      for(auto it = m_BakedCmdBufferInfo.begin(); it != m_BakedCmdBufferInfo.end(); it++)
      //        it->second.state = MetalRenderState();
    }
    else
    {
      // Copy the state in case m_RenderState was modified externally for the partial replay.
      //      m_BakedCmdBufferInfo[GetPartialCommandBuffer()].state = m_RenderState;
    }

    bool rpWasActive = false;

    // we'll need our own command buffer if we're replaying just a subsection
    // of events within a single command buffer record - always if it's only
    // one action, or if start event ID is > 0 we assume the outside code
    // has chosen a subsection that lies within a command buffer
    if(partial)
    {
      WrappedMTLCommandBuffer *cmd = m_OutsideCmdBuffer = GetNextCmd();

      if(cmd == NULL)
        return;

      // we'll explicitly submit this when we're ready
      RemovePendingCommandBuffer(cmd);

      rpWasActive = m_Partial[Primary].renderPassActive;

      if(m_Partial[Primary].renderPassActive)
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

        // if we have an indirect action with one action, the subcommand will have an event which
        // isn't a ActionDescription and selecting it will still replay that indirect action. We
        // need to detect this case and ensure we prepare the RP. This doesn't happen for
        // multi-action indirects because there each subcommand has an actual ActionDescription
        //        if(rpUnneeded)
        //        {
        //          APIEvent ev = GetEvent(endEventID);
        //          if(m_StructuredFile->chunks[ev.chunkIndex]->metadata.chunkID ==
        //             (uint32_t)VulkanChunk::vkCmdIndirectSubCommand)
        //            rpUnneeded = false;
        //        }

        // if a render pass was active, begin it and set up the partial replay state
        m_RenderState.BeginRenderPassAndApplyState(
            this, cmd, rpUnneeded ? MetalRenderState::BindNone : MetalRenderState::BindGraphics);
      }
      else
      {
        // even outside of render passes, we need to restore the state
        m_RenderState.BindPipeline(this, cmd, MetalRenderState::BindInitial, false);
      }
    }

    ReplayStatus status = ReplayStatus::Succeeded;

    if(replayType == eReplay_Full)
      status = ContextReplayLog(m_State, startEventID, endEventID, partial);
    else if(replayType == eReplay_WithoutDraw)
      status = ContextReplayLog(m_State, startEventID, RDCMAX(1U, endEventID) - 1, partial);
    else if(replayType == eReplay_OnlyDraw)
      status = ContextReplayLog(m_State, endEventID, endEventID, partial);
    else
      RDCFATAL("Unexpected replay type");

    RDCASSERTEQUAL(status, ReplayStatus::Succeeded);

    if(m_OutsideCmdBuffer != NULL)
    {
      WrappedMTLCommandBuffer *cmd = m_OutsideCmdBuffer;

      // end any active XFB
      //      if(!m_RenderState.xfbcounters.empty())
      //        m_RenderState.EndTransformFeedback(this, cmd);

      // end any active conditional rendering
      //      if(m_RenderState.IsConditionalRenderingEnabled())
      //        m_RenderState.EndConditionalRendering(cmd);

      // check if the render pass is active - it could have become active
      // even if it wasn't before (if the above event was a CmdBeginRenderEncoder).
      // If we began our own custom single-action loadrp, and it was ended by a CmdEndRenderEncoder,
      // we need to reverse the virtual transitions we did above, as it won't happen otherwise
      if(m_Partial[Primary].renderPassActive)
        m_RenderState.EndRenderPass(cmd);

      // we might have replayed a CmdBeginRenderEncoder or CmdEndRenderEncoder,
      // but we want to keep the partial replay data state intact, so restore
      // whether or not a render pass was active.
      m_Partial[Primary].renderPassActive = rpWasActive;

      AddPendingCommandBuffer(cmd);

      SubmitCmds();

      m_OutsideCmdBuffer = NULL;
    }
  }

  //  VkMarkerRegion::Set("!!!!RenderDoc Internal: Done replay");
}

rdcstr WrappedMTLDevice::GetChunkName(uint32_t idx)
{
  if((SystemChunk)idx == SystemChunk::DriverInit)
    return "MTLCreateInstance"_lit;

  if((SystemChunk)idx < SystemChunk::FirstDriverChunk)
    return ToStr((SystemChunk)idx);

  return ToStr((MetalChunk)idx);
}

void WrappedMTLDevice::AddFrameCaptureRecordChunk(Chunk *chunk)
{
  m_FrameCaptureRecord->AddChunk(chunk);
}

void WrappedMTLDevice::AddCommandBufferRecord(MetalResourceRecord *record)
{
  SCOPED_LOCK(m_CmdBufferRecordsLock);
  m_CmdBufferRecords.push_back(record);
  RDCDEBUG("Adding CommandBufferRecord Count %zu", m_CmdBufferRecords.size());
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
    initStateCurCmd = GetNextCmd();

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

WrappedMTLCommandBuffer *WrappedMTLDevice::GetNextCmd()
{
  WrappedMTLCommandBuffer *cmd = GetWrapped(m_ReplayCommandQueue->commandBuffer());
  // cmd->enqueue();
  AddPendingCommandBuffer(cmd);
  return cmd;
}

void WrappedMTLDevice::RemovePendingCommandBuffer(WrappedMTLCommandBuffer *cmd)
{
  m_InternalCmds.pendingcmds.removeOne(cmd);
}

void WrappedMTLDevice::AddPendingCommandBuffer(WrappedMTLCommandBuffer *cmd)
{
  m_InternalCmds.pendingcmds.push_back(cmd);
}

void WrappedMTLDevice::SubmitCmds()
{
  RENDERDOC_PROFILEFUNCTION();
  if(HasFatalError())
    return;

  // nothing to do
  if(m_InternalCmds.pendingcmds.empty())
    return;

  for(size_t i = 0; i < m_InternalCmds.pendingcmds.size(); i++)
  {
    m_InternalCmds.pendingcmds[i]->commit();
  }

  m_InternalCmds.submittedcmds.append(m_InternalCmds.pendingcmds);
  m_InternalCmds.pendingcmds.clear();
}

void WrappedMTLDevice::FlushQ()
{
  RENDERDOC_PROFILEFUNCTION();

  if(HasFatalError())
    return;

  for(size_t i = 0; i < m_InternalCmds.submittedcmds.size(); i++)
  {
    WrappedMTLCommandBuffer *cmd = m_InternalCmds.submittedcmds[i];
    cmd->waitUntilCompleted();
  }
  if(!m_InternalCmds.submittedcmds.empty())
  {
    m_InternalCmds.submittedcmds.clear();
  }
}

MetalResources::LockedImageStateRef WrappedMTLDevice::FindImageState(ResourceId id)
{
  SCOPED_LOCK(m_ImageStatesLock);
  auto it = m_ImageStates.find(id);
  if(it != m_ImageStates.end())
    return it->second.LockWrite();
  else
    return MetalResources::LockedImageStateRef();
}

MetalResources::LockedConstImageStateRef WrappedMTLDevice::FindConstImageState(ResourceId id)
{
  SCOPED_LOCK(m_ImageStatesLock);
  auto it = m_ImageStates.find(id);
  if(it != m_ImageStates.end())
    return it->second.LockRead();
  else
    return MetalResources::LockedConstImageStateRef();
}

MetalResources::LockedImageStateRef WrappedMTLDevice::InsertImageState(
    id_MTLTexture wrappedHandle, ResourceId id, const MetalResources::ImageInfo &info,
    FrameRefType refType, bool *inserted)
{
  SCOPED_LOCK(m_ImageStatesLock);
  auto it = m_ImageStates.find(id);
  if(it != m_ImageStates.end())
  {
    if(inserted != NULL)
      *inserted = false;
    return it->second.LockWrite();
  }
  else
  {
    if(inserted != NULL)
      *inserted = true;
    it = m_ImageStates.insert({id, MetalResources::LockingImageState(wrappedHandle, info, refType)}).first;
    return it->second.LockWrite();
  }
}

void Metal_ProcessStructured(RDCFile *rdc, SDFile &output)
{
  WrappedMTLDevice wrappedMTLDevice;

  int sectionIdx = rdc->SectionIndex(SectionType::FrameCapture);

  if(sectionIdx < 0)
    return;

  wrappedMTLDevice.SetStructuredExport(rdc->GetSectionProperties(sectionIdx).version);
  ReplayStatus status = wrappedMTLDevice.ReadLogInitialisation(rdc, true);

  if(status == ReplayStatus::Succeeded)
    wrappedMTLDevice.GetStructuredFile()->Swap(output);
}

static StructuredProcessRegistration MetalProcessRegistration(RDCDriver::Metal,
                                                              &Metal_ProcessStructured);
