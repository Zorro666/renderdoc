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
#include "metal_command_queue.h"
#include "metal_function.h"
#include "metal_helpers_bridge.h"
#include "metal_library.h"
#include "metal_manager.h"
#include "metal_render_pipeline_state.h"
#include "metal_texture.h"

WrappedMTLDevice::WrappedMTLDevice(MTL::Device *realMTLDevice, ResourceId objId)
    : WrappedMTLObject(realMTLDevice, objId, this, GetStateRef())
{
  m_ObjcBridge = AllocateObjCBridge(this);
  m_WrappedMTLDevice = this;
  threadSerialiserTLSSlot = Threading::AllocateTLSSlot();

  m_ResourceManager = new MetalResourceManager(m_State, this);
  RDCASSERT(m_WrappedMTLDevice == this);
  GetResourceManager()->AddCurrentResource(objId, this);
}

WrappedMTLDevice *WrappedMTLDevice::MTLCreateSystemDefaultDevice(MTL::Device *realMTLDevice)
{
  MTLFixupForMetalDriverAssert();
  ResourceId objId = ResourceIDGen::GetNewUniqueID();
  WrappedMTLDevice *wrappedMTLDevice = new WrappedMTLDevice(realMTLDevice, objId);

  return wrappedMTLDevice;
}

// Serialised MTLDevice APIs

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_newCommandQueue(SerialiserType &ser, WrappedMTLCommandQueue *queue)
{
  SERIALISE_ELEMENT_LOCAL(Device, this);
  SERIALISE_ELEMENT_LOCAL(CommandQueue, GetResID(queue)).TypedAs("MTLCommandQueue"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    // TODO: implement RD MTL replay
  }
  return true;
}

WrappedMTLCommandQueue *WrappedMTLDevice::newCommandQueue()
{
  MTL::CommandQueue *realMTLCommandQueue;
  SERIALISE_TIME_CALL(realMTLCommandQueue = Unwrap(this)->newCommandQueue());
  WrappedMTLCommandQueue *wrappedMTLCommandQueue;
  ResourceId id = GetResourceManager()->WrapResource(realMTLCommandQueue, wrappedMTLCommandQueue);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newCommandQueue);
      Serialise_newCommandQueue(ser, wrappedMTLCommandQueue);
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
  return wrappedMTLCommandQueue;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_newDefaultLibrary(SerialiserType &ser, WrappedMTLLibrary *library)
{
  bytebuf data;
  if(ser.IsWriting())
  {
    ObjC::Get_defaultLibraryData(data);
  }

  SERIALISE_ELEMENT_LOCAL(Device, this);
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);
  SERIALISE_ELEMENT(data);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    // TODO: implement RD MTL replay
  }
  return true;
}

WrappedMTLLibrary *WrappedMTLDevice::newDefaultLibrary()
{
  MTL::Library *realMTLLibrary;

  SERIALISE_TIME_CALL(realMTLLibrary = Unwrap(this)->newDefaultLibrary());
  WrappedMTLLibrary *wrappedMTLLibrary;
  ResourceId id = GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newDefaultLibrary);
      Serialise_newDefaultLibrary(ser, wrappedMTLLibrary);
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
  return wrappedMTLLibrary;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_newLibraryWithSource(SerialiserType &ser,
                                                      WrappedMTLLibrary *library, NS::String *source,
                                                      MTL::CompileOptions *options, NS::Error **error)
{
  SERIALISE_ELEMENT_LOCAL(Device, this);
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);
  SERIALISE_ELEMENT(source);
  // TODO:SERIALISE_ELEMENT(options);

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    // TODO: implement RD MTL replay
  }
  return true;
}

