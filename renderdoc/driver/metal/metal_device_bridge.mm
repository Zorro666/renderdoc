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

#include "metal_device_bridge.h"
#import <Metal/MTLDevice.h>
#include "metal_buffer.h"
#include "metal_buffer_bridge.h"
#include "metal_command_queue.h"
#include "metal_command_queue_bridge.h"
#include "metal_device.h"
#include "metal_function_bridge.h"
#include "metal_library_bridge.h"
#include "metal_render_pipeline_state.h"
#include "metal_render_pipeline_state_bridge.h"

id_MTLDevice WrappedMTLDevice::CreateObjCWrappedMTLDevice()
{
  ObjCWrappedMTLDevice *objCWrappedMTLDevice = [ObjCWrappedMTLDevice alloc];
  objCWrappedMTLDevice.wrappedMTLDevice = this;
  return objCWrappedMTLDevice;
}

id_MTLFunction WrappedMTLDevice::GetVertexFunction(MTLRenderPipelineDescriptor *descriptor)
{
  RDCASSERT([descriptor.vertexFunction isKindOfClass:[ObjCWrappedMTLFunction class]]);
  ObjCWrappedMTLFunction *vertexFunction = (ObjCWrappedMTLFunction *)descriptor.vertexFunction;
  return vertexFunction;
}

id_MTLFunction WrappedMTLDevice::GetFragmentFunction(MTLRenderPipelineDescriptor *descriptor)
{
  RDCASSERT([descriptor.fragmentFunction isKindOfClass:[ObjCWrappedMTLFunction class]]);
  ObjCWrappedMTLFunction *fragmentFunction = (ObjCWrappedMTLFunction *)descriptor.fragmentFunction;
  return fragmentFunction;
}

id_MTLLibrary WrappedMTLDevice::real_newDefaultLibrary()
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLLibrary realMTLLibrary = [realMTLDevice newDefaultLibrary];
  return realMTLLibrary;
}

id_MTLCommandQueue WrappedMTLDevice::real_newCommandQueue()
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLCommandQueue realMTLCommandQueue = [realMTLDevice newCommandQueue];
  return realMTLCommandQueue;
}

id_MTLRenderPipelineState WrappedMTLDevice::real_newRenderPipelineStateWithDescriptor(
    MTLRenderPipelineDescriptor *descriptor, NSError **error)
{
  id_MTLDevice realDevice = Unwrap<id_MTLDevice>(this);

  // TODO descriptor contains wrapped resources : these need to be unwrapped before calling real API
  ObjCWrappedMTLFunction *vertexFunction = (ObjCWrappedMTLFunction *)GetVertexFunction(descriptor);
  id_MTLFunction realVertexFunction = vertexFunction.realMTLFunction;
  ObjCWrappedMTLFunction *fragmentFunction =
      (ObjCWrappedMTLFunction *)GetFragmentFunction(descriptor);
  id_MTLFunction realFragmentFunction = fragmentFunction.realMTLFunction;
  descriptor.vertexFunction = realVertexFunction;
  descriptor.fragmentFunction = realFragmentFunction;

  id_MTLRenderPipelineState realMTLRenderPipelineState =
      [realDevice newRenderPipelineStateWithDescriptor:descriptor error:error];
  return realMTLRenderPipelineState;
}

id_MTLBuffer WrappedMTLDevice::real_newBufferWithBytes(const void *pointer, unsigned int length,
                                                       MTLResourceOptions options)
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLBuffer realMTLBuffer =
      [realMTLDevice newBufferWithBytes:pointer length:length options:options];
  return realMTLBuffer;
}

// Wrapper for MTLDevice
@implementation ObjCWrappedMTLDevice

// ObjCWrappedMTLDevice specific
- (id<MTLDevice>)realMTLDevice
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(self.wrappedMTLDevice);
  return realMTLDevice;
}

// MTLDevice
- (NSString *)name
{
  return self.realMTLDevice.name;
}

- (uint64_t)registryID
{
  return self.realMTLDevice.registryID;
}

- (MTLSize)maxThreadsPerThreadgroup
{
  return self.realMTLDevice.maxThreadsPerThreadgroup;
}

- (BOOL)isLowPower
{
  return self.realMTLDevice.lowPower;
}

- (BOOL)isHeadless
{
  return self.realMTLDevice.headless;
}

- (BOOL)isRemovable
{
  return self.realMTLDevice.removable;
}

