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

class WrappedMTLDevice : public WrappedMTLObject, public IFrameCapturer
{
public:
  friend class MetalReplay;
  friend class MetalResourceManager;

  WrappedMTLDevice();
  WrappedMTLDevice(MTL::Device *realMTLDevice, ResourceId objId);
  virtual ~WrappedMTLDevice() {}
  static MTL::Device *MTLCreateSystemDefaultDevice(MTL::Device *realMTLDevice);

  DECLARE_WRAPPED_API(MTL::Library *, newDefaultLibrary);
  DECLARE_WRAPPED_API(MTL::Library *, newLibraryWithSource, NS::String *source,
                      MTL::CompileOptions *options, NS::Error **error);
  DECLARE_WRAPPED_API(MTL::Buffer *, newBufferWithBytes, const void *pointer, NS::UInteger length,
                      MTL::ResourceOptions options);
  DECLARE_WRAPPED_API(MTL::Buffer *, newBufferWithLength, NS::UInteger length,
                      MTL::ResourceOptions options);
  DECLARE_WRAPPED_API(MTL::RenderPipelineState *, newRenderPipelineStateWithDescriptor,
                      MTL::RenderPipelineDescriptor *descriptor, NS::Error **error);
  DECLARE_WRAPPED_API(MTL::CommandQueue *, newCommandQueue);
  DECLARE_WRAPPED_API(MTL::Texture *, newTextureWithDescriptor, MTL::TextureDescriptor *descriptor,
                      IOSurfaceRef iosurface, NS::UInteger plane);
  DECLARE_WRAPPED_API(MTL::Texture *, newTextureWithDescriptor, MTL::TextureDescriptor *descriptor);

  RDCDriver GetFrameCaptureDriver() { return RDCDriver::Metal; }
  CaptureState GetState() { return m_State; }
  CaptureState &GetStateRef() { return m_State; }
  void StartFrameCapture(void *dev, void *wnd);
  bool EndFrameCapture(void *dev, void *wnd);
  bool DiscardFrameCapture(void *dev, void *wnd);

  ReplayStatus Initialise(MetalInitParams &params, uint64_t sectionVersion,
                          const ReplayOptions &opts);
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

  void SetActiveRenderCommandEnvoder(WrappedMTLRenderCommandEncoder *renderCommandEncoder)
  {
    m_ActiveRenderCommandEncoder = renderCommandEncoder;
  }

  void SetRenderPassActiveState(PartialReplayIndex index, bool state)
  {
    m_Partial[index].renderPassActive = state;
  }

  MetalCreationInfo &GetCreationInfo() { return m_CreationInfo; }
  rdcarray<EventUsage> GetUsage(ResourceId id) { return m_ResourceUses[id]; }
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newDefaultLibrary, WrappedMTLDevice *device,
                              WrappedMTLLibrary *library, const void *pData, uint32_t bytesCount);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newLibraryWithSource, WrappedMTLDevice *device,
                              WrappedMTLLibrary *library, NS::String *source,
                              MTL::CompileOptions *options);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newCommandQueue, WrappedMTLDevice *device,
                              WrappedMTLCommandQueue *queue);
  DECLARE_FUNCTION_SERIALISED(MTLCreateSystemDefaultDevice, WrappedMTLDevice *device);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newRenderPipelineStateWithDescriptor,
                              WrappedMTLDevice *device, WrappedMTLRenderPipelineState *pipelineState,
                              MTL::RenderPipelineDescriptor *descriptor);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newBuffer, WrappedMTLDevice *device,
                              WrappedMTLBuffer *buffer, const void *pointer, NS::UInteger length,
                              MTL::ResourceOptions options);
  DECLARE_FUNCTION_SERIALISED(mtlDevice_newTextureWithDescriptor, WrappedMTLDevice *device,
                              WrappedMTLTexture *texture, MTL::TextureDescriptor *descriptor,
                              IOSurfaceRef iosurface, NS::UInteger plane);

  enum
  {
    TypeEnum = eResDevice
  };