WrappedMTLLibrary *WrappedMTLDevice::newLibraryWithSource(NS::String *source,
                                                          MTL::CompileOptions *options,
                                                          NS::Error **error)
{
  MTL::Library *realMTLLibrary;
  SERIALISE_TIME_CALL(realMTLLibrary = Unwrap(this)->newLibrary(source, options, error));
  WrappedMTLLibrary *wrappedMTLLibrary;
  ResourceId id = GetResourceManager()->WrapResource(realMTLLibrary, wrappedMTLLibrary);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newLibraryWithSource);
      Serialise_newLibraryWithSource(ser, wrappedMTLLibrary, source, options, error);
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
  return wrappedMTLLibrary;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_newRenderPipelineStateWithDescriptor(
    SerialiserType &ser, WrappedMTLRenderPipelineState *pipelineState,
    MTL::RenderPipelineDescriptor *descriptor, NS::Error **error)
{
  SERIALISE_ELEMENT_LOCAL(RenderPipelineState, GetResID(pipelineState))
      .TypedAs("MTLRenderPipelineState"_lit);
  SERIALISE_ELEMENT(descriptor);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

WrappedMTLRenderPipelineState *WrappedMTLDevice::newRenderPipelineStateWithDescriptor(
    MTL::RenderPipelineDescriptor *descriptor, NS::Error **error)
{
  MTL::RenderPipelineDescriptor *realDescriptor = descriptor->copy();

  // The source descriptor contains objc bridge MTLFunction resources
  // The clone needs the real resources
  MTL::Function *objcBridgeVertexFunction = descriptor->vertexFunction();
  if(objcBridgeVertexFunction != NULL)
  {
    realDescriptor->setVertexFunction(GetReal(objcBridgeVertexFunction));
  }

  MTL::Function *objcBridgeFragmentFunction = descriptor->fragmentFunction();
  if(objcBridgeFragmentFunction != NULL)
  {
    realDescriptor->setFragmentFunction(GetReal(objcBridgeFragmentFunction));
  }

  MTL::RenderPipelineState *realMTLRenderPipelineState;
  SERIALISE_TIME_CALL(realMTLRenderPipelineState =
                          Unwrap(this)->newRenderPipelineState(realDescriptor, error));
  realDescriptor->release();

  WrappedMTLRenderPipelineState *wrappedMTLRenderPipelineState;
  ResourceId id =
      GetResourceManager()->WrapResource(realMTLRenderPipelineState, wrappedMTLRenderPipelineState);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLDevice_newRenderPipelineStateWithDescriptor);
      Serialise_newRenderPipelineStateWithDescriptor(ser, wrappedMTLRenderPipelineState, descriptor,
                                                     error);
      chunk = scope.Get();
    }

    MetalResourceRecord *record =
        GetResourceManager()->AddResourceRecord(wrappedMTLRenderPipelineState);
    record->AddChunk(chunk);
    record->AddParent(GetRecord(GetWrapped(objcBridgeVertexFunction)));
    record->AddParent(GetRecord(GetWrapped(objcBridgeFragmentFunction)));
  }
  else
  {
    // TODO: implement RD MTL replay
    //     GetResourceManager()->AddLiveResource(id, *wrappedMTLRenderPipelineState);
  }
  return wrappedMTLRenderPipelineState;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_newTextureWithDescriptor(SerialiserType &ser,
                                                          WrappedMTLTexture *texture,
                                                          MTL::TextureDescriptor *descriptor,
                                                          bool frameBufferOnly, bool hasIoSurface)
{
  SERIALISE_ELEMENT_LOCAL(Texture, GetResID(texture)).TypedAs("MTLTexture"_lit);
  SERIALISE_ELEMENT(descriptor);
  SERIALISE_ELEMENT(frameBufferOnly).Hidden();
  SERIALISE_ELEMENT(hasIoSurface).Hidden();

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
  }
  return true;
}

WrappedMTLTexture *WrappedMTLDevice::newTextureWithDescriptor(MTL::TextureDescriptor *descriptor)
{
  MTL::Texture *realMTLTexture;
  SERIALISE_TIME_CALL(realMTLTexture = Unwrap(this)->newTexture(descriptor));
  WrappedMTLTexture *wrappedMTLTexture = NewTexture(realMTLTexture, descriptor, false);
  return wrappedMTLTexture;
}

WrappedMTLTexture *WrappedMTLDevice::newTextureWithDescriptor(MTL::TextureDescriptor *descriptor,
                                                              IOSurfaceRef iosurface,
                                                              NS::UInteger plane)
{
  MTL::Texture *realMTLTexture;
  SERIALISE_TIME_CALL(realMTLTexture = Unwrap(this)->newTexture(descriptor, iosurface, plane));
  WrappedMTLTexture *wrappedMTLTexture = NewTexture(realMTLTexture, descriptor, true);
  if(IsCaptureMode(m_State))
  {
    {
      SCOPED_LOCK(m_PotentialBackBuffersLock);
      m_PotentialBackBuffers.insert(wrappedMTLTexture);
    }
  }
  return wrappedMTLTexture;
}

// Non-Serialised MTLDevice APIs

bool WrappedMTLDevice::isDepth24Stencil8PixelFormatSupported()
{
  return Unwrap(this)->depth24Stencil8PixelFormatSupported();
}

MTL::ReadWriteTextureTier WrappedMTLDevice::readWriteTextureSupport()
{
  return Unwrap(this)->readWriteTextureSupport();
}

MTL::ArgumentBuffersTier WrappedMTLDevice::argumentBuffersSupport()
{
  return Unwrap(this)->argumentBuffersSupport();
}

bool WrappedMTLDevice::areRasterOrderGroupsSupported()
{
  return Unwrap(this)->rasterOrderGroupsSupported();
}

bool WrappedMTLDevice::supports32BitFloatFiltering()
{
  return Unwrap(this)->supports32BitFloatFiltering();
}

bool WrappedMTLDevice::supports32BitMSAA()
{
  return Unwrap(this)->supports32BitMSAA();
}

