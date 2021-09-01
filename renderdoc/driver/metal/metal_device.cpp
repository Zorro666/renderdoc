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

#include "metal_device.h"
#include "metal_metal.h"
#include "metal_replay.h"

WrappedMTLDevice::WrappedMTLDevice()
{
  Construct();
}

WrappedMTLDevice::WrappedMTLDevice(id_MTLDevice realMTLDevice, ResourceId objId)
    : WrappedMTLObject(realMTLDevice, objId, this)
{
  objc = CreateObjCWrappedMTLDevice();
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

id_MTLDevice WrappedMTLDevice::MTLCreateSystemDefaultDevice(id_MTLDevice realMTLDevice)
{
  FixupForMetalDriverAssert();
  ResourceId objId = ResourceIDGen::GetNewUniqueID();
  WrappedMTLDevice *wrappedMTLDevice = new WrappedMTLDevice(realMTLDevice, objId);

  return wrappedMTLDevice->CreateInstance();
}

id_MTLDevice WrappedMTLDevice::CreateInstance()
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

  return objc;
}

ReplayStatus WrappedMTLDevice::Initialise(MetalInitParams &params, uint64_t sectionVersion,
                                          const ReplayOptions &opts)
{
  m_InitParams = params;
  m_SectionVersion = sectionVersion;
  m_ReplayOptions = opts;

  m_ResourceManager->SetOptimisationLevel(m_ReplayOptions.optimisation);

  real = real_MTLCreateSystemDefaultDevice();
  objc = CreateObjCWrappedMTLDevice();
  id = ResourceIDGen::GetNewUniqueID();
  GetResourceManager()->AddCurrentResource(id, this);
  id_MTLCommandQueue commandQueue_objc = newCommandQueue();
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

id_MTLBuffer WrappedMTLDevice::newBufferWithLength(NSUInteger length, MTLResourceOptions options)
{
  id_MTLBuffer realMTLBuffer;
  SERIALISE_TIME_CALL(realMTLBuffer = real_newBufferWithLength(length, options));

  MetalResourceManager::UnwrapHelper<id_MTLBuffer>::Outer *wrappedMTLBuffer;
  ResourceId id = GetResourceManager()->WrapResource(realMTLBuffer, wrappedMTLBuffer);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newBufferWithLength);
      Serialise_mtlDevice_newBuffer(ser, this, wrappedMTLBuffer, NULL, (NSUInteger_objc)length,
                                    (MTLResourceOptions_objc)options);
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
  return wrappedMTLBuffer->objc;
}

id_MTLBuffer WrappedMTLDevice::newBufferWithBytes(const void *pointer, NSUInteger length,
                                                  MTLResourceOptions options)
{
  id_MTLBuffer realMTLBuffer;
  SERIALISE_TIME_CALL(realMTLBuffer = real_newBufferWithBytes(pointer, length, options));

  MetalResourceManager::UnwrapHelper<id_MTLBuffer>::Outer *wrappedMTLBuffer;
  ResourceId id = GetResourceManager()->WrapResource(realMTLBuffer, wrappedMTLBuffer);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newBuffer);
      Serialise_mtlDevice_newBuffer(ser, this, wrappedMTLBuffer, pointer, (NSUInteger_objc)length,
                                    (MTLResourceOptions_objc)options);
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
  return wrappedMTLBuffer->objc;
}

id_MTLRenderPipelineState WrappedMTLDevice::newRenderPipelineStateWithDescriptor(
    MTLRenderPipelineDescriptor *descriptor, NSError **error)
{
  id_MTLRenderPipelineState realMTLRenderPipelineState;
  SERIALISE_TIME_CALL(realMTLRenderPipelineState =
                          real_newRenderPipelineStateWithDescriptor(descriptor, error));

  MetalResourceManager::UnwrapHelper<id_MTLRenderPipelineState>::Outer *wrappedMTLRenderPipelineState;
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
    record->AddParent(GetRecord(GetWrapped(MTL_GET(descriptor, vertexFunction))));
    record->AddParent(GetRecord(GetWrapped(MTL_GET(descriptor, fragmentFunction))));
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLRenderPipelineState);
  }
  return wrappedMTLRenderPipelineState->objc;
}

