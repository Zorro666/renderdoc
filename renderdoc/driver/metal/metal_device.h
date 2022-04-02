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

#pragma once

#include "metal_common.h"
#include "metal_core.h"
#include "metal_info.h"
#include "metal_manager.h"
#include "metal_render_state.h"
#include "metal_resources.h"
#include "metal_types.h"

class MetalReplay;

class MetalCapturer;

class WrappedMTLDevice : public WrappedMTLObject
{
  friend class MetalResourceManager;
  friend class MetalReplay;

public:
  WrappedMTLDevice();
  WrappedMTLDevice(MTL::Device *realMTLDevice, ResourceId objId);
  ~WrappedMTLDevice() {}
  template <typename SerialiserType>
  bool Serialise_MTLCreateSystemDefaultDevice(SerialiserType &ser);
  static WrappedMTLDevice *MTLCreateSystemDefaultDevice(MTL::Device *realMTLDevice);

  // Serialised MTLDevice APIs
  DECLARE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLCommandQueue *, newCommandQueue);
  DECLARE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLLibrary *, newDefaultLibrary);
  DECLARE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLLibrary *, newLibraryWithSource,
                                          NS::String *source, MTL::CompileOptions *options,
                                          NS::Error **error);
  DECLARE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLRenderPipelineState *,
                                          newRenderPipelineStateWithDescriptor,
                                          RDMTL::RenderPipelineDescriptor &descriptor,
                                          NS::Error **error);
  WrappedMTLTexture *newTextureWithDescriptor(RDMTL::TextureDescriptor &descriptor,
                                              IOSurfaceRef iosurface, NS::UInteger plane);
  DECLARE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLTexture *, newTextureWithDescriptor,
                                          RDMTL::TextureDescriptor &descriptor);
  WrappedMTLBuffer *newBufferWithLength(NS::UInteger length, MTL::ResourceOptions options);
  DECLARE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLBuffer *, newBufferWithBytes, const void *pointer,
                                          NS::UInteger length, MTL::ResourceOptions options);

  // Non-Serialised MTLDevice APIs
  bool isDepth24Stencil8PixelFormatSupported();
  MTL::ReadWriteTextureTier readWriteTextureSupport();
  MTL::ArgumentBuffersTier argumentBuffersSupport();
  bool areRasterOrderGroupsSupported();
  bool supports32BitFloatFiltering();
  bool supports32BitMSAA();
  bool supportsQueryTextureLOD();
  bool supportsBCTextureCompression();
  bool supportsPullModelInterpolation();
  bool areBarycentricCoordsSupported();
  bool supportsShaderBarycentricCoordinates();
  bool supportsFeatureSet(MTL::FeatureSet featureSet);
  bool supportsFamily(MTL::GPUFamily gpuFamily);
  bool supportsTextureSampleCount(NS::UInteger sampleCount);
  bool areProgrammableSamplePositionsSupported();
  bool supportsRasterizationRateMapWithLayerCount(NS::UInteger layerCount);
  bool supportsCounterSampling(MTL::CounterSamplingPoint samplingPoint);
  bool supportsVertexAmplificationCount(NS::UInteger count);
  bool supportsDynamicLibraries();
  bool supportsRenderDynamicLibraries();
  bool supportsRaytracing();
  bool supportsFunctionPointers();
  bool supportsFunctionPointersFromRender();
  bool supportsRaytracingFromRender();
  bool supportsPrimitiveMotionBlur();
  // End of MTLDevice APIs

  CaptureState &GetStateRef() { return m_State; }
  CaptureState GetState() { return m_State; }
  MetalResourceManager *GetResourceManager() { return m_ResourceManager; };
  WriteSerialiser &GetThreadSerialiser();

  // IFrameCapturer interface
  RDCDriver GetFrameCaptureDriver() { return RDCDriver::Metal; }
  void StartFrameCapture(void *dev, void *wnd);
  bool EndFrameCapture(void *dev, void *wnd);
  bool DiscardFrameCapture(void *dev, void *wnd);
  // IFrameCapturer interface

  void AdvanceFrame();
  void Present(WrappedMTLTexture *backBuffer, CA::MetalLayer *outputLayer);

  void AddCommandBufferRecord(MetalResourceRecord *record);

  RDResult Initialise(MetalInitParams &params, uint64_t sectionVersion, const ReplayOptions &opts);
  uint64_t GetLogVersion() { return m_SectionVersion; }
  void SetStructuredExport(uint64_t sectionVersion)
  {
    m_SectionVersion = sectionVersion;
    m_State = CaptureState::StructuredExport;
  }

  RDResult ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers);
  SDFile *GetStructuredFile() { return m_StructuredFile; }
  MetalReplay *GetReplay() { return m_Replay; }
  void AddFrameCaptureRecordChunk(Chunk *chunk) { m_FrameCaptureRecord->AddChunk(chunk); }
  void AddResource(ResourceId id, ResourceType type, const char *defaultNamePrefix);
  void DerivedResource(ResourceId parentLive, ResourceId child);
  template <typename MetalType>
  void DerivedResource(MetalType parent, ResourceId child)
  {
    DerivedResource(GetResID(parent), child);
  }

  void AddEvent();
  void AddAction(const ActionDescription &a);
  const ActionDescription *GetAction(uint32_t eventId);

  MetalRenderState &GetCmdRenderState()
  {
    //    RDCASSERT(m_LastCmdBufferID != ResourceId());
    // auto it = m_BakedCmdBufferInfo.find(m_LastCmdBufferID);
    // RDCASSERT(it != m_BakedCmdBufferInfo.end());
    return m_RenderState;
  }

  enum PartialReplayIndex
  {
    Primary,
    Secondary,
    ePartialNum
  };

  void SetActiveRenderCommandEncoder(WrappedMTLRenderCommandEncoder *renderCommandEncoder)
  {
    m_ActiveRenderCommandEncoder = renderCommandEncoder;
  }

  void SetRenderPassActiveState(PartialReplayIndex index, bool state)
  {
    m_Partial[index].renderPassActive = state;
  }

  WrappedMTLCommandBuffer *GetNextCommandBuffer();
  void RemovePendingCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer);

  MetalCreationInfo &GetCreationInfo() { return m_CreationInfo; }
  rdcarray<EventUsage> GetUsage(ResourceId id) { return m_ResourceUses[id]; }
  enum
  {
    TypeEnum = eResDevice
  };

  static uint64_t g_nextDrawableTLSSlot;
  static IMP g_real_CAMetalLayer_nextDrawable;

