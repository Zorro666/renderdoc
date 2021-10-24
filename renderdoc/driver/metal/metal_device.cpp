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

#include "metal_device.h"
#include "metal_helpers.h"
#include "metal_replay.h"

#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_command_queue.h"
#include "metal_function.h"
#include "metal_library.h"
#include "metal_render_command_encoder.h"
#include "metal_render_pipeline_state.h"
#include "metal_texture.h"

WrappedMTLDevice::WrappedMTLDevice() : WrappedMTLObject(this, GetStateRef())
{
  Construct();
}

WrappedMTLDevice::WrappedMTLDevice(MTL::Device *realMTLDevice, ResourceId objId)
    : WrappedMTLObject(realMTLDevice, objId, this, GetStateRef())
{
  wrappedObjC = AllocateObjCWrapper(this);
  Construct();
  GetResourceManager()->AddCurrentResource(objId, this);
}

void WrappedMTLDevice::Construct()
{
  m_WrappedMTLDevice = this;
  m_DummyReplayLibrary = NULL;
  m_DummyReplayCommandQueue = NULL;
  m_DummyReplayCommandBuffer = NULL;
  m_DummyReplayRenderCommandEncoder = NULL;
  m_DummyBuffer = NULL;
  m_OutsideCmdBuffer = NULL;

  if(RenderDoc::Inst().IsReplayApp())
  {
    m_State = CaptureState::LoadingReplaying;
    m_DummyReplayLibrary = new WrappedMTLLibrary(this);
    m_DummyReplayCommandQueue = new WrappedMTLCommandQueue(this);
    m_DummyReplayCommandBuffer = new WrappedMTLCommandBuffer(this);
    m_DummyReplayCommandBuffer->SetWrappedMTLCommandQueue(m_DummyReplayCommandQueue);
    m_DummyReplayRenderCommandEncoder = new WrappedMTLRenderCommandEncoder(this);
    m_DummyReplayRenderCommandEncoder->SetWrappedMTLCommandBuffer(m_DummyReplayCommandBuffer);
    m_DummyBuffer = new WrappedMTLBuffer(this);
  }
  else
  {
    m_State = CaptureState::BackgroundCapturing;
  }

  m_SectionVersion = MetalInitParams::CurrentVersion;

  threadSerialiserTLSSlot = Threading::AllocateTLSSlot();

  m_RootEventID = 1;
  m_RootActionID = 1;
  m_FirstEventID = 0;
  m_LastEventID = ~0U;

  // m_ActionCallback = NULL;

  m_CurChunkOffset = 0;
  m_AddedAction = false;

  m_LastCmdBufferID = ResourceId();

  m_ActionStack.push_back(&m_ParentAction);

  m_ResourceManager = new MetalResourceManager(m_State, this);

  m_Replay = new MetalReplay(this);

  m_HeaderChunk = NULL;

  if(!RenderDoc::Inst().IsReplayApp())
  {
    m_FrameCaptureRecord = GetResourceManager()->AddResourceRecord(ResourceIDGen::GetNewUniqueID());
    m_FrameCaptureRecord->DataInSerialiser = false;
    m_FrameCaptureRecord->Length = 0;
    m_FrameCaptureRecord->InternalResource = true;
  }
  else
  {
    m_FrameCaptureRecord = NULL;

    ResourceIDGen::SetReplayResourceIDs();
  }
  RDCASSERT(m_WrappedMTLDevice == this);
}

MTL::Device *WrappedMTLDevice::MTLCreateSystemDefaultDevice(MTL::Device *realMTLDevice)
{
  MTLFixupForMetalDriverAssert();
  ResourceId objId = ResourceIDGen::GetNewUniqueID();
  WrappedMTLDevice *wrappedMTLDevice = new WrappedMTLDevice(realMTLDevice, objId);

  return wrappedMTLDevice->CreateInstance();
}

MTL::Device *WrappedMTLDevice::CreateInstance()
{
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;

    {
      CACHE_THREAD_SERIALISER();

      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLCreateSystemDefaultDevice);
      Serialise_MTLCreateSystemDefaultDevice(ser, this);
      chunk = scope.Get();
    }

    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(this);
    RDCASSERT(record);

    record->AddChunk(chunk);
  }
  else
  {
    GetResourceManager()->AddLiveResource(id, this);
    m_Replay->CreateResources();
  }

  FirstFrame();

  return UnwrapObjC<MTL::Device *>(this);
}

