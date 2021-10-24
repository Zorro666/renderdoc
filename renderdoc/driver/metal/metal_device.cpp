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
#include "metal_buffer.h"
#include "metal_command_queue.h"
#include "metal_common.h"
#include "metal_core.h"
#include "metal_device.h"
#include "metal_manager.h"
#include "metal_render_pipeline_state.h"
#include "metal_replay.h"

WrappedMTLDevice::WrappedMTLDevice()
{
  Construct();
}

WrappedMTLDevice::WrappedMTLDevice(id_MTLDevice realMTLDevice, ResourceId objId)
    : WrappedMTLObject(realMTLDevice, objId, this)
{
  m_ObjCWrappedMTLDevice = CreateObjCWrappedMTLDevice();
  Construct();
  GetResourceManager()->AddCurrentResource(objId, this);
}

void WrappedMTLDevice::Construct()
{
  m_WrappedMTLDevice = this;
  if(RenderDoc::Inst().IsReplayApp())
  {
    m_State = CaptureState::LoadingReplaying;
    m_DummyReplayLibrary = new WrappedMTLLibrary(this);
    m_DummyReplayCommandQueue = new WrappedMTLCommandQueue(this);
    m_DummyReplayCommandBuffer = new WrappedMTLCommandBuffer(this);
    m_DummyReplayCommandBuffer->SetWrappedMTLCommandQueue(m_DummyReplayCommandQueue);
    m_DummyReplayRenderCommandEncoder = new WrappedMTLRenderCommandEncoder(this);
    m_DummyReplayRenderCommandEncoder->SetWrappedMTLCommandBuffer(m_DummyReplayCommandBuffer);
  }
  else
  {
    m_State = CaptureState::BackgroundCapturing;
    m_DummyReplayLibrary = NULL;
    m_DummyReplayCommandQueue = NULL;
    m_DummyReplayCommandBuffer = NULL;
    m_DummyReplayRenderCommandEncoder = NULL;
  }

  m_SectionVersion = MetalInitParams::CurrentVersion;

  threadSerialiserTLSSlot = Threading::AllocateTLSSlot();

  m_ResourceManager = new MetalResourceManager(m_State, this);

  m_Replay = new MetalReplay();

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
  ResourceId objId = ResourceIDGen::GetNewUniqueID();
  WrappedMTLDevice *wrappedMTLDevice = new WrappedMTLDevice(realMTLDevice, objId);

  void *window = (void *)wrappedMTLDevice;
  RenderDoc::Inst().AddFrameCapturer(wrappedMTLDevice, window, wrappedMTLDevice);

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
  }

  FirstFrame();

  return GetObjCWrappedMTLDevice();
}

id_MTLBuffer WrappedMTLDevice::newBufferWithBytes(const void *pointer, unsigned int length,
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
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlDevice_newBufferWithBytes);
      Serialise_mtlDevice_newBufferWithBytes(ser, this, wrappedMTLBuffer);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLBuffer);
    record->AddChunk(chunk);
    GetResourceManager()->MarkResourceFrameReferenced(id, eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
  return wrappedMTLBuffer->GetObjCWrappedMTLBuffer();
}

id_MTLRenderPipelineState WrappedMTLDevice::newRenderPipelineStateWithDescriptor(
    MTLRenderPipelineDescriptor *descriptor, NSError **error)
{
  id_MTLRenderPipelineState realMTLRenderPipelineState;
  // TODO descriptor contains wrapped resources : these need to be unwrapped before calling real API
  id_MTLFunction vertexFunction = GetVertexFunction(descriptor);
  WrappedMTLFunction *wrappedVertexFunction = GetWrappedFromObjC(vertexFunction);
  ResourceId vertexFunctionId = GetResID(wrappedVertexFunction);
  id_MTLFunction fragmentFunction = GetFragmentFunction(descriptor);
  WrappedMTLFunction *wrappedFragmentFunction = GetWrappedFromObjC(fragmentFunction);
  ResourceId fragmentFunctionId = GetResID(wrappedFragmentFunction);
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
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlDevice_newRenderPipelineStateWithDescriptor);
      // TODO Serialise "descriptor"
      Serialise_mtlDevice_newRenderPipelineStateWithDescriptor(ser, this,
                                                               wrappedMTLRenderPipelineState);
      chunk = scope.Get();
    }
    MetalResourceRecord *record =
        GetResourceManager()->AddResourceRecord(wrappedMTLRenderPipelineState);
    record->AddChunk(chunk);

    GetResourceManager()->MarkResourceFrameReferenced(vertexFunctionId, eFrameRef_Read);
    GetResourceManager()->MarkResourceFrameReferenced(fragmentFunctionId, eFrameRef_Read);
    record->AddParent(GetRecord(wrappedVertexFunction));
    record->AddParent(GetRecord(wrappedFragmentFunction));
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLRenderPipelineState);
  }
  return wrappedMTLRenderPipelineState->GetObjCWrappedMTLRenderPipelineState();
}

