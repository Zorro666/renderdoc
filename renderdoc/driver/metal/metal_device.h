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

#pragma once

#include "metal_common.h"
#include "metal_core.h"
#include "metal_resources.h"

class WrappedMTLDevice : public WrappedMTLObject, public IFrameCapturer
{
public:
  WrappedMTLDevice();
  WrappedMTLDevice(id_MTLDevice realMTLDevice, ResourceId objId);
  virtual ~WrappedMTLDevice() {}
  static id_MTLDevice MTLCreateSystemDefaultDevice(id_MTLDevice realMTLDevice);

  DECLARE_WRAPPED_API(id_MTLLibrary, newDefaultLibrary);
  DECLARE_WRAPPED_API(id_MTLBuffer, newBufferWithBytes, const void *pointer, unsigned int length,
                      MTLResourceOptions options);
  DECLARE_WRAPPED_API(id_MTLRenderPipelineState, newRenderPipelineStateWithDescriptor,
                      MTLRenderPipelineDescriptor *descriptor, NSError **error);
  DECLARE_WRAPPED_API(id_MTLCommandQueue, newCommandQueue);

  id_MTLDevice GetObjCWrappedMTLDevice() { return m_ObjCWrappedMTLDevice; };
  RDCDriver GetFrameCaptureDriver() { return RDCDriver::Metal; }
  CaptureState GetState() { return m_State; }
  CaptureState &GetStateRef() { return m_State; }
  void StartFrameCapture(void *dev, void *wnd);
  bool EndFrameCapture(void *dev, void *wnd);
  bool DiscardFrameCapture(void *dev, void *wnd);

  // TODO: implement RD MTL replay
  // ReplayStatus Initialise(VkInitParams &params, uint64_t sectionVersion, const
  // ReplayOptions &opts);
  uint64_t GetLogVersion() { return m_SectionVersion; }
  void SetStructuredExport(uint64_t sectionVersion)
  {
    m_SectionVersion = sectionVersion;
    m_State = CaptureState::StructuredExport;
  }

  ReplayStatus ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers);
  SDFile *GetStructuredFile() { return m_StructuredFile; }
  void AdvanceFrame();
  void Present(void *wnd);
  MetalReplay *GetReplay() { return m_Replay; }
  MetalResourceManager *GetResourceManager() { return m_ResourceManager; };
  WriteSerialiser &GetThreadSerialiser();
  void AddFrameCaptureRecordChunk(Chunk *chunk);
  void AddCommandBufferRecord(MetalResourceRecord *record);

  DECLARE_FUNCTION_SERIALISED(mtlDevice_newDefaultLibrary, WrappedMTLDevice *device,
                              WrappedMTLLibrary *library);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newCommandQueue, WrappedMTLDevice *device,
                              WrappedMTLCommandQueue *queue);
  DECLARE_FUNCTION_SERIALISED(MTLCreateSystemDefaultDevice, WrappedMTLDevice *device);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newRenderPipelineStateWithDescriptor,
                              WrappedMTLDevice *device, WrappedMTLRenderPipelineState *pipelineState);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newBufferWithBytes, WrappedMTLDevice *device,
                              WrappedMTLBuffer *buffer);

  enum
  {
    TypeEnum = eResDevice
  };

private:
  void Construct();
  id_MTLDevice CreateInstance();
  id_MTLFunction GetVertexFunction(MTLRenderPipelineDescriptor *descriptor);
  id_MTLFunction GetFragmentFunction(MTLRenderPipelineDescriptor *descriptor);

  void FirstFrame();

  void AddEvent();
  const APIEvent &GetEvent(uint32_t eventId);
  template <typename SerialiserType>
  bool Serialise_CaptureScope(SerialiserType &ser);
  template <typename SerialiserType>
  bool Serialise_BeginCaptureFrame(SerialiserType &ser);
  bool ProcessChunk(ReadSerialiser &ser, MetalChunk chunk);
  bool ContextProcessChunk(ReadSerialiser &ser, MetalChunk chunk);
  bool HasSuccessfulCapture();
  void EndCaptureFrame();
  static rdcstr GetChunkName(uint32_t idx);
  ReplayStatus ContextReplayLog(CaptureState readType, uint32_t startEventID, uint32_t endEventID,
                                bool partial);

  rdcarray<DebugMessage> m_EventMessages;

  // list of all debug messages by EID in the frame
  rdcarray<DebugMessage> m_DebugMessages;

  id_MTLDevice CreateObjCWrappedMTLDevice();
  id_MTLDevice m_ObjCWrappedMTLDevice;
  MetalResourceManager *m_ResourceManager;

  MetalReplay *m_Replay;
  rdcarray<APIEvent> m_RootEvents, m_Events;
  bool m_AddedAction;

  uint64_t m_CurChunkOffset;
  SDChunkMetaData m_ChunkMetadata;
  uint32_t m_RootEventID, m_RootActionID;
  uint32_t m_FirstEventID, m_LastEventID;
  MetalChunk m_LastChunk;
  // on replay, the current command buffer for the last chunk we
  // handled.
  ResourceId m_LastCmdBufferID;
  WrappedMTLCommandBuffer *m_DummyReplayCommandBuffer;
  WrappedMTLCommandQueue *m_DummyReplayCommandQueue;
  WrappedMTLRenderCommandEncoder *m_DummyReplayRenderCommandEncoder;
  WrappedMTLLibrary *m_DummyReplayLibrary;

  // we record the command buffer records so we can insert them
  // individually, that means even if they were recorded locklessly
  // in parallel, on replay they are disjoint and it makes things
  // much easier to process (we will enforce/display ordering
  // by queue submit order anyway, so it's OK to lose the record
  // order).
  Threading::CriticalSection m_CmdBufferRecordsLock;
  rdcarray<MetalResourceRecord *> m_CmdBufferRecords;

  ReplayStatus m_FailedReplayStatus = ReplayStatus::APIUnsupported;

  CaptureState m_State;
  bool m_AppControlledCapture = false;
  int32_t m_ReuseEnabled = 1;
  PerformanceTimer m_CaptureTimer;
  uint32_t m_FrameCounter = 0;
  rdcarray<FrameDescription> m_CapturedFrames;
  Threading::RWLock m_CapTransitionLock;

  MetalResourceRecord *m_FrameCaptureRecord;
  Chunk *m_HeaderChunk;

  MetalInitParams m_InitParams;

  std::set<rdcstr> m_StringDB;
  uint64_t m_SectionVersion;
  uint64_t m_TimeBase = 0;
  double m_TimeFrequency = 1.0f;
  SDFile *m_StructuredFile;
  SDFile m_StoredStructuredData;
  StreamReader *m_FrameReader = NULL;

  bool m_MarkedActive = false;
  uint32_t m_SubmitCounter = 0;

  uint64_t threadSerialiserTLSSlot;
  Threading::CriticalSection m_ThreadSerialisersLock;
  rdcarray<WriteSerialiser *> m_ThreadSerialisers;
};
