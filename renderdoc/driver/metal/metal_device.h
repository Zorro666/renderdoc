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

#pragma once

#include "metal_common.h"
#include "metal_core.h"
#include "metal_info.h"
#include "metal_manager.h"
#include "metal_render_state.h"
#include "metal_resources.h"
#include "metal_types.h"

class WrappedMTLDevice;
class MetalReplay;
class MetalDebugManager;

typedef std::map<ResourceId, MetalTextureState> MetalTextureStates;

struct MetalDrawableInfo
{
  CA::MetalLayer *mtlLayer;
  WrappedMTLTexture *texture;
  NS::UInteger drawableID;
};

class MetalCapturer : public IFrameCapturer
{
public:
  MetalCapturer(WrappedMTLDevice &device) : m_Device(device) {}
  // IFrameCapturer interface
  RDCDriver GetFrameCaptureDriver() { return RDCDriver::Metal; }
  void StartFrameCapture(DeviceOwnedWindow devWnd);
  bool EndFrameCapture(DeviceOwnedWindow devWnd);
  bool DiscardFrameCapture(DeviceOwnedWindow devWnd);
  // IFrameCapturer interface

private:
  WrappedMTLDevice &m_Device;
};

class WrappedMTLDevice : public WrappedMTLObject
{
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
  bool shouldMaximizeConcurrentCompilation();
  NS::UInteger maximumConcurrentCompilationTaskCount();
  // End of MTLDevice APIs

  CaptureState &GetStateRef() { return m_State; }
  CaptureState GetState() { return m_State; }
  MetalResourceManager *GetResourceManager() { return m_ResourceManager; };
  void WaitForGPU();
  MetalDebugManager *GetDebugManager() { return m_DebugManager; };
  WriteSerialiser &GetThreadSerialiser();

  // IFrameCapturer interface
  RDCDriver GetFrameCaptureDriver() { return RDCDriver::Metal; }
  void StartFrameCapture(DeviceOwnedWindow devWnd);
  bool EndFrameCapture(DeviceOwnedWindow devWnd);
  bool DiscardFrameCapture(DeviceOwnedWindow devWnd);
  // IFrameCapturer interface

  void CaptureCmdBufCommit(MetalResourceRecord *cbRecord);
  void CaptureCmdBufEnqueue(MetalResourceRecord *cbRecord);

  void AddFrameCaptureRecordChunk(Chunk *chunk) { m_FrameCaptureRecord->AddChunk(chunk); }
  // From ResourceManager interface
  bool Prepare_InitialState(WrappedMTLObject *res);
  uint64_t GetSize_InitialState(ResourceId id, const MetalInitialContents &initial);
  template <typename SerialiserType>
  bool Serialise_InitialState(SerialiserType &ser, ResourceId id, MetalResourceRecord *record,
                              const MetalInitialContents *initial);
  void Create_InitialState(ResourceId id, WrappedMTLObject *live, bool hasData);
  void Apply_InitialState(WrappedMTLObject *live, const MetalInitialContents &initial);
  // From ResourceManager interface

  void RegisterMetalLayer(CA::MetalLayer *mtlLayer);
  void UnregisterMetalLayer(CA::MetalLayer *mtlLayer);

  void RegisterDrawableInfo(CA::MetalDrawable *caMtlDrawable);
  MetalDrawableInfo UnregisterDrawableInfo(MTL::Drawable *mtlDrawable);

  RDResult Initialise(MetalInitParams &params, uint64_t sectionVersion, const ReplayOptions &opts);
  uint64_t GetLogVersion() { return m_SectionVersion; }
  void SetStructuredExport(uint64_t sectionVersion)
  {
    m_SectionVersion = sectionVersion;
    m_State = CaptureState::StructuredExport;
  }