id_MTLLibrary WrappedMTLDevice::newDefaultLibrary()
{
  id_MTLLibrary realMTLLibrary;
  SERIALISE_TIME_CALL(realMTLLibrary = real_newDefaultLibrary());
  MetalResourceManager::UnwrapHelper<id_MTLLibrary>::Outer *wrappedMTLLibrary;
  ResourceId id = GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
  if(IsCaptureMode(m_State))
  {
    const void *pData;
    uint32_t bytesCount;
    MTL::Get_defaultLibraryData(&pData, &bytesCount);
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newDefaultLibrary);
      Serialise_mtlDevice_newDefaultLibrary(ser, this, wrappedMTLLibrary, pData, bytesCount);
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
  return wrappedMTLLibrary->objc;
}

id_MTLLibrary WrappedMTLDevice::newLibraryWithSource(NSString *source, MTLCompileOptions *options,
                                                     NSError **error)
{
  id_MTLLibrary realMTLLibrary;
  SERIALISE_TIME_CALL(realMTLLibrary = real_newLibraryWithSource(source, options, error));
  MetalResourceManager::UnwrapHelper<id_MTLLibrary>::Outer *wrappedMTLLibrary;
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
  return wrappedMTLLibrary->objc;
}

id_MTLCommandQueue WrappedMTLDevice::newCommandQueue()
{
  id_MTLCommandQueue realMTLCommandQueue;
  SERIALISE_TIME_CALL(realMTLCommandQueue = real_newCommandQueue());
  MetalResourceManager::UnwrapHelper<id_MTLCommandQueue>::Outer *wrappedMTLCommandQueue;
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
  return wrappedMTLCommandQueue->objc;
}

id_MTLTexture WrappedMTLDevice::newTextureWithDescriptor(MTLTextureDescriptor *descriptor)
{
  id_MTLTexture realMTLTexture;
  SERIALISE_TIME_CALL(realMTLTexture = real_newTextureWithDescriptor(descriptor));
  MetalResourceManager::UnwrapHelper<id_MTLTexture>::Outer *wrappedMTLTexture;
  ResourceId id = GetResourceManager()->WrapResource(realMTLTexture, wrappedMTLTexture);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newTextureWithDescriptor);
      Serialise_mtlDevice_newTextureWithDescriptor(ser, this, wrappedMTLTexture, descriptor, NULL,
                                                   (NSUInteger_objc)~0);
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
  return wrappedMTLTexture->objc;
}