ReplayStatus WrappedMTLDevice::Initialise(MetalInitParams &params, uint64_t sectionVersion,
                                          const ReplayOptions &opts)
{
  m_InitParams = params;
  m_SectionVersion = sectionVersion;
  m_ReplayOptions = opts;

  m_ResourceManager->SetOptimisationLevel(m_ReplayOptions.optimisation);

  real = real_MTLCreateSystemDefaultDevice();
  wrappedObjC = AllocateObjCWrapper(this);
  id = ResourceIDGen::GetNewUniqueID();
  GetResourceManager()->AddCurrentResource(id, this);
  MTL::CommandQueue *commandQueue_objc = newCommandQueue();
  m_ReplayCommandQueue = GetWrapped(commandQueue_objc);

  /*
    m_Instance = NULL;

    GetResourceManager()->WrapResource(m_Instance, m_Instance);
    // we'll add the chunk later when we re-process it.
    if(params.InstanceID != ResourceId())
    {
      GetResourceManager()->AddLiveResource(params.InstanceID, m_Instance);

      AddResource(params.InstanceID, ResourceType::Device, "Instance");
      GetReplay()->GetResourceDesc(params.InstanceID).initialisationChunks.clear();
    }
    else
    {
      GetResourceManager()->AddLiveResource(GetResID(m_Instance), m_Instance);
    }
  */
  m_Replay->CreateResources();
  return ReplayStatus::Succeeded;
}

MTL::Buffer *WrappedMTLDevice::newBufferWithLength(NS::UInteger length, MTL::ResourceOptions options)
{
  MTL::Buffer *realMTLBuffer;
  SERIALISE_TIME_CALL(realMTLBuffer = real_newBufferWithLength(length, options));

  MetalResourceManager::UnwrapHelper<MTL::Buffer *>::Outer *wrappedMTLBuffer;
  ResourceId id = GetResourceManager()->WrapResource(realMTLBuffer, wrappedMTLBuffer);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newBufferWithLength);
      Serialise_mtlDevice_newBuffer(ser, this, wrappedMTLBuffer, NULL, length, options);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLBuffer);
    record->AddChunk(chunk);
    GetResourceManager()->MarkResourceFrameReferenced(id, eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, wrappedMTLBuffer);
  }
  return UnwrapObjC<MTL::Buffer *>(wrappedMTLBuffer);
}

MTL::Buffer *WrappedMTLDevice::newBufferWithBytes(const void *pointer, NS::UInteger length,
                                                  MTL::ResourceOptions options)
{
  MTL::Buffer *realMTLBuffer;
  SERIALISE_TIME_CALL(realMTLBuffer = real_newBufferWithBytes(pointer, length, options));

  MetalResourceManager::UnwrapHelper<MTL::Buffer *>::Outer *wrappedMTLBuffer;
  ResourceId id = GetResourceManager()->WrapResource(realMTLBuffer, wrappedMTLBuffer);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newBuffer);
      Serialise_mtlDevice_newBuffer(ser, this, wrappedMTLBuffer, pointer, length, options);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLBuffer);
    record->AddChunk(chunk);
    GetResourceManager()->MarkResourceFrameReferenced(id, eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, wrappedMTLBuffer);
  }
  return UnwrapObjC<MTL::Buffer *>(wrappedMTLBuffer);
}

MTL::RenderPipelineState *WrappedMTLDevice::newRenderPipelineStateWithDescriptor(
    MTL::RenderPipelineDescriptor *descriptor, NS::Error **error)
{
  MTL::RenderPipelineState *realMTLRenderPipelineState;
  SERIALISE_TIME_CALL(realMTLRenderPipelineState =
                          real_newRenderPipelineStateWithDescriptor(descriptor, error));

  MetalResourceManager::UnwrapHelper<MTL::RenderPipelineState *>::Outer *wrappedMTLRenderPipelineState;
  ResourceId id =
      GetResourceManager()->WrapResource(realMTLRenderPipelineState, wrappedMTLRenderPipelineState);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newRenderPipelineStateWithDescriptor);
      Serialise_mtlDevice_newRenderPipelineStateWithDescriptor(
          ser, this, wrappedMTLRenderPipelineState, descriptor);
      chunk = scope.Get();
    }

    record->AddChunk(chunk);
    record->AddParent(GetRecord(GetWrapped((MTL::Function *)descriptor->vertexFunction())));
    record->AddParent(GetRecord(GetWrapped((MTL::Function *)descriptor->fragmentFunction())));
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLRenderPipelineState);
  }
  return UnwrapObjC<MTL::RenderPipelineState *>(wrappedMTLRenderPipelineState);
}