  RDResult ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers);
  SDFile *GetStructuredFile() { return m_StructuredFile; }
  bool IsPartialReplay() { return m_ReplayPartialCmdBufferID != ResourceId(); }
  bool IsCurrentCommandBufferEventInReplayRange();
  WrappedMTLCommandBuffer *GetCurrentReplayCommandBuffer();
  void NewReplayCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer);
  void ReplayCommandBufferCommit(WrappedMTLCommandBuffer *cmdBuffer);
  void ReplayCommandBufferEnqueue(WrappedMTLCommandBuffer *cmdBuffer);
  void ResetReplayCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer);
  void SetCurrentCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer);
  void SetCurrentCommandBufferRenderPassDescriptor(const RDMTL::RenderPassDescriptor &rpDesc);
  rdcstr MakeRenderPassOpString(bool startPass);

  void AddEvent();
  void AddAction(const ActionDescription &a);
  const ActionDescription *GetAction(uint32_t eventId);

  MetalReplay *GetReplay() { return m_Replay; }
  void AddResource(ResourceId id, ResourceType type, const char *defaultNamePrefix);
  void DerivedResource(ResourceId parentLive, ResourceId child);
  template <typename MetalType>
  void DerivedResource(MetalType parent, ResourceId child)
  {
    DerivedResource(GetResID(parent), child);
  }

  void SetLastPresentedIamge(ResourceId lastPresentedImage)
  {
    m_LastPresentedImage = lastPresentedImage;
  }

  MetalRenderState &GetCmdRenderState()
  {
    if(m_ReplayPartialCmdBufferID != ResourceId())
      return m_RenderState;

    RDCASSERT(m_ReplayCurrentCmdBufferID != ResourceId());
    auto it = m_ReplayCmdBufferInfos.find(m_ReplayCurrentCmdBufferID);
    RDCASSERT(it != m_ReplayCmdBufferInfos.end());
    return it->second.renderState;
  }

  void ClearActiveRenderCommandEncoder();
  void SetActiveRenderCommandEncoder(WrappedMTLRenderCommandEncoder *renderCommandEncoder);
  WrappedMTLRenderCommandEncoder *GetCurrentReplayRenderEncoder();

  void ClearActiveBlitCommandEncoder();
  void SetActiveBlitCommandEncoder(WrappedMTLBlitCommandEncoder *blitCommandEncoder);
  WrappedMTLBlitCommandEncoder *GetCurrentReplayBlitEncoder();

  WrappedMTLCommandBuffer *GetNextCommandBuffer();
  void RemovePendingCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer);

  MetalCreationInfo &GetCreationInfo() { return m_CreationInfo; }
  rdcarray<EventUsage> GetUsage(ResourceId id) { return m_ResourceUses[id]; }
  MetalTextureStates &GetTextureStates() { return m_TextureStates; }
  MetalTextureState *FindTextureState(ResourceId id);
  const MetalTextureState *FindConstTextureState(ResourceId id);

  MetalRenderState &GetRenderState() { return m_RenderState; }
  void ReplayLog(uint32_t startEventID, uint32_t endEventID, ReplayLogType replayType);

  void SubmitCmds();
  void FlushQ();

  CA::MetalDrawable *GetNextDrawable(CA::MetalLayer *layer);

  RDResult FatalErrorCheck() { return m_FatalError; }
  enum
  {
    TypeEnum = eResDevice
  };

  static uint64_t g_nextDrawableTLSSlot;
  static IMP g_real_CAMetalLayer_nextDrawable;

