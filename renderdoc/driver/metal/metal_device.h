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
#include "metal_manager.h"

class MetalCapturer;

class WrappedMTLDevice : public WrappedMTLObject
{
  friend class MetalResourceManager;

public:
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

  template <typename SerialiserType>
  bool Serialise_CaptureScope(SerialiserType &ser);
  template <typename SerialiserType>
  bool Serialise_BeginCaptureFrame(SerialiserType &ser);
  void EndCaptureFrame(ResourceId backbuffer);

  bool Prepare_InitialState(WrappedMTLObject *res);
  uint64_t GetSize_InitialState(ResourceId id, const MetalInitialContents &initial);
  template <typename SerialiserType>
  bool Serialise_InitialState(SerialiserType &ser, ResourceId id, MetalResourceRecord *record,
                              const MetalInitialContents *initial);
  void Create_InitialState(ResourceId id, WrappedMTLObject *live, bool hasData);
  void Apply_InitialState(WrappedMTLObject *live, const MetalInitialContents &initial);

  void WaitForGPU();

  WrappedMTLTexture *Common_NewTexture(RDMTL::TextureDescriptor &descriptor, MetalChunk chunkType,
                                       bool ioSurfaceTexture, IOSurfaceRef iosurface,
                                       NS::UInteger plane);
  WrappedMTLBuffer *Common_NewBuffer(bool withBytes, const void *pointer, NS::UInteger length,
                                     MTL::ResourceOptions options);

  MetalResourceManager *m_ResourceManager;
  ResourceId m_LastPresentedImage;

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
  PerformanceTimer m_CaptureTimer;
  uint32_t m_FrameCounter = 0;
  rdcarray<FrameDescription> m_CapturedFrames;
  Threading::RWLock m_CapTransitionLock;
  MetalResourceRecord *m_FrameCaptureRecord;
  Chunk *m_HeaderChunk;

  MetalInitParams m_InitParams;
  uint64_t m_SectionVersion;

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