MTL::Library *WrappedMTLDevice::newDefaultLibrary()
{
  MTL::Library *realMTLLibrary;
  SERIALISE_TIME_CALL(realMTLLibrary = real_newDefaultLibrary());
  MetalResourceManager::UnwrapHelper<MTL::Library *>::Outer *wrappedMTLLibrary;
  ResourceId id = GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
  if(IsCaptureMode(m_State))
  {
    void *pData;
    uint32_t bytesCount;
    ObjC::Get_defaultLibraryData(pData, bytesCount);
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newDefaultLibrary);
      Serialise_mtlDevice_newDefaultLibrary(ser, this, wrappedMTLLibrary, pData, bytesCount);
      chunk = scope.Get();
    }
    free(pData);
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLLibrary);
    record->AddChunk(chunk);
    GetResourceManager()->MarkResourceFrameReferenced(id, eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, wrappedMTLLibrary);
  }
  return UnwrapObjC<MTL::Library *>(wrappedMTLLibrary);
}

MTL::Library *WrappedMTLDevice::newLibraryWithSource(NS::String *source,
                                                     MTL::CompileOptions *options, NS::Error **error)
{
  MTL::Library *realMTLLibrary;
  SERIALISE_TIME_CALL(realMTLLibrary = real_newLibraryWithSource(source, options, error));
  MetalResourceManager::UnwrapHelper<MTL::Library *>::Outer *wrappedMTLLibrary;
  ResourceId id = GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newLibraryWithSource);
      Serialise_mtlDevice_newLibraryWithSource(ser, this, wrappedMTLLibrary, source, options);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLLibrary);
    record->AddChunk(chunk);
    GetResourceManager()->MarkResourceFrameReferenced(id, eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, wrappedMTLLibrary);
  }
  return UnwrapObjC<MTL::Library *>(wrappedMTLLibrary);
}

MTL::CommandQueue *WrappedMTLDevice::newCommandQueue()
{
  MTL::CommandQueue *realMTLCommandQueue;
  SERIALISE_TIME_CALL(realMTLCommandQueue = real_newCommandQueue());
  MetalResourceManager::UnwrapHelper<MTL::CommandQueue *>::Outer *wrappedMTLCommandQueue;
  ResourceId id = GetResourceManager()->WrapResource(realMTLCommandQueue, wrappedMTLCommandQueue);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newCommandQueue);
      Serialise_mtlDevice_newCommandQueue(ser, this, wrappedMTLCommandQueue);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLCommandQueue);
    record->AddChunk(chunk);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, wrappedMTLCommandQueue);
  }
  return UnwrapObjC<MTL::CommandQueue *>(wrappedMTLCommandQueue);
}

MTL::Texture *WrappedMTLDevice::newTextureWithDescriptor(MTL::TextureDescriptor *descriptor)
{
  MTL::Texture *realMTLTexture;
  SERIALISE_TIME_CALL(realMTLTexture = real_newTextureWithDescriptor(descriptor));
  MetalResourceManager::UnwrapHelper<MTL::Texture *>::Outer *wrappedMTLTexture;
  ResourceId id = GetResourceManager()->WrapResource(realMTLTexture, wrappedMTLTexture);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newTextureWithDescriptor);
      Serialise_mtlDevice_newTextureWithDescriptor(ser, this, wrappedMTLTexture, descriptor, NULL,
                                                   ~0);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLTexture);
    record->AddChunk(chunk);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, wrappedMTLCommandQueue);
  }
  return UnwrapObjC<MTL::Texture *>(wrappedMTLTexture);
}