private:
  void Construct();
  MTL::Device *CreateInstance();
  static MTL::Device *real_MTLCreateSystemDefaultDevice();
  MTL::Library *CreateMTLLibrary(const void *pData, uint32_t bytesCount);

  void FirstFrame();
  ReplayStatus FatalErrorCheck() { return m_FatalError; }
  bool HasFatalError() { return m_FatalError != ReplayStatus::Succeeded; }
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
  void EndCaptureFrame();
  static rdcstr GetChunkName(uint32_t idx);
  ReplayStatus ContextReplayLog(CaptureState readType, uint32_t startEventID, uint32_t endEventID,
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
  WrappedMTLCommandBuffer *GetNextCmd();
  WrappedMTLCommandBuffer *GetInitStateCmd();
  void CloseInitStateCmd();
  void RemovePendingCommandBuffer(WrappedMTLCommandBuffer *cmd);
  void AddPendingCommandBuffer(WrappedMTLCommandBuffer *cmd);
  void SubmitCmds();
  void FlushQ();

  MetalResources::LockedImageStateRef FindImageState(ResourceId id);
  MetalResources::LockedConstImageStateRef FindConstImageState(ResourceId id);
  MetalResources::LockedImageStateRef InsertImageState(MTL::Texture *wrappedHandle, ResourceId id,
                                                       const MetalResources::ImageInfo &info,
                                                       FrameRefType refType, bool *inserted);

  std::map<ResourceId, rdcarray<EventUsage>> m_ResourceUses;
  rdcarray<DebugMessage> m_EventMessages;

  // list of all debug messages by EID in the frame
  rdcarray<DebugMessage> m_DebugMessages;

  MetalResourceManager *m_ResourceManager;

  MetalReplay *m_Replay;
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
  WrappedMTLCommandBuffer *m_OutsideCmdBuffer;
  WrappedMTLRenderCommandEncoder *m_ActiveRenderCommandEncoder;

  // used both on capture and replay side to track image state. Only locked
  // in capture
  std::map<ResourceId, MetalResources::LockingImageState> m_ImageStates;
  Threading::CriticalSection m_ImageStatesLock;

  rdcarray<APIEvent> m_RootEvents, m_Events;
  bool m_AddedAction;
  ActionDescription m_ParentAction;
  rdcarray<ActionDescription *> m_ActionStack;
  rdcarray<ActionDescription *> m_Actions;

  uint64_t m_CurChunkOffset;
  SDChunkMetaData m_ChunkMetadata;
  uint32_t m_RootEventID, m_RootActionID;
  uint32_t m_FirstEventID, m_LastEventID;
  MetalChunk m_LastChunk;
  // on replay, the current command buffer for the last chunk we
  // handled.
  ResourceId m_LastCmdBufferID;

  // Dummy objects used for serialisation replay
  WrappedMTLCommandBuffer *m_DummyReplayCommandBuffer;
  WrappedMTLCommandQueue *m_DummyReplayCommandQueue;
  WrappedMTLRenderCommandEncoder *m_DummyReplayRenderCommandEncoder;
  WrappedMTLLibrary *m_DummyReplayLibrary;
  WrappedMTLBuffer *m_DummyBuffer;

  // Active object used for replaying
  WrappedMTLCommandQueue *m_ReplayCommandQueue;

  // we record the command buffer records so we can insert them
  // individually, that means even if they were recorded locklessly
  // in parallel, on replay they are disjoint and it makes things
  // much easier to process (we will enforce/display ordering
  // by queue submit order anyway, so it's OK to lose the record
  // order).
  Threading::CriticalSection m_CmdBufferRecordsLock;
  rdcarray<MetalResourceRecord *> m_CmdBufferRecords;

  ReplayStatus m_FailedReplayStatus = ReplayStatus::APIUnsupported;

  ReplayStatus m_FatalError = ReplayStatus::Succeeded;
  CaptureState m_State;
  bool m_AppControlledCapture = false;
  int32_t m_ReuseEnabled = 1;
  PerformanceTimer m_CaptureTimer;
  uint32_t m_FrameCounter = 0;
  rdcarray<FrameDescription> m_CapturedFrames;
  Threading::RWLock m_CapTransitionLock;

  MetalResourceRecord *m_FrameCaptureRecord;
  Chunk *m_HeaderChunk;

  MetalCreationInfo m_CreationInfo;
  MetalInitParams m_InitParams;
  uint64_t m_SectionVersion;

  std::set<rdcstr> m_StringDB;
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