private:
  struct ReplayCmdBufferInfo;
  static void MTLFixupForMetalDriverAssert();
  static void MTLHookObjcMethods();
  void FirstFrame();
  void AdvanceFrame();
  void Present(MetalResourceRecord *record);

  void CaptureClearSubmittedCmdBuffers();
  void CaptureCmdBufSubmit(MetalResourceRecord *record);
  void EndCaptureFrame(ResourceId backbuffer);

  void Construct();
  void CreateInstance();

  bool HasFatalError() { return m_FatalError != ResultCode::Succeeded; }
  rdcarray<MetalActionTreeNode *> &GetActionStack()
  {
    if(m_ReplayCurrentCmdBufferID != ResourceId())
      return m_ReplayCmdBufferInfos[m_ReplayCurrentCmdBufferID].actionStack;
    return m_ActionStack;
  }
  const APIEvent &GetEvent(uint32_t eventId);

  template <typename SerialiserType>
  bool Serialise_CaptureScope(SerialiserType &ser);
  template <typename SerialiserType>
  bool Serialise_BeginCaptureFrame(SerialiserType &ser);

  void AddResourceCurChunk(ResourceDescription &descr);
  void AddResourceCurChunk(ResourceId id);

  void ApplyInitialContents();
  bool ProcessChunk(ReadSerialiser &ser, MetalChunk chunk);
  bool ContextProcessChunk(ReadSerialiser &ser, MetalChunk chunk);
  bool HasSuccessfulCapture();
  static rdcstr GetChunkName(uint32_t idx);
  RDResult ContextReplayLog(CaptureState readType, uint32_t startEventID, uint32_t endEventID,
                            bool partial);

  WrappedMTLCommandBuffer *GetInitStateCmd();
  void CloseInitStateCmd();
  void AddPendingCommandBuffer(WrappedMTLCommandBuffer *cmdBuffer);
  void InsertCommandBufferActionsAndRefreshIDs(ReplayCmdBufferInfo &cmdBufInfo);

  void ClearSubmittedCmdBuffers();
  void SubmitCommandBuffer(MetalResourceRecord *record);

  MetalTextureState *InsertTextureState(WrappedMTLTexture *wrappedHandle, ResourceId id,
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

  MetalDebugManager *m_DebugManager = NULL;

  MetalRenderState m_RenderState;

  struct
  {
    void Reset()
    {
      m_PendingCmds.clear();
      m_SubmittedCmds.clear();
    }

    // GetNextCommandBuffer() ->
    rdcarray<WrappedMTLCommandBuffer *> m_PendingCmds;
    // -> SubmitCmds() ->
    rdcarray<WrappedMTLCommandBuffer *> m_SubmittedCmds;
    // -> FlushQ()

  } m_InternalCmds;

  static const int initialStateMaxBatch = 100;
  int initStateCurBatch = 0;
  WrappedMTLCommandBuffer *initStateCurCmd = NULL;

  struct ReplayCmdBufferInfo
  {
    ReplayCmdBufferInfo() {}
    ~ReplayCmdBufferInfo() { SAFE_DELETE(action); }
    rdcarray<APIEvent> curEvents;
    rdcarray<MetalActionTreeNode *> actionStack;

    uint32_t beginChunk = 0;
    uint32_t endChunk = 0;

    MetalRenderState renderState;
    RDMTL::RenderPassDescriptor rpDesc;

    // whether the renderdoc commandbuffer execution has an encoder
    // open and replaying and need to end the encoder before commit
    MetalRenderState::PipelineBinding activeEncoder = MetalRenderState::BindNone;
    WrappedMTLCommandBuffer *cmdBuffer = NULL;

    MetalActionTreeNode *action = NULL;    // the root action to copy from when submitting
    uint32_t baseRootActionID = 0;         // which root action ID the cmd buffer starts from
    uint32_t baseRootEventID = 0;          // which root event ID the cmd buffer starts from
    uint32_t actionCount = 0;              // how many actions are in the cmd buffer
    uint32_t eventCount = 0;    // how many events are in the cmd buffer, for quick skipping
    uint32_t curEventID = 0;    // current event ID while reading or executing
    int32_t queueIndex = 0;     // the GPU submission index for this command buffer
  };

  int32_t m_ReplayNextCmdBufferQueueIndex = 0;
  // on replay, the current command buffer for the last chunk we handled.
  ResourceId m_ReplayCurrentCmdBufferID;
  // data for a command buffer - its actions and events
  std::map<ResourceId, ReplayCmdBufferInfo> m_ReplayCmdBufferInfos;
  // on replay, the command buffer which was only partially committed
  ResourceId m_ReplayPartialCmdBufferID;

  // if we're replaying just a single action, we don't go through the
  // whole original command buffers to set up the partial replay,
  // we just set this command buffer
  WrappedMTLCommandBuffer *m_ReplayPartialCmdBuffer = NULL;

  // used both on capture and replay side to track texture state.
  // Only locked in capture
  MetalTextureStates m_TextureStates;
  Threading::CriticalSection m_TextureStatesLock;

  rdcarray<APIEvent> m_RootEvents, m_Events;
  bool m_AddedAction = false;
  MetalActionTreeNode m_ParentAction;
  rdcarray<MetalActionTreeNode *> m_ActionStack;
  rdcarray<ActionDescription *> m_Actions;

  uint64_t m_CurChunkOffset = 0;
  SDChunkMetaData m_ChunkMetadata;
  uint32_t m_RootEventID = 1;
  uint32_t m_RootActionID = 1;
  uint32_t m_FirstEventID = 0;
  uint32_t m_LastEventID = ~0U;
  MetalChunk m_LastChunk;

  // Active object used for replaying
  WrappedMTLCommandQueue *m_ReplayCommandQueue = NULL;

  // Dummy objects used for serialisation replay
  WrappedMTLBuffer *m_DummyBuffer = NULL;
  WrappedMTLCommandBuffer *m_DummyReplayCommandBuffer = NULL;
  WrappedMTLCommandQueue *m_DummyReplayCommandQueue = NULL;
  WrappedMTLLibrary *m_DummyReplayLibrary = NULL;
  WrappedMTLRenderCommandEncoder *m_DummyReplayRenderCommandEncoder = NULL;
  WrappedMTLBlitCommandEncoder *m_DummyReplayBlitCommandEncoder = NULL;

  MetalReplay *m_Replay = NULL;
  ReplayOptions m_ReplayOptions;

  // Back buffer and swap chain emulation
  Threading::CriticalSection m_CapturePotentialBackBuffersLock;
  std::unordered_set<WrappedMTLTexture *> m_CapturePotentialBackBuffers;
  Threading::CriticalSection m_CaptureOutputLayersLock;
  std::unordered_set<CA::MetalLayer *> m_CaptureOutputLayers;
  WrappedMTLTexture *m_CapturedBackbuffer = NULL;
  Threading::CriticalSection m_CaptureDrawablesLock;
  rdcflatmap<MTL::Drawable *, MetalDrawableInfo> m_CaptureDrawableInfos;

  CaptureState m_State;
  bool m_AppControlledCapture = false;
  SDFile *m_StructuredFile = NULL;
  SDFile m_StoredStructuredData;
  StreamReader *m_FrameReader = NULL;

  RDResult m_FailedReplayResult = ResultCode::APIUnsupported;
  RDResult m_FatalError = ResultCode::Succeeded;

  std::set<rdcstr> m_StringDB;
  uint64_t m_TimeBase = 0;
  double m_TimeFrequency = 1.0f;

  bool m_MarkedActive = false;

  uint64_t threadSerialiserTLSSlot;
  Threading::CriticalSection m_ThreadSerialisersLock;
  rdcarray<WriteSerialiser *> m_ThreadSerialisers;
  uint64_t m_SectionVersion = 0;

  MetalCapturer m_Capturer;
  uint32_t m_FrameCounter = 0;
  rdcarray<FrameDescription> m_CapturedFrames;
  Threading::RWLock m_CapTransitionLock;
  MetalResourceRecord *m_FrameCaptureRecord = NULL;

  // record the command buffer records to insert them individually
  // (even if they were recorded locklessly in parallel)
  // queue submit order will enforce/display ordering, record order is not important
  Threading::CriticalSection m_CaptureCommandBuffersLock;
  rdcarray<MetalResourceRecord *> m_CaptureCommandBuffersEnqueued;
  rdcarray<MetalResourceRecord *> m_CaptureCommandBuffersSubmitted;

  MetalCreationInfo m_CreationInfo;
  PerformanceTimer m_CaptureTimer;
  MetalInitParams m_InitParams;

  MTL::CommandQueue *m_mtlCommandQueue = NULL;
};

inline void MetalCapturer::StartFrameCapture(DeviceOwnedWindow devWnd)
{
  return m_Device.StartFrameCapture(devWnd);
}
inline bool MetalCapturer::EndFrameCapture(DeviceOwnedWindow devWnd)
{
  return m_Device.EndFrameCapture(devWnd);
}
inline bool MetalCapturer::DiscardFrameCapture(DeviceOwnedWindow devWnd)
{
  return m_Device.DiscardFrameCapture(devWnd);
}