MTL::Texture *WrappedMTLDevice::newTextureWithDescriptor(MTL::TextureDescriptor *descriptor,
                                                         IOSurfaceRef iosurface, NS::UInteger plane)
{
  void *window = (void *)this;
  RenderDoc::Inst().AddFrameCapturer(this, window, this);

  MTL::Texture *realMTLTexture;
  SERIALISE_TIME_CALL(realMTLTexture = real_newTextureWithDescriptor(descriptor, iosurface, plane));
  MetalResourceManager::UnwrapHelper<MTL::Texture *>::Outer *wrappedMTLTexture;
  ResourceId id = GetResourceManager()->WrapResource(realMTLTexture, wrappedMTLTexture);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newTextureWithDescriptor);
      Serialise_mtlDevice_newTextureWithDescriptor(ser, this, wrappedMTLTexture, descriptor,
                                                   iosurface, plane);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLTexture);
    record->AddChunk(chunk);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, wrappedMTLCommandQueue);
  }
  return UnwrapObjC<MTL::Texture *>(wrappedMTLTexture);
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newBuffer(SerialiserType &ser, WrappedMTLDevice *device,
                                                     WrappedMTLBuffer *buffer, const void *pointer,
                                                     NS::UInteger length,
                                                     MTL::ResourceOptions options)
{
  SERIALISE_ELEMENT_LOCAL(Buffer, GetResID(buffer)).TypedAs("MTLBuffer"_lit);
  SERIALISE_ELEMENT_ARRAY(pointer, length);
  SERIALISE_ELEMENT(length);
  SERIALISE_ELEMENT(options);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    MTL::Buffer *realMTLBuffer;
    if(pointer)
    {
      realMTLBuffer = real_newBufferWithBytes(pointer, length, options);
    }
    else
    {
      realMTLBuffer = real_newBufferWithLength(length, options);
    }
    MetalResourceManager::UnwrapHelper<MTL::Buffer *>::Outer *wrappedMTLBuffer;
    GetResourceManager()->WrapResource(realMTLBuffer, wrappedMTLBuffer);
    GetResourceManager()->AddLiveResource(Buffer, wrappedMTLBuffer);

    AddResource(Buffer, ResourceType::Buffer, "Buffer");
    DerivedResource(this, Buffer);

    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //
    //      ActionDescription action;
    //      action.flags |= ActionFlags::CmdList;
    //      action.customName = "newBufferWithBytes";
    //
    //      AddAction(action);
    //    }
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newDefaultLibrary(SerialiserType &ser,
                                                             WrappedMTLDevice *device,
                                                             WrappedMTLLibrary *library,
                                                             const void *pData, uint32_t bytesCount)
{
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);
  SERIALISE_ELEMENT(bytesCount);
  SERIALISE_ELEMENT_ARRAY(pData, bytesCount);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    MTL::Library *realMTLLibrary = CreateMTLLibrary(pData, bytesCount);
    MetalResourceManager::UnwrapHelper<MTL::Library *>::Outer *wrappedMTLLibrary;
    GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
    GetResourceManager()->AddLiveResource(Library, wrappedMTLLibrary);
    AddResource(Library, ResourceType::Pool, "Library");
    DerivedResource(this, Library);
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newLibraryWithSource(SerialiserType &ser,
                                                                WrappedMTLDevice *device,
                                                                WrappedMTLLibrary *library,
                                                                NS::String *source,
                                                                MTL::CompileOptions *options)
{
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);
  SERIALISE_ELEMENT(source);
  // TODO:SERIALISE_ELEMENT(options);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    NS::Error *error = NULL;
    MTL::Library *realMTLLibrary = real_newLibraryWithSource(source, options, &error);
    MetalResourceManager::UnwrapHelper<MTL::Library *>::Outer *wrappedMTLLibrary;
    GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
    GetResourceManager()->AddLiveResource(Library, wrappedMTLLibrary);
    AddResource(Library, ResourceType::Pool, "Library");
    DerivedResource(this, Library);
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newRenderPipelineStateWithDescriptor(
    SerialiserType &ser, WrappedMTLDevice *device, WrappedMTLRenderPipelineState *pipelineState,
    MTL::RenderPipelineDescriptor *descriptor)
{
  SERIALISE_ELEMENT_LOCAL(RenderPipelineState, GetResID(pipelineState))
      .TypedAs("MTLRenderPipelineState"_lit);
  SERIALISE_ELEMENT(descriptor);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    ResourceId liveID;

    MTL::RenderPipelineState *realMTLRenderPipelineState =
        real_newRenderPipelineStateWithDescriptor(descriptor, NULL);
    MetalResourceManager::UnwrapHelper<MTL::RenderPipelineState *>::Outer *wrappedMTLRenderPipelineState;
    liveID = GetResourceManager()->WrapResource(realMTLRenderPipelineState,
                                                wrappedMTLRenderPipelineState);
    GetResourceManager()->AddLiveResource(RenderPipelineState, wrappedMTLRenderPipelineState);
    AddResource(RenderPipelineState, ResourceType::PipelineState, "PipelineState");
    DerivedResource(this, RenderPipelineState);

    MetalCreationInfo::Pipeline &pipeline = m_CreationInfo.m_Pipeline[liveID];
    // Save pipeline settings from MTLRenderPipelineDescriptor
    // MTL::Function* = MTLRenderPipelineDescriptor::vertexFunction
    // MTL::Function* = MTLRenderPipelineDescriptor::fragmentFunction
    // TODO: vertexDescriptor : MTLVertexDescriptor
    // TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
    // TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
    const uint32_t colorAttachmentCount = MAX_RENDER_PASS_COLOR_ATTACHMENTS;
    pipeline.attachments.resize(colorAttachmentCount);
    for(uint32_t i = 0; i < colorAttachmentCount; ++i)
    {
      MTL::RenderPipelineColorAttachmentDescriptor *mtlAttachment =
          descriptor->colorAttachments()->object(i);
      MetalCreationInfo::Pipeline::Attachment &attachment = pipeline.attachments[i];
      attachment.pixelFormat = mtlAttachment->pixelFormat();
      attachment.blendingEnabled = mtlAttachment->blendingEnabled();
      attachment.sourceRGBBlendFactor = mtlAttachment->sourceRGBBlendFactor();
      attachment.destinationRGBBlendFactor = mtlAttachment->destinationRGBBlendFactor();
      attachment.rgbBlendOperation = mtlAttachment->rgbBlendOperation();
      attachment.sourceAlphaBlendFactor = mtlAttachment->sourceAlphaBlendFactor();
      attachment.destinationAlphaBlendFactor = mtlAttachment->destinationAlphaBlendFactor();
      attachment.alphaBlendOperation = mtlAttachment->alphaBlendOperation();
      attachment.writeMask = mtlAttachment->writeMask();
    }
    // MTLPixelFormat = MTLRenderPipelineDescriptor::depthAttachmentPixelFormat
    // MTLPixelFormat = MTLRenderPipelineDescriptor::stencilAttachmentPixelFormat
    // NSUInteger = MTLRenderPipelineDescriptor::sampleCount
    pipeline.alphaToCoverageEnabled = descriptor->alphaToCoverageEnabled();
    pipeline.alphaToOneEnabled = descriptor->alphaToOneEnabled();
    pipeline.rasterizationEnabled = descriptor->rasterizationEnabled();
    // MTLPrimitiveTopologyClass = MTLRenderPipelineDescriptor::inputPrimitiveTopology
    // NSUInteger = MTLRenderPipelineDescriptor::rasterSampleCount
    // NSUInteger = MTLRenderPipelineDescriptor::maxTessellationFactor
    // bool = MTLRenderPipelineDescriptor::tessellationFactorScaleEnabled
    // MTLTessellationFactorFormat = MTLRenderPipelineDescriptor::tessellationFactorFormat
    // MTLTessellationControlPointIndexType =
    // MTLRenderPipelineDescriptor::tessellationControlPointIndexType
    // MTLTessellationFactorStepFunction =
    // MTLRenderPipelineDescriptor::tessellationFactorStepFunction
    // MTLWinding = MTLRenderPipelineDescriptor::tessellationOutputWindingOrder
    // MTLTessellationPartitionMode = MTLRenderPipelineDescriptor::tessellationPartitionMode
    // bool = MTLRenderPipelineDescriptor::supportIndirectCommandBuffers
    // NSUInteger = MTLRenderPipelineDescriptor::maxVertexAmplificationCount
    // TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newCommandQueue(SerialiserType &ser,
                                                           WrappedMTLDevice *device,
                                                           WrappedMTLCommandQueue *queue)
{
  SERIALISE_ELEMENT_LOCAL(CommandQueue, GetResID(queue)).TypedAs("MTLCommandQueue"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    MTL::CommandQueue *realMTLCommandQueue = real_newCommandQueue();
    MetalResourceManager::UnwrapHelper<MTL::CommandQueue *>::Outer *wrappedMTLCommandQueue;
    GetResourceManager()->WrapResource(realMTLCommandQueue, wrappedMTLCommandQueue);
    GetResourceManager()->AddLiveResource(CommandQueue, wrappedMTLCommandQueue);

    AddResource(CommandQueue, ResourceType::Queue, "Queue");
    DerivedResource(this, CommandQueue);
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_MTLCreateSystemDefaultDevice(SerialiserType &ser,
                                                              WrappedMTLDevice *device)
{
  SERIALISE_ELEMENT_LOCAL(Device, GetResID(device)).TypedAs("MTLDevice"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    RDCASSERT(real);
    RDCASSERT(id != ResourceId::Null());
    //    real = real_MTLCreateSystemDefaultDevice();
    //    id = ResourceIDGen::GetNewUniqueID();
    //    GetResourceManager()->AddCurrentResource(id, this);
    //    MTL::CommandQueue* commandQueue_objc = newCommandQueue();
    //    m_ReplayCommandQueue = GetWrapped(commandQueue_objc);

    GetResourceManager()->AddLiveResource(Device, this);
    AddResource(Device, ResourceType::Device, "Device");
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newTextureWithDescriptor(
    SerialiserType &ser, WrappedMTLDevice *device, WrappedMTLTexture *texture,
    MTL::TextureDescriptor *descriptor, IOSurfaceRef iosurface, NS::UInteger plane)
{
  SERIALISE_ELEMENT_LOCAL(Texture, GetResID(texture)).TypedAs("MTLTexture"_lit);
  SERIALISE_ELEMENT(descriptor);
  SERIALISE_ELEMENT_LOCAL(IOSurfaceRef, (uint64_t)iosurface).TypedAs("IOSurfaceRef"_lit);
  SERIALISE_ELEMENT(plane);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    MTL::Texture *realMTLTexture = real_newTextureWithDescriptor(descriptor);
    MetalResourceManager::UnwrapHelper<MTL::Texture *>::Outer *wrappedMTLTexture;
    ResourceId liveID = GetResourceManager()->WrapResource(realMTLTexture, wrappedMTLTexture);
    GetResourceManager()->AddLiveResource(Texture, wrappedMTLTexture);

    m_CreationInfo.m_Texture[liveID].Init(GetResourceManager(), m_CreationInfo, descriptor);
    m_CreationInfo.m_Texture[liveID].creationFlags |= TextureCategory::SwapBuffer;

    bool inserted = false;
    MetalResources::ImageInfo info;
    info.usage = descriptor->usage();
    info.extent.width = descriptor->width();
    info.extent.height = descriptor->height();
    info.extent.depth = descriptor->depth();
    info.type = descriptor->textureType();
    info.layerCount = 0;
    info.levelCount = 0;
    info.sampleCount = 0;
    info.storage = 0;

    MTL::Texture *objCTexture = UnwrapObjC<MTL::Texture *>(wrappedMTLTexture);
    InsertImageState(objCTexture, liveID, info, eFrameRef_Unknown, &inserted);

    AddResource(Texture, ResourceType::Texture, "Texture");
    DerivedResource(this, Texture);
  }
  return true;
}

MTL::Library *WrappedMTLDevice::real_newDefaultLibrary()
{
  fprintf(stderr, "real_newDefaultLibrary\n");
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::Library *realMTLLibrary = realMTLDevice->newDefaultLibrary();
  return realMTLLibrary;
}

MTL::Library *WrappedMTLDevice::real_newLibraryWithSource(NS::String *source,
                                                          MTL::CompileOptions *options,
                                                          NS::Error **error)
{
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::Library *realMTLLibrary =
      realMTLDevice->newLibrary(source, (MTL::CompileOptions *)options, error);
  return realMTLLibrary;
}

MTL::Device *WrappedMTLDevice::real_MTLCreateSystemDefaultDevice()
{
  MTLFixupForMetalDriverAssert();
  return MTL::CreateSystemDefaultDevice();
}

MTL::CommandQueue *WrappedMTLDevice::real_newCommandQueue()
{
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::CommandQueue *realMTLCommandQueue = realMTLDevice->newCommandQueue();
  return realMTLCommandQueue;
}

MTL::RenderPipelineState *WrappedMTLDevice::real_newRenderPipelineStateWithDescriptor(
    MTL::RenderPipelineDescriptor *descriptor, NS::Error **error)
{
  MTL::Device *realDevice = Unwrap<MTL::Device *>(this);

  MTL::RenderPipelineDescriptor *realDescriptor = descriptor->copy();

  // The source descriptor contains wrapped MTLFunction resources
  // These need to be unwrapped in the clone which is used when calling real API
  MTL::Function *wrappedVertexFunction = descriptor->vertexFunction();
  if(wrappedVertexFunction != NULL)
  {
    realDescriptor->setVertexFunction(GetReal(wrappedVertexFunction));
  }

  MTL::Function *wrappedFragmentFunction = descriptor->fragmentFunction();
  if(wrappedFragmentFunction != NULL)
  {
    realDescriptor->setFragmentFunction(GetReal(wrappedFragmentFunction));
  }

  MTL::RenderPipelineState *realMTLRenderPipelineState =
      realDevice->newRenderPipelineState(realDescriptor, error);
  realDescriptor->release();

  return realMTLRenderPipelineState;
}

MTL::Buffer *WrappedMTLDevice::real_newBufferWithLength(NS::UInteger length,
                                                        MTL::ResourceOptions options)
{
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::Buffer *realMTLBuffer = realMTLDevice->newBuffer(length, options);
  return realMTLBuffer;
}

MTL::Buffer *WrappedMTLDevice::real_newBufferWithBytes(const void *pointer, NS::UInteger length,
                                                       MTL::ResourceOptions options)
{
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::Buffer *realMTLBuffer = realMTLDevice->newBuffer(pointer, length, options);
  return realMTLBuffer;
}

MTL::Texture *WrappedMTLDevice::real_newTextureWithDescriptor(MTL::TextureDescriptor *descriptor,
                                                              IOSurfaceRef iosurface,
                                                              NS::UInteger plane)
{
  // TODO: should only modify this when Capturing
  // TODO: what about other usage types ie. MTLTextureUsageShaderRead
  if(descriptor->usage() == MTL::TextureUsageRenderTarget)
    descriptor->setUsage(MTL::TextureUsageUnknown);
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::Texture *realMTLTexture = realMTLDevice->newTexture(descriptor, iosurface, plane);
  return realMTLTexture;
}

MTL::Texture *WrappedMTLDevice::real_newTextureWithDescriptor(MTL::TextureDescriptor *descriptor)
{
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::Texture *realMTLTexture = realMTLDevice->newTexture(descriptor);
  return realMTLTexture;
}

MTL::Library *WrappedMTLDevice::CreateMTLLibrary(const void *pData, uint32_t bytesCount)
{
  dispatch_data_t data = dispatch_data_create(pData, bytesCount, dispatch_get_main_queue(),
                                              DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  NS::Error *error;
  MTL::Device *realMTLDevice = Unwrap<MTL::Device *>(this);
  MTL::Library *library = realMTLDevice->newLibrary(data, &error);
  dispatch_release(data);
  return library;
}

CA::MetalDrawable *WrappedMTLDevice::GetNextDrawable(void *layer)
{
  ObjC::Set_Device(layer, (MTL::Device *)wrappedObjC);
  CA::MetalDrawable *drawable = MTLGetNextDrawable(layer);
  return drawable;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newDefaultLibrary,
                                WrappedMTLDevice *device, WrappedMTLLibrary *library,
                                const void *pData, uint32_t bytesCount);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newLibraryWithSource,
                                WrappedMTLDevice *device, WrappedMTLLibrary *library,
                                NS::String *source, MTL::CompileOptions *options);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newCommandQueue,
                                WrappedMTLDevice *device, WrappedMTLCommandQueue *queue);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, MTLCreateSystemDefaultDevice,
                                WrappedMTLDevice *device);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice,
                                mtlDevice_newRenderPipelineStateWithDescriptor,
                                WrappedMTLDevice *device,
                                WrappedMTLRenderPipelineState *renderPipelineState,
                                MTL::RenderPipelineDescriptor *descriptor);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newBuffer, WrappedMTLDevice *device,
                                WrappedMTLBuffer *buffer, const void *pointer, NS::UInteger length,
                                MTL::ResourceOptions options);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newTextureWithDescriptor,
                                WrappedMTLDevice *device, WrappedMTLTexture *texture,
                                MTL::TextureDescriptor *descriptor, IOSurfaceRef iosurface,
                                NS::UInteger plane);