bool WrappedMTLDevice::supportsQueryTextureLOD()
{
  return Unwrap(this)->supportsQueryTextureLOD();
}

bool WrappedMTLDevice::supportsBCTextureCompression()
{
  return Unwrap(this)->supportsBCTextureCompression();
}

bool WrappedMTLDevice::supportsPullModelInterpolation()
{
  return Unwrap(this)->supportsPullModelInterpolation();
}

bool WrappedMTLDevice::areBarycentricCoordsSupported()
{
  return Unwrap(this)->barycentricCoordsSupported();
}

bool WrappedMTLDevice::supportsShaderBarycentricCoordinates()
{
  return Unwrap(this)->supportsShaderBarycentricCoordinates();
}

bool WrappedMTLDevice::supportsFeatureSet(MTL::FeatureSet featureSet)
{
  return Unwrap(this)->supportsFeatureSet(featureSet);
}

bool WrappedMTLDevice::supportsFamily(MTL::GPUFamily gpuFamily)
{
  return Unwrap(this)->supportsFamily(gpuFamily);
}

bool WrappedMTLDevice::supportsTextureSampleCount(NS::UInteger sampleCount)
{
  return Unwrap(this)->supportsTextureSampleCount(sampleCount);
}

bool WrappedMTLDevice::areProgrammableSamplePositionsSupported()
{
  return Unwrap(this)->programmableSamplePositionsSupported();
}

bool WrappedMTLDevice::supportsRasterizationRateMapWithLayerCount(NS::UInteger layerCount)
{
  return Unwrap(this)->supportsRasterizationRateMap(layerCount);
}

bool WrappedMTLDevice::supportsCounterSampling(MTL::CounterSamplingPoint samplingPoint)
{
  return Unwrap(this)->supportsCounterSampling(samplingPoint);
}

bool WrappedMTLDevice::supportsVertexAmplificationCount(NS::UInteger count)
{
  return Unwrap(this)->supportsVertexAmplificationCount(count);
}

bool WrappedMTLDevice::supportsDynamicLibraries()
{
  return Unwrap(this)->supportsDynamicLibraries();
}

bool WrappedMTLDevice::supportsRenderDynamicLibraries()
{
  return Unwrap(this)->supportsRenderDynamicLibraries();
}

bool WrappedMTLDevice::supportsRaytracing()
{
  // RD device does not support ray tracing
  return false;
}

bool WrappedMTLDevice::supportsFunctionPointers()
{
  return Unwrap(this)->supportsFunctionPointers();
}

bool WrappedMTLDevice::supportsFunctionPointersFromRender()
{
  return Unwrap(this)->supportsFunctionPointersFromRender();
}

bool WrappedMTLDevice::supportsRaytracingFromRender()
{
  // RD device does not support ray tracing
  return false;
}

bool WrappedMTLDevice::supportsPrimitiveMotionBlur()
{
  return Unwrap(this)->supportsPrimitiveMotionBlur();
}

// End of MTLDevice APIs

WrappedMTLTexture *WrappedMTLDevice::NewTexture(MTL::Texture *realMTLTexture,
                                                MTL::TextureDescriptor *descriptor, bool hasIoSurface)
{
  WrappedMTLTexture *wrappedMTLTexture;
  ResourceId id = GetResourceManager()->WrapResource(realMTLTexture, wrappedMTLTexture);
  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(hasIoSurface ? MetalChunk::MTLDevice_newTextureWithDescriptor_iosurface
                                          : MetalChunk::MTLDevice_newTextureWithDescriptor);
      bool frameBufferOnly = realMTLTexture->framebufferOnly();
      Serialise_newTextureWithDescriptor(ser, wrappedMTLTexture, descriptor, frameBufferOnly,
                                         hasIoSurface);
      chunk = scope.Get();
    }
    MetalResourceRecord *textureRecord = GetResourceManager()->AddResourceRecord(wrappedMTLTexture);
    textureRecord->AddChunk(chunk);
  }
  return wrappedMTLTexture;
}

INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLDevice, WrappedMTLCommandQueue *,
                                            newCommandQueue);
INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLDevice, WrappedMTLLibrary *, newDefaultLibrary);
INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLDevice, WrappedMTLLibrary *,
                                            newLibraryWithSource, NS::String *source,
                                            MTL::CompileOptions *options, NS::Error **error);
INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLDevice,
                                            WrappedMTLRenderPipelineState *renderPipelineState,
                                            newRenderPipelineStateWithDescriptor,
                                            MTL::RenderPipelineDescriptor *descriptor,
                                            NS::Error **error);
INSTANTIATE_FUNCTION_WITH_RETURN_SERIALISED(WrappedMTLDevice, WrappedMTLTexture *,
                                            newTextureWithDescriptor,
                                            MTL::TextureDescriptor *descriptor,
                                            bool frameBufferOnly, bool ioSurface);