id_MTLTexture WrappedMTLDevice::newTextureWithDescriptor(MTLTextureDescriptor *descriptor,
                                                         IOSurfaceRef iosurface, NSUInteger plane)
{
  void *window = (void *)this;
  RenderDoc::Inst().AddFrameCapturer(this, window, this);

  id_MTLTexture realMTLTexture;
  SERIALISE_TIME_CALL(realMTLTexture = real_newTextureWithDescriptor(descriptor, iosurface, plane));
  MetalResourceManager::UnwrapHelper<id_MTLTexture>::Outer *wrappedMTLTexture;
  ResourceId id = GetResourceManager()->WrapResource(realMTLTexture, wrappedMTLTexture);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newTextureWithDescriptor);
      Serialise_mtlDevice_newTextureWithDescriptor(ser, this, wrappedMTLTexture, descriptor,
                                                   iosurface, (NSUInteger_objc)plane);
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
  return wrappedMTLTexture->objc;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newBuffer(SerialiserType &ser, WrappedMTLDevice *device,
                                                     WrappedMTLBuffer *buffer, const void *pointer,
                                                     NSUInteger_objc length,
                                                     MTLResourceOptions_objc options)
{
  SERIALISE_ELEMENT_LOCAL(Buffer, GetResID(buffer)).TypedAs("MTLBuffer"_lit);
  SERIALISE_ELEMENT_ARRAY(pointer, length);
  SERIALISE_ELEMENT(length);
  SERIALISE_ELEMENT(options);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    id_MTLBuffer realMTLBuffer;
    if(pointer)
    {
      realMTLBuffer = real_newBufferWithBytes(pointer, length, options);
    }
    else
    {
      realMTLBuffer = real_newBufferWithLength(length, options);
    }
    MetalResourceManager::UnwrapHelper<id_MTLBuffer>::Outer *wrappedMTLBuffer;
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
    id_MTLLibrary realMTLLibrary = CreateMTLLibrary(pData, bytesCount);
    MetalResourceManager::UnwrapHelper<id_MTLLibrary>::Outer *wrappedMTLLibrary;
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
                                                                NSString *source,
                                                                MTLCompileOptions *options)
{
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);
  SERIALISE_ELEMENT(source);
  // TODO:SERIALISE_ELEMENT(options);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    NSError *error = NULL;
    id_MTLLibrary realMTLLibrary = real_newLibraryWithSource(source, options, &error);
    MetalResourceManager::UnwrapHelper<id_MTLLibrary>::Outer *wrappedMTLLibrary;
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
    MTLRenderPipelineDescriptor *descriptor)
{
  SERIALISE_ELEMENT_LOCAL(RenderPipelineState, GetResID(pipelineState))
      .TypedAs("MTLRenderPipelineState"_lit);
  SERIALISE_ELEMENT(descriptor).TypedAs("MTLRenderPipelineDescriptor");

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    ResourceId liveID;

    id_MTLRenderPipelineState realMTLRenderPipelineState =
        real_newRenderPipelineStateWithDescriptor(descriptor, NULL);
    MetalResourceManager::UnwrapHelper<id_MTLRenderPipelineState>::Outer *wrappedMTLRenderPipelineState;
    liveID = GetResourceManager()->WrapResource(realMTLRenderPipelineState,
                                                wrappedMTLRenderPipelineState);
    GetResourceManager()->AddLiveResource(RenderPipelineState, wrappedMTLRenderPipelineState);
    AddResource(RenderPipelineState, ResourceType::PipelineState, "PipelineState");
    DerivedResource(this, RenderPipelineState);

    MetalCreationInfo::Pipeline &pipeline = m_CreationInfo.m_Pipeline[liveID];
    // Save pipeline settings from MTLRenderPipelineDescriptor
    // id_MTLFunction = MTLRenderPipelineDescriptor::vertexFunction
    // id_MTLFunction = MTLRenderPipelineDescriptor::fragmentFunction
    // TODO: vertexDescriptor : MTLVertexDescriptor
    // TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
    // TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
    const uint32_t colorAttachmentCount = MAX_RENDER_PASS_COLOR_ATTACHMENTS;
    pipeline.attachments.resize(colorAttachmentCount);
    for(uint32_t i = 0; i < colorAttachmentCount; ++i)
    {
      MTLRenderPipelineColorAttachmentDescriptor *mtlAttachment =
          MTL_GET(descriptor, colorAttachment, i);
      MetalCreationInfo::Pipeline::Attachment &attachment = pipeline.attachments[i];
      attachment.pixelFormat = MTL_GET(mtlAttachment, pixelFormat);
      attachment.blendingEnabled = MTL_GET(mtlAttachment, blendingEnabled);
      attachment.sourceRGBBlendFactor = MTL_GET(mtlAttachment, sourceRGBBlendFactor);
      attachment.destinationRGBBlendFactor = MTL_GET(mtlAttachment, destinationRGBBlendFactor);
      attachment.rgbBlendOperation = MTL_GET(mtlAttachment, rgbBlendOperation);
      attachment.sourceAlphaBlendFactor = MTL_GET(mtlAttachment, sourceAlphaBlendFactor);
      attachment.destinationAlphaBlendFactor = MTL_GET(mtlAttachment, destinationAlphaBlendFactor);
      attachment.alphaBlendOperation = MTL_GET(mtlAttachment, alphaBlendOperation);
      attachment.writeMask = MTL_GET(mtlAttachment, writeMask);
    }
    // MTLPixelFormat = MTLRenderPipelineDescriptor::depthAttachmentPixelFormat
    // MTLPixelFormat = MTLRenderPipelineDescriptor::stencilAttachmentPixelFormat
    // NSUInteger = MTLRenderPipelineDescriptor::sampleCount
    pipeline.alphaToCoverageEnabled = MTL_GET(descriptor, alphaToCoverageEnabled);
    pipeline.alphaToOneEnabled = MTL_GET(descriptor, alphaToOneEnabled);
    pipeline.rasterizationEnabled = MTL_GET(descriptor, rasterizationEnabled);
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
    id_MTLCommandQueue realMTLCommandQueue = real_newCommandQueue();
    MetalResourceManager::UnwrapHelper<id_MTLCommandQueue>::Outer *wrappedMTLCommandQueue;
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
    //    id_MTLCommandQueue commandQueue_objc = newCommandQueue();
    //    m_ReplayCommandQueue = GetWrapped(commandQueue_objc);

    GetResourceManager()->AddLiveResource(Device, this);
    AddResource(Device, ResourceType::Device, "Device");
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newTextureWithDescriptor(
    SerialiserType &ser, WrappedMTLDevice *device, WrappedMTLTexture *texture,
    MTLTextureDescriptor *descriptor, IOSurfaceRef iosurface, NSUInteger_objc plane)
{
  SERIALISE_ELEMENT_LOCAL(Texture, GetResID(texture)).TypedAs("MTLTexture"_lit);
  SERIALISE_ELEMENT(descriptor).TypedAs("MTLTextureDescriptor");
  SERIALISE_ELEMENT_LOCAL(IOSurfaceRef, (uint64_t)(void *)iosurface).TypedAs("IOSurfaceRef");
  SERIALISE_ELEMENT(plane);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    id_MTLTexture realMTLTexture = real_newTextureWithDescriptor(descriptor);
    MetalResourceManager::UnwrapHelper<id_MTLTexture>::Outer *wrappedMTLTexture;
    ResourceId liveID = GetResourceManager()->WrapResource(realMTLTexture, wrappedMTLTexture);
    GetResourceManager()->AddLiveResource(Texture, wrappedMTLTexture);

    m_CreationInfo.m_Texture[liveID].Init(GetResourceManager(), m_CreationInfo, descriptor);

    bool inserted = false;
    MetalResources::ImageInfo info;
    info.usage = MTL_GET(descriptor, usage);
    info.extent.width = MTL_GET(descriptor, width);
    info.extent.height = MTL_GET(descriptor, height);
    info.extent.depth = MTL_GET(descriptor, depth);
    info.type = (MTLTextureType)MTL_GET(descriptor, textureType);
    info.layerCount = 0;
    info.levelCount = 0;
    info.sampleCount = 0;
    info.storage = 0;
    id_MTLTexture objCTexture = wrappedMTLTexture->objc;
    InsertImageState(objCTexture, liveID, info, eFrameRef_Unknown, &inserted);

    AddResource(Texture, ResourceType::Texture, "Texture");
    DerivedResource(this, Texture);
  }
  return true;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newDefaultLibrary,
                                WrappedMTLDevice *device, WrappedMTLLibrary *library,
                                const void *pData, uint32_t bytesCount);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newLibraryWithSource,
                                WrappedMTLDevice *device, WrappedMTLLibrary *library,
                                NSString *source, MTLCompileOptions *options);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newCommandQueue,
                                WrappedMTLDevice *device, WrappedMTLCommandQueue *queue);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, MTLCreateSystemDefaultDevice,
                                WrappedMTLDevice *device);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice,
                                mtlDevice_newRenderPipelineStateWithDescriptor,
                                WrappedMTLDevice *device,
                                WrappedMTLRenderPipelineState *renderPipelineState,
                                MTLRenderPipelineDescriptor *descriptor);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newBuffer, WrappedMTLDevice *device,
                                WrappedMTLBuffer *buffer, const void *pointer,
                                NSUInteger_objc length, MTLResourceOptions_objc options);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newTextureWithDescriptor,
                                WrappedMTLDevice *device, WrappedMTLTexture *texture,
                                MTLTextureDescriptor *descriptor, IOSurfaceRef iosurface,
                                NSUInteger_objc plane);
