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
#include "core/settings.h"
#include "serialise/rdcfile.h"

#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_device.h"
#include "metal_helpers_bridge.h"
#include "metal_library.h"
#include "metal_render_command_encoder.h"
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

void WrappedMTLDevice::WaitForGPU()
{
  MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
  mtlCommandBuffer->commit();
  mtlCommandBuffer->waitUntilCompleted();
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_BeginCaptureFrame(SerialiserType &ser)
{
  // TODO: serialise image references and states

  SERIALISE_CHECK_READ_ERRORS();

  return true;
}

void WrappedMTLDevice::StartFrameCapture(void *dev, void *wnd)
{
  if(!IsBackgroundCapturing(m_State))
    return;

  RDCLOG("Starting capture");

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
    // TODO: sync all active command buffers
    GetResourceManager()->PrepareInitialContents();

    RDCDEBUG("Attempting capture");
    m_FrameCaptureRecord->DeleteChunks();
    // TODO: handle image states
    m_State = CaptureState::ActiveCapturing;
  }

  GetResourceManager()->MarkResourceFrameReferenced(GetResID(this), eFrameRef_Read);
  // TODO: do resources referenced by the command queues & buffers need to be marked

  // TODO: is there any other type of resource that needs to be marked as frame referenced
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

  // atomically transition back to IDLE
  {
    SCOPED_WRITELOCK(m_CapTransitionLock);
    EndCaptureFrame(bbId);

    m_State = CaptureState::BackgroundCapturing;
  }

  // wait for the GPU to be idle
  for(size_t i = 0; i < m_CommandBufferRecords.size(); i++)
  {
    WrappedMTLCommandBuffer *commandBuffer =
        (WrappedMTLCommandBuffer *)m_CommandBufferRecords[i]->m_Resource;
    Unwrap(commandBuffer)->waitUntilCompleted();
  }

  if(m_CommandBufferRecords.isEmpty())
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
      RDCDEBUG("Flushing %zu command buffer records to file serialiser",
               m_CommandBufferRecords.size());
      // ensure all command buffer records within the frame even if recorded before, but
      // otherwise order must be preserved (vs. queue submits and desc set updates)
      for(size_t i = 0; i < m_CommandBufferRecords.size(); i++)
      {
        RDCDEBUG("Adding chunks from command buffer %s",
                 ToStr(m_CommandBufferRecords[i]->GetResourceID()).c_str());

        size_t prevSize = recordlist.size();
        (void)prevSize;

        m_CommandBufferRecords[i]->Insert(recordlist);

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

  // delete cmd buffers now - had to keep them alive until after serialiser flush.
  for(size_t i = 0; i < m_CommandBufferRecords.size(); i++)
    m_CommandBufferRecords[i]->Delete(GetResourceManager());

  m_CommandBufferRecords.clear();

  GetResourceManager()->ResetLastWriteTimes();

  GetResourceManager()->MarkUnwrittenResources();

  // TODO: handle memory resources in the resource manager

  GetResourceManager()->ClearReferencedResources();

  GetResourceManager()->FreeInitialContents();

  // TODO: handle memory resources in the initial contents
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
  for(size_t i = 0; i < m_CommandBufferRecords.size(); i++)
    m_CommandBufferRecords[i]->Delete(GetResourceManager());

  m_CommandBufferRecords.clear();

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
    // TODO: implement RD MTL replay
  }
  return true;
}

void WrappedMTLDevice::AddCommandBufferRecord(MetalResourceRecord *record)
{
  SCOPED_LOCK(m_CommandBufferRecordsLock);
  m_CommandBufferRecords.push_back(record);
  RDCDEBUG("Adding CommandBufferRecord Count %zu", m_CommandBufferRecords.size());
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

void WrappedMTLDevice::Present(WrappedMTLTexture *backBuffer, CA::MetalLayer *outputLayer)
{
  {
    SCOPED_LOCK(m_PotentialBackBuffersLock);
    if(m_PotentialBackBuffers.count(backBuffer) == 0)
    {
      RDCERR("Capture ignoring Present called on unknown backbuffer");
      return;
    }
  }

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
    RDCASSERT(m_CapturedBackbuffer == NULL);
    m_CapturedBackbuffer = backBuffer;
    RenderDoc::Inst().EndFrameCapture(this, outputLayer);
  }

  if(RenderDoc::Inst().ShouldTriggerCapture(m_FrameCounter) && IsBackgroundCapturing(m_State))
  {
    RenderDoc::Inst().StartFrameCapture(this, outputLayer);

    m_AppControlledCapture = false;
    m_CapturedFrames.back().frameNumber = m_FrameCounter;
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

INSTANTIATE_SERIALISE_TYPE(MetalInitParams);