- (BOOL)hasUnifiedMemory
{
  return self.realMTLDevice.hasUnifiedMemory;
}

- (uint64_t)recommendedMaxWorkingSetSize
{
  return self.realMTLDevice.recommendedMaxWorkingSetSize;
}

- (MTLDeviceLocation)location
{
  return self.realMTLDevice.location;
}

- (NSUInteger)locationNumber
{
  return self.realMTLDevice.locationNumber;
}

- (uint64_t)maxTransferRate
{
  return self.realMTLDevice.maxTransferRate;
}

- (BOOL)isDepth24Stencil8PixelFormatSupported
{
  return self.realMTLDevice.depth24Stencil8PixelFormatSupported;
}

- (MTLReadWriteTextureTier)readWriteTextureSupport
{
  return self.realMTLDevice.readWriteTextureSupport;
}

- (MTLArgumentBuffersTier)argumentBuffersSupport
{
  return self.realMTLDevice.argumentBuffersSupport;
}

- (BOOL)areRasterOrderGroupsSupported
{
  return self.realMTLDevice.areRasterOrderGroupsSupported;
}

- (BOOL)supports32BitFloatFiltering
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supports32BitFloatFiltering;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)supports32BitMSAA
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supports32BitMSAA;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)supportsQueryTextureLOD
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supportsQueryTextureLOD;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)supportsBCTextureCompression
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supportsBCTextureCompression;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)supportsPullModelInterpolation
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supportsPullModelInterpolation;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)areBarycentricCoordsSupported
{
  return self.realMTLDevice.barycentricCoordsSupported;
}

- (BOOL)supportsShaderBarycentricCoordinates
{
  return self.realMTLDevice.supportsShaderBarycentricCoordinates;
}

- (NSUInteger)currentAllocatedSize
{
  return self.realMTLDevice.currentAllocatedSize;
}

- (NSUInteger)maxThreadgroupMemoryLength
{
  return self.realMTLDevice.maxThreadgroupMemoryLength;
}

- (NSUInteger)maxArgumentBufferSamplerCount
{
  return self.realMTLDevice.maxArgumentBufferSamplerCount;
}

- (BOOL)areProgrammableSamplePositionsSupported
{
  return self.realMTLDevice.programmableSamplePositionsSupported;
}

- (NSUInteger)maxBufferLength
{
  return self.realMTLDevice.maxBufferLength;
}

- (NSArray<id<MTLCounterSet>> *)counterSets
{
  return self.realMTLDevice.counterSets;
}

- (uint64_t)peerGroupID
{
  return self.realMTLDevice.peerGroupID;
}

- (uint32_t)peerIndex
{
  return self.realMTLDevice.peerIndex;
}

- (uint32_t)peerCount
{
  return self.realMTLDevice.peerCount;
}

- (BOOL)supportsDynamicLibraries
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supportsDynamicLibraries;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (NSUInteger)sparseTileSizeInBytes
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.sparseTileSizeInBytes;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (BOOL)supportsRaytracing
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supportsRaytracing;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)supportsFunctionPointers
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLDevice.supportsFunctionPointers;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (nullable id<MTLCommandQueue>)newCommandQueue
{
  return self.wrappedMTLDevice->newCommandQueue();
}

- (nullable id<MTLCommandQueue>)newCommandQueueWithMaxCommandBufferCount:(NSUInteger)maxCommandBufferCount
{
  return [self.realMTLDevice newCommandQueueWithMaxCommandBufferCount:maxCommandBufferCount];
}

- (MTLSizeAndAlign)heapTextureSizeAndAlignWithDescriptor:(MTLTextureDescriptor *)desc
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  return [self.realMTLDevice heapTextureSizeAndAlignWithDescriptor:desc];
}

- (MTLSizeAndAlign)heapBufferSizeAndAlignWithLength:(NSUInteger)length
                                            options:(MTLResourceOptions)options
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  return [self.realMTLDevice heapBufferSizeAndAlignWithLength:length options:options];
}

- (nullable id<MTLHeap>)newHeapWithDescriptor:(MTLHeapDescriptor *)descriptor
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  return [self.realMTLDevice newHeapWithDescriptor:descriptor];
}

- (nullable id<MTLBuffer>)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options
{
  return [self.realMTLDevice newBufferWithLength:length options:options];
}

- (nullable id<MTLBuffer>)newBufferWithBytes:(const void *)pointer
                                      length:(NSUInteger)length
                                     options:(MTLResourceOptions)options
{
  return self.wrappedMTLDevice->newBufferWithBytes(pointer, length, options);
}