private:
  static void MTLFixupForMetalDriverAssert();
  static void MTLHookObjcMethods();
  void FirstFrame();

  void Construct();
  void CreateInstance();

  RDResult FatalErrorCheck() { return m_FatalError; }
  bool HasFatalError() { return m_FatalError != ResultCode::Succeeded; }
  rdcarray<ActionDescription *> &GetActionStack()
  {
    /*
        if(m_LastCmdBufferID != ResourceId())
          return m_BakedCmdBufferInfo[m_LastCmdBufferID].actionStack;
    */
    return m_ActionStack;
  }

  const APIEvent &GetEvent(uint32_t eventId);
  template <typename SerialiserType>
  bool Serialise_CaptureScope(SerialiserType &ser);
  template <typename SerialiserType>
  bool Serialise_BeginCaptureFrame(SerialiserType &ser);
  void ApplyInitialContents();
  bool ProcessChunk(ReadSerialiser &ser, MetalChunk chunk);
  bool ContextProcessChunk(ReadSerialiser &ser, MetalChunk chunk);
  bool HasSuccessfulCapture();
  void EndCaptureFrame(ResourceId backbuffer);
  static rdcstr GetChunkName(uint32_t idx);
  RDResult ContextReplayLog(CaptureState readType, uint32_t startEventID, uint32_t endEventID,
                            bool partial);
  void ReplayLog(uint32_t startEventID, uint32_t endEventID, ReplayLogType replayType);

  bool Prepare_InitialState(WrappedMTLObject *res);
  uint64_t GetSize_InitialState(ResourceId id, const MetalInitialContents &initial);
  template <typename SerialiserType>
  bool Serialise_InitialState(SerialiserType &ser, ResourceId id, MetalResourceRecord *record,
                              const MetalInitialContents *initial);
  void Create_InitialState(ResourceId id, WrappedMTLObject *live, bool hasData);
  void Apply_InitialState(WrappedMTLObject *live, const MetalInitialContents &initial);

  void AddResourceCurChunk(ResourceDescription &descr);
  void AddResourceCurChunk(ResourceId id);

  CA::MetalDrawable *GetNextDrawable(void *layer);
  WrappedMTLCommandBuffer *GetInitStateCmd();
  void CloseInitStateCmd();
  void AddPendingCommandBuffer(WrappedMTLCommandBuffer *cmd);
  void SubmitCmds();
  void FlushQ();
  void WaitForGPU();

  MetalLockedTextureStateRef FindTextureState(ResourceId id);
  MetalLockedConstTextureStateRef FindConstTextureState(ResourceId id);
  MetalLockedTextureStateRef InsertTextureState(WrappedMTLTexture *wrappedHandle, ResourceId id,
                                                const MetalTextureInfo &info, FrameRefType refType,
                                                bool *inserted);

  WrappedMTLTexture *Common_NewTexture(RDMTL::TextureDescriptor &descriptor, MetalChunk chunkType,
                                       bool ioSurfaceTexture, IOSurfaceRef iosurface,
                                       NS::UInteger plane);
  WrappedMTLBuffer *Common_NewBuffer(bool withBytes, const void *pointer, NS::UInteger length,
                                     MTL::ResourceOptions options);

  MetalResourceManager *m_ResourceManager = NULL;
  ResourceId m_LastPresentedImage;

  std::map<ResourceId, rdcarray<EventUsage>> m_ResourceUses;
  rdcarray<DebugMessage> m_EventMessages;

  // list of all debug messages by EID in the frame
  rdcarray<DebugMessage> m_DebugMessages;

  MetalReplay *m_Replay = NULL;
  ReplayOptions m_ReplayOptions;

  MetalRenderState m_RenderState;

  struct
  {
    void Reset()
    {
      pendingcmds.clear();
      submittedcmds.clear();
    }

    // GetNextCommandBuffer() ->
    rdcarray<WrappedMTLCommandBuffer *> pendingcmds;
    // -> SubmitCmds() ->
    rdcarray<WrappedMTLCommandBuffer *> submittedcmds;
    // -> FlushQ()

  } m_InternalCmds;

  static const int initialStateMaxBatch = 100;
  int initStateCurBatch = 0;
  WrappedMTLCommandBuffer *initStateCurCmd = NULL;

  struct Submission
  {
    Submission(uint32_t eid) : baseEvent(eid), rebased(false) {}
    uint32_t baseEvent = 0;
    bool rebased = false;
  };

  // by definition, when replaying we must have N completely submitted command buffers, and at most
  // two partially-submitted command buffers. One primary, that we're part-way through, and then
  // if we're part-way through a vkCmdExecuteCommandBuffers inside that primary then there's one
  // secondary.
  struct PartialReplayData
  {
    PartialReplayData() { Reset(); }
    void Reset()
    {
      partialParent = ResourceId();
      baseEvent = 0;
      renderPassActive = false;
    }

    // this records where in the frame a command buffer was submitted, so that we know if our replay
    // range ends in one of these ranges we need to construct a partial command buffer for future
    // replaying. Note that we always have the complete command buffer around - it's the bakeID
    // itself.
    // Since we only ever record a bakeID once the key is unique - note that the same command buffer
    // could be recorded multiple times a frame, so the parent command buffer ID (the one recorded
    // in vkCmd chunks) is NOT unique.
    // However, a single baked command list can be submitted multiple times - so we have to have a
    // list of base events
    // Note in the case of secondary command buffers we mark when these are rebased to 'absolute'
    // event IDs, since they could be submitted multiple times in the frame and we don't want to
    // rebase all of them each time.
    // Map from bakeID -> vector<Submission>
    std::map<ResourceId, rdcarray<Submission>> cmdBufferSubmits;

    // identifies the baked ID of the command buffer that's actually partial at each level.
    ResourceId partialParent;

    // the base even of the submission that's partial, as defined above in partialParent
    uint32_t baseEvent;

    // whether a renderpass is currently active in the partial recording - as with baseEvent, only
    // valid for the command buffer referred to by partialParent.
    bool renderPassActive;
  } m_Partial[ePartialNum];

  // if we're replaying just a single action or a particular command
  // buffer subsection of command events, we don't go through the
  // whole original command buffers to set up the partial replay,
  // so we just set this command buffer
  WrappedMTLCommandBuffer *m_OutsideCmdBuffer = NULL;
  WrappedMTLRenderCommandEncoder *m_ActiveRenderCommandEncoder = NULL;

  // used both on capture and replay side to track texture state. Only locked
  // in capture
  std::map<ResourceId, MetalLockingTextureState> m_TextureStates;
  Threading::CriticalSection m_TextureStatesLock;

  rdcarray<APIEvent> m_RootEvents, m_Events;
  bool m_AddedAction = false;
  ActionDescription m_ParentAction;
  rdcarray<ActionDescription *> m_ActionStack;
  rdcarray<ActionDescription *> m_Actions;

  uint64_t m_CurChunkOffset = 0;
  SDChunkMetaData m_ChunkMetadata;
  uint32_t m_RootEventID = 1;
  uint32_t m_RootActionID = 1;
  uint32_t m_FirstEventID = 0;
  uint32_t m_LastEventID = ~0U;
  MetalChunk m_LastChunk;
  // on replay, the current command buffer for the last chunk we
  // handled.
  ResourceId m_LastCmdBufferID;

  // Dummy objects used for serialisation replay
  WrappedMTLBuffer *m_DummyBuffer = NULL;
  WrappedMTLCommandBuffer *m_DummyReplayCommandBuffer = NULL;
  WrappedMTLCommandQueue *m_DummyReplayCommandQueue = NULL;
  WrappedMTLLibrary *m_DummyReplayLibrary = NULL;
  WrappedMTLRenderCommandEncoder *m_DummyReplayRenderCommandEncoder = NULL;

  // Active object used for replaying
  WrappedMTLCommandQueue *m_ReplayCommandQueue = NULL;

  // Helper Metal objects
  MTL::CommandQueue *m_mtlCommandQueue = NULL;

  // record the command buffer records so we can insert them
  // individually, that means even if they were recorded locklessly
  // in parallel, on replay they are disjoint and it makes things
  // much easier to process (queue submit order will enforce/display
  // ordering, record order is not important)
  Threading::CriticalSection m_CommandBufferRecordsLock;
  rdcarray<MetalResourceRecord *> m_CommandBufferRecords;

  // Back buffer and swap chain emulation
  Threading::CriticalSection m_PotentialBackBuffersLock;
  std::unordered_set<WrappedMTLTexture *> m_PotentialBackBuffers;
  Threading::CriticalSection m_OutputLayersLock;
  std::unordered_set<CA::MetalLayer *> m_OutputLayers;
  WrappedMTLTexture *m_CapturedBackbuffer = NULL;

  MetalCapturer *m_Capturer = NULL;
  CaptureState m_State;
  bool m_AppControlledCapture = false;
  RDResult m_FailedReplayResult = ResultCode::APIUnsupported;
  RDResult m_FatalError = ResultCode::Succeeded;

  PerformanceTimer m_CaptureTimer;
  uint32_t m_FrameCounter = 0;
  rdcarray<FrameDescription> m_CapturedFrames;
  Threading::RWLock m_CapTransitionLock;
  MetalResourceRecord *m_FrameCaptureRecord = NULL;
  Chunk *m_HeaderChunk = NULL;

  MetalCreationInfo m_CreationInfo;
  MetalInitParams m_InitParams;
  uint64_t m_SectionVersion = 0;

  std::set<rdcstr> m_StringDB;
  uint64_t m_TimeBase = 0;
  double m_TimeFrequency = 1.0f;
  SDFile *m_StructuredFile = NULL;
  SDFile m_StoredStructuredData;
  StreamReader *m_FrameReader = NULL;

  bool m_MarkedActive = false;

  uint64_t threadSerialiserTLSSlot;
  Threading::CriticalSection m_ThreadSerialisersLock;
  rdcarray<WriteSerialiser *> m_ThreadSerialisers;
};

class MetalCapturer : public IFrameCapturer
{
public:
  MetalCapturer(WrappedMTLDevice *device) : m_Device(device) {}
  // IFrameCapturer interface
  RDCDriver GetFrameCaptureDriver() { return RDCDriver::Metal; }
  void StartFrameCapture(void *dev, void *wnd) { return m_Device->StartFrameCapture(dev, wnd); }
  bool EndFrameCapture(void *dev, void *wnd) { return m_Device->EndFrameCapture(dev, wnd); };
  bool DiscardFrameCapture(void *dev, void *wnd)
  {
    return m_Device->DiscardFrameCapture(dev, wnd);
  };
  // IFrameCapturer interface

private:
  WrappedMTLDevice *m_Device = NULL;
};