id_MTLLibrary WrappedMTLDevice::newDefaultLibrary()
{
  id_MTLLibrary realMTLLibrary;
  SERIALISE_TIME_CALL(realMTLLibrary = real_newDefaultLibrary());
  MetalResourceManager::UnwrapHelper<id_MTLLibrary>::Outer *wrappedMTLLibrary;
  ResourceId id = GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlDevice_newDefaultLibrary);
      Serialise_mtlDevice_newDefaultLibrary(ser, this, wrappedMTLLibrary);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLLibrary);
    record->AddChunk(chunk);
    GetResourceManager()->MarkResourceFrameReferenced(id, eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
  return wrappedMTLLibrary->GetObjCWrappedMTLLibrary();
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
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlDevice_newCommandQueue);
      Serialise_mtlDevice_newCommandQueue(ser, this, wrappedMTLCommandQueue);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLCommandQueue);
    record->AddChunk(chunk);
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLLibrary);
  }
  return wrappedMTLCommandQueue->GetObjCWrappedMTLCommandQueue();
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newBufferWithBytes(SerialiserType &ser,
                                                              WrappedMTLDevice *device,
                                                              WrappedMTLBuffer *buffer)
{
  SERIALISE_ELEMENT_LOCAL(Device, GetResID(device)).TypedAs("MTLDevice"_lit);
  SERIALISE_ELEMENT_LOCAL(Buffer, GetResID(buffer)).TypedAs("MTLBuffer"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newDefaultLibrary(SerialiserType &ser,
                                                             WrappedMTLDevice *device,
                                                             WrappedMTLLibrary *library)
{
  SERIALISE_ELEMENT_LOCAL(Device, GetResID(device)).TypedAs("MTLDevice"_lit);
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newRenderPipelineStateWithDescriptor(
    SerialiserType &ser, WrappedMTLDevice *device, WrappedMTLRenderPipelineState *pipelineState)
{
  SERIALISE_ELEMENT_LOCAL(Device, GetResID(device)).TypedAs("MTLDevice"_lit);
  SERIALISE_ELEMENT_LOCAL(RenderPipelineState, GetResID(pipelineState))
      .TypedAs("MTLRenderPipelineState"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_mtlDevice_newCommandQueue(SerialiserType &ser,
                                                           WrappedMTLDevice *device,
                                                           WrappedMTLCommandQueue *queue)
{
  SERIALISE_ELEMENT_LOCAL(Device, GetResID(device)).TypedAs("MTLDevice"_lit);
  SERIALISE_ELEMENT_LOCAL(CommandQueue, GetResID(queue)).TypedAs("MTLCommandQueue"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_MTLCreateSystemDefaultDevice(SerialiserType &ser,
                                                              WrappedMTLDevice *device)
{
  SERIALISE_ELEMENT_LOCAL(Device, GetResID(device)).TypedAs("MTLDevice"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newDefaultLibrary,
                                WrappedMTLDevice *device, WrappedMTLLibrary *library);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newCommandQueue,
                                WrappedMTLDevice *device, WrappedMTLCommandQueue *queue);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, MTLCreateSystemDefaultDevice,
                                WrappedMTLDevice *device);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice,
                                mtlDevice_newRenderPipelineStateWithDescriptor,
                                WrappedMTLDevice *device,
                                WrappedMTLRenderPipelineState *renderPipelineState);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLDevice, mtlDevice_newBufferWithBytes,
                                WrappedMTLDevice *device, WrappedMTLBuffer *buffer);