- (nullable id<MTLBuffer>)newBufferWithBytesNoCopy:(void *)pointer
                                            length:(NSUInteger)length
                                           options:(MTLResourceOptions)options
                                       deallocator:(void (^__nullable)(void *pointer,
                                                                       NSUInteger length))deallocator
{
  return [self.realMTLDevice newBufferWithBytesNoCopy:pointer
                                               length:length
                                              options:options
                                          deallocator:deallocator];
}

- (nullable id<MTLDepthStencilState>)newDepthStencilStateWithDescriptor:
    (MTLDepthStencilDescriptor *)descriptor
{
  return [self.realMTLDevice newDepthStencilStateWithDescriptor:descriptor];
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
{
  return [self.realMTLDevice newTextureWithDescriptor:descriptor];
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
                                          iosurface:(IOSurfaceRef)iosurface
                                              plane:(NSUInteger)plane
    API_AVAILABLE(macos(10.11), ios(11.0))
{
  return [self.realMTLDevice newTextureWithDescriptor:descriptor iosurface:iosurface plane:plane];
}

- (nullable id<MTLTexture>)newSharedTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
    API_AVAILABLE(macos(10.14), ios(13.0))
{
  return [self.realMTLDevice newSharedTextureWithDescriptor:descriptor];
}

- (nullable id<MTLTexture>)newSharedTextureWithHandle:(MTLSharedTextureHandle *)sharedHandle
    API_AVAILABLE(macos(10.14), ios(13.0))
{
  return [self.realMTLDevice newSharedTextureWithHandle:sharedHandle];
}

- (nullable id<MTLSamplerState>)newSamplerStateWithDescriptor:(MTLSamplerDescriptor *)descriptor
{
  return [self.realMTLDevice newSamplerStateWithDescriptor:descriptor];
}

- (nullable id<MTLLibrary>)newDefaultLibrary
{
  return self.wrappedMTLDevice->newDefaultLibrary();
}

- (nullable id<MTLLibrary>)newDefaultLibraryWithBundle:(NSBundle *)bundle
                                                 error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.12), ios(10.0))
{
  return [self.realMTLDevice newDefaultLibraryWithBundle:bundle error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithFile:(NSString *)filepath
                                        error:(__autoreleasing NSError **)error
{
  return [self.realMTLDevice newLibraryWithFile:filepath error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithURL:(NSURL *)url
                                       error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  return [self.realMTLDevice newLibraryWithURL:url error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithData:(dispatch_data_t)data
                                        error:(__autoreleasing NSError **)error
{
  return [self.realMTLDevice newLibraryWithData:data error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithSource:(NSString *)source
                                        options:(nullable MTLCompileOptions *)options
                                          error:(__autoreleasing NSError **)error
{
  return [self.realMTLDevice newLibraryWithSource:source options:options error:error];
}

- (void)newLibraryWithSource:(NSString *)source
                     options:(nullable MTLCompileOptions *)options
           completionHandler:(MTLNewLibraryCompletionHandler)completionHandler
{
  return [self.realMTLDevice newLibraryWithSource:source
                                          options:options
                                completionHandler:completionHandler];
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                               error:(__autoreleasing NSError **)error
{
  return self.wrappedMTLDevice->newRenderPipelineStateWithDescriptor(descriptor, error);
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                             options:(MTLPipelineOption)options
                          reflection:(MTLAutoreleasedRenderPipelineReflection *__nullable)reflection
                               error:(__autoreleasing NSError **)error
{
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                          options:options
                                                       reflection:reflection
                                                            error:error];
}

- (void)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                           completionHandler:(MTLNewRenderPipelineStateCompletionHandler)completionHandler
{
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                completionHandler:completionHandler];
}

- (void)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                                     options:(MTLPipelineOption)options
                           completionHandler:
                               (MTLNewRenderPipelineStateWithReflectionCompletionHandler)completionHandler
{
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                          options:options
                                                completionHandler:completionHandler];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                              error:(__autoreleasing NSError **)error
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction error:error];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                            options:(MTLPipelineOption)options
                         reflection:(MTLAutoreleasedComputePipelineReflection *__nullable)reflection
                              error:(__autoreleasing NSError **)error
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                                         options:options
                                                      reflection:reflection
                                                           error:error];
}

- (void)newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                          completionHandler:(MTLNewComputePipelineStateCompletionHandler)completionHandler
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                               completionHandler:completionHandler];
}

- (void)newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                                    options:(MTLPipelineOption)options
                          completionHandler:
                              (MTLNewComputePipelineStateWithReflectionCompletionHandler)completionHandler
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                                         options:options
                                               completionHandler:completionHandler];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor *)descriptor
                              options:(MTLPipelineOption)options
                           reflection:(MTLAutoreleasedComputePipelineReflection *__nullable)reflection
                                error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  return [self.realMTLDevice newComputePipelineStateWithDescriptor:descriptor
                                                           options:options
                                                        reflection:reflection
                                                             error:error];
}

- (void)newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor *)descriptor
                                      options:(MTLPipelineOption)options
                            completionHandler:
                                (MTLNewComputePipelineStateWithReflectionCompletionHandler)completionHandler
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  return [self.realMTLDevice newComputePipelineStateWithDescriptor:descriptor
                                                           options:options
                                                 completionHandler:completionHandler];
}

- (nullable id<MTLFence>)newFence API_AVAILABLE(macos(10.13), ios(10.0))
{
  return [self.realMTLDevice newFence];
}

- (BOOL)supportsFeatureSet:(MTLFeatureSet)featureSet
{
  return [self.realMTLDevice supportsFeatureSet:featureSet];
}

- (BOOL)supportsFamily:(MTLGPUFamily)gpuFamily API_AVAILABLE(macos(10.15), ios(13.0))
{
  return [self.realMTLDevice supportsFamily:gpuFamily];
}

- (BOOL)supportsTextureSampleCount:(NSUInteger)sampleCount API_AVAILABLE(macos(10.11), ios(9.0))
{
  return [self.realMTLDevice supportsTextureSampleCount:sampleCount];
}

- (NSUInteger)minimumLinearTextureAlignmentForPixelFormat:(MTLPixelFormat)format
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  return [self.realMTLDevice minimumLinearTextureAlignmentForPixelFormat:format];
}

- (NSUInteger)minimumTextureBufferAlignmentForPixelFormat:(MTLPixelFormat)format
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  return [self.realMTLDevice minimumTextureBufferAlignmentForPixelFormat:format];
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithTileDescriptor:(MTLTileRenderPipelineDescriptor *)descriptor
                                 options:(MTLPipelineOption)options
                              reflection:(MTLAutoreleasedRenderPipelineReflection *__nullable)reflection
                                   error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0))API_UNAVAILABLE(tvos)
{
  return [self.realMTLDevice newRenderPipelineStateWithTileDescriptor:descriptor
                                                              options:options
                                                           reflection:reflection
                                                                error:error];
}

- (void)newRenderPipelineStateWithTileDescriptor:(MTLTileRenderPipelineDescriptor *)descriptor
                                         options:(MTLPipelineOption)options
                               completionHandler:
                                   (MTLNewRenderPipelineStateWithReflectionCompletionHandler)completionHandler
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0))API_UNAVAILABLE(tvos)
{
  return [self.realMTLDevice newRenderPipelineStateWithTileDescriptor:descriptor
                                                              options:options
                                                    completionHandler:completionHandler];
}

- (void)getDefaultSamplePositions:(MTLSamplePosition *)positions
                            count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(11.0))
{
  return [self.realMTLDevice getDefaultSamplePositions:positions count:count];
}

- (nullable id<MTLArgumentEncoder>)newArgumentEncoderWithArguments:
    (NSArray<MTLArgumentDescriptor *> *)arguments API_AVAILABLE(macos(10.13), ios(11.0))
{
  return [self.realMTLDevice newArgumentEncoderWithArguments:arguments];
}

- (BOOL)supportsRasterizationRateMapWithLayerCount:(NSUInteger)layerCount
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  return [self.realMTLDevice supportsRasterizationRateMapWithLayerCount:layerCount];
}

- (nullable id<MTLRasterizationRateMap>)newRasterizationRateMapWithDescriptor:
    (MTLRasterizationRateMapDescriptor *)descriptor
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  return [self.realMTLDevice newRasterizationRateMapWithDescriptor:descriptor];
}

- (nullable id<MTLIndirectCommandBuffer>)
newIndirectCommandBufferWithDescriptor:(MTLIndirectCommandBufferDescriptor *)descriptor
                       maxCommandCount:(NSUInteger)maxCount
                               options:(MTLResourceOptions)options
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  return [self.realMTLDevice newIndirectCommandBufferWithDescriptor:descriptor
                                                    maxCommandCount:maxCount
                                                            options:options];
}

- (nullable id<MTLEvent>)newEvent API_AVAILABLE(macos(10.14), ios(12.0))
{
  return [self.realMTLDevice newEvent];
}

- (nullable id<MTLSharedEvent>)newSharedEvent API_AVAILABLE(macos(10.14), ios(12.0))
{
  return [self.realMTLDevice newSharedEvent];
}

- (nullable id<MTLSharedEvent>)newSharedEventWithHandle:(MTLSharedEventHandle *)sharedEventHandle
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  return [self.realMTLDevice newSharedEventWithHandle:sharedEventHandle];
}

- (MTLSize)sparseTileSizeWithTextureType:(MTLTextureType)textureType
                             pixelFormat:(MTLPixelFormat)pixelFormat
                             sampleCount:(NSUInteger)sampleCount
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  return [self.realMTLDevice sparseTileSizeWithTextureType:textureType
                                               pixelFormat:pixelFormat
                                               sampleCount:sampleCount];
}

- (void)convertSparsePixelRegions:(const MTLRegion[_Nonnull])pixelRegions
                    toTileRegions:(MTLRegion[_Nonnull])tileRegions
                     withTileSize:(MTLSize)tileSize
                    alignmentMode:(MTLSparseTextureRegionAlignmentMode)mode
                       numRegions:(NSUInteger)numRegions
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  return [self.realMTLDevice convertSparsePixelRegions:pixelRegions
                                         toTileRegions:tileRegions
                                          withTileSize:tileSize
                                         alignmentMode:mode
                                            numRegions:numRegions];
}

- (void)convertSparseTileRegions:(const MTLRegion[_Nonnull])tileRegions
                  toPixelRegions:(MTLRegion[_Nonnull])pixelRegions
                    withTileSize:(MTLSize)tileSize
                      numRegions:(NSUInteger)numRegions
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  return [self.realMTLDevice convertSparseTileRegions:tileRegions
                                       toPixelRegions:pixelRegions
                                         withTileSize:tileSize
                                           numRegions:numRegions];
}

- (nullable id<MTLCounterSampleBuffer>)newCounterSampleBufferWithDescriptor:
                                           (MTLCounterSampleBufferDescriptor *)descriptor
                                                                      error:(NSError **)error
    API_AVAILABLE(macos(10.15), ios(14.0))
{
  return [self.realMTLDevice newCounterSampleBufferWithDescriptor:descriptor error:error];
}

- (void)sampleTimestamps:(MTLTimestamp *)cpuTimestamp
            gpuTimestamp:(MTLTimestamp *)gpuTimestamp API_AVAILABLE(macos(10.15), ios(14.0))
{
  return [self.realMTLDevice sampleTimestamps:cpuTimestamp gpuTimestamp:gpuTimestamp];
}

- (BOOL)supportsCounterSampling:(MTLCounterSamplingPoint)samplingPoint
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLDevice supportsCounterSampling:samplingPoint];
}

- (BOOL)supportsVertexAmplificationCount:(NSUInteger)count
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  return [self.realMTLDevice supportsVertexAmplificationCount:count];
}

- (nullable id<MTLDynamicLibrary>)newDynamicLibrary:(id<MTLLibrary>)library
                                              error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLDevice newDynamicLibrary:library error:error];
}

- (nullable id<MTLDynamicLibrary>)newDynamicLibraryWithURL:(NSURL *)url
                                                     error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLDevice newDynamicLibraryWithURL:url error:error];
}

- (nullable id<MTLBinaryArchive>)newBinaryArchiveWithDescriptor:(MTLBinaryArchiveDescriptor *)descriptor
                                                          error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLDevice newBinaryArchiveWithDescriptor:descriptor error:error];
}

- (MTLAccelerationStructureSizes)accelerationStructureSizesWithDescriptor:
    (MTLAccelerationStructureDescriptor *)descriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLDevice accelerationStructureSizesWithDescriptor:descriptor];
}

- (nullable id<MTLAccelerationStructure>)newAccelerationStructureWithSize:(NSUInteger)size
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLDevice newAccelerationStructureWithSize:size];
}

- (nullable id<MTLAccelerationStructure>)newAccelerationStructureWithDescriptor:
    (MTLAccelerationStructureDescriptor *)descriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLDevice newAccelerationStructureWithDescriptor:descriptor];
}

@end
