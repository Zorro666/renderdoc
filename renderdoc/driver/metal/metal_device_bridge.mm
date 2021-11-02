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
#import <AppKit/AppKit.h>
#import <objc/runtime.h>
#include "metal_device.h"
#include "metal_function_bridge.h"
#include "metal_texture_bridge.h"

static bool s_fixupMetalDriverAssert = false;

// helper defined in vk_apple_helpers.mm
void getMetalLayerSize(void *layerHandle, int &width, int &height);

id_MTLTexture MTL::Get_texture(id_CAMetalDrawable drawable)
{
  // TODO: this is likely to be an unwrapped texture
  id_MTLTexture texture = [drawable texture];
  RDCASSERT([texture isKindOfClass:[ObjCWrappedMTLTexture class]]);
  return texture;
}

void MTL::ReleaseDrawable(id_CAMetalDrawable &drawable)
{
  if(drawable == nil)
    return;
  [drawable release];
  drawable = nil;
}

void MTL::Get_defaultLibraryData(const void **pData, uint32_t *bytesCount)
{
  NSBundle *mainAppBundle = [NSBundle mainBundle];
  NSString *defaultLibaryPath = [mainAppBundle pathForResource:@"default" ofType:@"metallib"];
  NSData *myData = [NSData dataWithContentsOfFile:defaultLibaryPath];
  dispatch_data_t data = dispatch_data_create(
      myData.bytes, myData.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  NSData *nsData = (NSData *)data;
  *pData = nsData.bytes;
  *bytesCount = (uint32_t)nsData.length;
}

void MTL::Get_LayerSize(void *layerHandle, int &width, int &height)
{
  ::getMetalLayerSize(layerHandle, width, height);
}

void MTL::Set_LayerSize(void *layerHandle, int w, int h)
{
  CAMetalLayer *layer = (CAMetalLayer *)layerHandle;
  assert([layer isKindOfClass:[CAMetalLayer class]]);

  CGSize cgSize;
  cgSize.width = w;
  cgSize.height = h;
  layer.drawableSize = cgSize;
}

void WrappedMTLDevice::FixupForMetalDriverAssert()
{
  /*
    if(RenderDoc::Inst().IsReplayApp())
      return;
  */

  if(s_fixupMetalDriverAssert)
    return;

#if ENABLED(RDOC_DEVEL)
  NSLog(@"Fixup for Metal Driver debug assert. Adding protocol `MTLTextureImplementation` to "
        @"`ObjCWrappedMTLTexture`");
  class_addProtocol([ObjCWrappedMTLTexture class], objc_getProtocol("MTLTextureImplementation"));
#endif
  s_fixupMetalDriverAssert = true;
}

id_CAMetalDrawable WrappedMTLDevice::GetNextDrawable(void *layer)
{
  CAMetalLayer *metalLayer = (CAMetalLayer *)layer;
  RDCASSERT([metalLayer isKindOfClass:[CAMetalLayer class]]);
  if(metalLayer.device == NULL)
    metalLayer.device = (id_MTLDevice)objc;
  metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  metalLayer.framebufferOnly = NO;
  metalLayer.allowsNextDrawableTimeout = YES;
  // metalLayer.drawableSize = ???;
  return [metalLayer nextDrawable];
}

id_MTLLibrary WrappedMTLDevice::CreateMTLLibrary(const void *pData, uint32_t bytesCount)
{
  dispatch_data_t data = dispatch_data_create(pData, bytesCount, dispatch_get_main_queue(),
                                              DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  NSError *error;
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLLibrary library = [realMTLDevice newLibraryWithData:data error:&error];
  return library;
}

id_MTLDevice WrappedMTLDevice::CreateObjCWrappedMTLDevice()
{
  ObjCWrappedMTLDevice *objCWrappedMTLDevice = [ObjCWrappedMTLDevice new];
  objCWrappedMTLDevice.wrappedMTLDevice = this;
  return objCWrappedMTLDevice;
}

id_MTLDevice WrappedMTLDevice::real_MTLCreateSystemDefaultDevice()
{
  FixupForMetalDriverAssert();
  return ::MTLCreateSystemDefaultDevice();
}

id_MTLLibrary WrappedMTLDevice::real_newDefaultLibrary()
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLLibrary realMTLLibrary = [realMTLDevice newDefaultLibrary];
  return realMTLLibrary;
}

id_MTLLibrary WrappedMTLDevice::real_newLibraryWithSource(NSString *source,
                                                          MTLCompileOptions *options, NSError **error)
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLLibrary realMTLLibrary =
      [realMTLDevice newLibraryWithSource:source options:options error:error];
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

  MTLRenderPipelineDescriptor *realDescriptor = [descriptor copy];

  // The source descriptor contains wrapped MTLFunction resources
  // These need to be unwrapped in the clone which is used when calling real API
  id_MTLFunction wrappedVertexFunction = descriptor.vertexFunction;
  if(wrappedVertexFunction != NULL)
  {
    realDescriptor.vertexFunction = GetReal(wrappedVertexFunction);
  }

  id_MTLFunction wrappedFragmentFunction = descriptor.fragmentFunction;
  if(wrappedFragmentFunction != NULL)
  {
    realDescriptor.fragmentFunction = GetReal(wrappedFragmentFunction);
  }

  id_MTLRenderPipelineState realMTLRenderPipelineState =
      [realDevice newRenderPipelineStateWithDescriptor:realDescriptor error:error];

  [realDescriptor dealloc];

  return realMTLRenderPipelineState;
}

id_MTLBuffer WrappedMTLDevice::real_newBufferWithLength(NSUInteger length, MTLResourceOptions options)
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLBuffer realMTLBuffer = [realMTLDevice newBufferWithLength:length options:options];
  return realMTLBuffer;
}

id_MTLBuffer WrappedMTLDevice::real_newBufferWithBytes(const void *pointer, NSUInteger length,
                                                       MTLResourceOptions options)
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLBuffer realMTLBuffer =
      [realMTLDevice newBufferWithBytes:pointer length:length options:options];
  return realMTLBuffer;
}

id_MTLTexture WrappedMTLDevice::real_newTextureWithDescriptor(MTLTextureDescriptor *descriptor,
                                                              IOSurfaceRef iosurface,
                                                              NSUInteger plane)
{
  // TODO: should only modify this when Capturing
  // TODO: what about other usage types ie. MTLTextureUsageShaderRead
  if(descriptor.usage == MTLTextureUsageRenderTarget)
    descriptor.usage = MTLTextureUsageUnknown;
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLTexture realMTLTexture =
      [realMTLDevice newTextureWithDescriptor:descriptor iosurface:iosurface plane:plane];
  return realMTLTexture;
}

id_MTLTexture WrappedMTLDevice::real_newTextureWithDescriptor(MTLTextureDescriptor *descriptor)
{
  id_MTLDevice realMTLDevice = Unwrap<id_MTLDevice>(this);
  id_MTLTexture realMTLTexture = [realMTLDevice newTextureWithDescriptor:descriptor];
  return realMTLTexture;
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newCommandQueueWithMaxCommandBufferCount:maxCommandBufferCount];
}

- (MTLSizeAndAlign)heapTextureSizeAndAlignWithDescriptor:(MTLTextureDescriptor *)desc
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice heapTextureSizeAndAlignWithDescriptor:desc];
}

- (MTLSizeAndAlign)heapBufferSizeAndAlignWithLength:(NSUInteger)length
                                            options:(MTLResourceOptions)options
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice heapBufferSizeAndAlignWithLength:length options:options];
}

- (nullable id<MTLHeap>)newHeapWithDescriptor:(MTLHeapDescriptor *)descriptor
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newHeapWithDescriptor:descriptor];
}

- (nullable id<MTLBuffer>)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options
{
  return self.wrappedMTLDevice->newBufferWithLength(length, options);
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newBufferWithBytesNoCopy:pointer
                                               length:length
                                              options:options
                                          deallocator:deallocator];
}

- (nullable id<MTLDepthStencilState>)newDepthStencilStateWithDescriptor:
    (MTLDepthStencilDescriptor *)descriptor
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newDepthStencilStateWithDescriptor:descriptor];
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
{
  return self.wrappedMTLDevice->newTextureWithDescriptor(descriptor);
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
                                          iosurface:(IOSurfaceRef)iosurface
                                              plane:(NSUInteger)plane
    API_AVAILABLE(macos(10.11), ios(11.0))
{
  return self.wrappedMTLDevice->newTextureWithDescriptor(descriptor, iosurface, plane);
}

- (nullable id<MTLTexture>)newSharedTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
    API_AVAILABLE(macos(10.14), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newSharedTextureWithDescriptor:descriptor];
}

- (nullable id<MTLTexture>)newSharedTextureWithHandle:(MTLSharedTextureHandle *)sharedHandle
    API_AVAILABLE(macos(10.14), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newSharedTextureWithHandle:sharedHandle];
}

- (nullable id<MTLSamplerState>)newSamplerStateWithDescriptor:(MTLSamplerDescriptor *)descriptor
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newDefaultLibraryWithBundle:bundle error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithFile:(NSString *)filepath
                                        error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newLibraryWithFile:filepath error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithURL:(NSURL *)url
                                       error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newLibraryWithURL:url error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithData:(dispatch_data_t)data
                                        error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newLibraryWithData:data error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithSource:(NSString *)source
                                        options:(nullable MTLCompileOptions *)options
                                          error:(__autoreleasing NSError **)error
{
  return self.wrappedMTLDevice->newLibraryWithSource(source, options, error);
}

- (void)newLibraryWithSource:(NSString *)source
                     options:(nullable MTLCompileOptions *)options
           completionHandler:(MTLNewLibraryCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                          options:options
                                                       reflection:reflection
                                                            error:error];
}

- (void)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                           completionHandler:(MTLNewRenderPipelineStateCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                completionHandler:completionHandler];
}

- (void)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                                     options:(MTLPipelineOption)options
                           completionHandler:
                               (MTLNewRenderPipelineStateWithReflectionCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                          options:options
                                                completionHandler:completionHandler];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                              error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction error:error];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                            options:(MTLPipelineOption)options
                         reflection:(MTLAutoreleasedComputePipelineReflection *__nullable)reflection
                              error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                                         options:options
                                                      reflection:reflection
                                                           error:error];
}

- (void)newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                          completionHandler:(MTLNewComputePipelineStateCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                               completionHandler:completionHandler];
}

- (void)newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                                    options:(MTLPipelineOption)options
                          completionHandler:
                              (MTLNewComputePipelineStateWithReflectionCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newComputePipelineStateWithDescriptor:descriptor
                                                           options:options
                                                 completionHandler:completionHandler];
}

- (nullable id<MTLFence>)newFence API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newFence];
}

- (BOOL)supportsFeatureSet:(MTLFeatureSet)featureSet
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice supportsFeatureSet:featureSet];
}

- (BOOL)supportsFamily:(MTLGPUFamily)gpuFamily API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice supportsFamily:gpuFamily];
}

- (BOOL)supportsTextureSampleCount:(NSUInteger)sampleCount API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice supportsTextureSampleCount:sampleCount];
}

- (NSUInteger)minimumLinearTextureAlignmentForPixelFormat:(MTLPixelFormat)format
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice minimumLinearTextureAlignmentForPixelFormat:format];
}

- (NSUInteger)minimumTextureBufferAlignmentForPixelFormat:(MTLPixelFormat)format
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice minimumTextureBufferAlignmentForPixelFormat:format];
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithTileDescriptor:(MTLTileRenderPipelineDescriptor *)descriptor
                                 options:(MTLPipelineOption)options
                              reflection:(MTLAutoreleasedRenderPipelineReflection *__nullable)reflection
                                   error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0))API_UNAVAILABLE(tvos)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newRenderPipelineStateWithTileDescriptor:descriptor
                                                              options:options
                                                    completionHandler:completionHandler];
}

- (void)getDefaultSamplePositions:(MTLSamplePosition *)positions
                            count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice getDefaultSamplePositions:positions count:count];
}

- (nullable id<MTLArgumentEncoder>)newArgumentEncoderWithArguments:
    (NSArray<MTLArgumentDescriptor *> *)arguments API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newArgumentEncoderWithArguments:arguments];
}

- (BOOL)supportsRasterizationRateMapWithLayerCount:(NSUInteger)layerCount
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice supportsRasterizationRateMapWithLayerCount:layerCount];
}

- (nullable id<MTLRasterizationRateMap>)newRasterizationRateMapWithDescriptor:
    (MTLRasterizationRateMapDescriptor *)descriptor
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newRasterizationRateMapWithDescriptor:descriptor];
}

- (nullable id<MTLIndirectCommandBuffer>)
newIndirectCommandBufferWithDescriptor:(MTLIndirectCommandBufferDescriptor *)descriptor
                       maxCommandCount:(NSUInteger)maxCount
                               options:(MTLResourceOptions)options
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newIndirectCommandBufferWithDescriptor:descriptor
                                                    maxCommandCount:maxCount
                                                            options:options];
}

- (nullable id<MTLEvent>)newEvent API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newEvent];
}

- (nullable id<MTLSharedEvent>)newSharedEvent API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newSharedEvent];
}

- (nullable id<MTLSharedEvent>)newSharedEventWithHandle:(MTLSharedEventHandle *)sharedEventHandle
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newSharedEventWithHandle:sharedEventHandle];
}

- (MTLSize)sparseTileSizeWithTextureType:(MTLTextureType)textureType
                             pixelFormat:(MTLPixelFormat)pixelFormat
                             sampleCount:(NSUInteger)sampleCount
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
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
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newCounterSampleBufferWithDescriptor:descriptor error:error];
}

- (void)sampleTimestamps:(MTLTimestamp *)cpuTimestamp
            gpuTimestamp:(MTLTimestamp *)gpuTimestamp API_AVAILABLE(macos(10.15), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice sampleTimestamps:cpuTimestamp gpuTimestamp:gpuTimestamp];
}

- (BOOL)supportsCounterSampling:(MTLCounterSamplingPoint)samplingPoint
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice supportsCounterSampling:samplingPoint];
}

- (BOOL)supportsVertexAmplificationCount:(NSUInteger)count
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice supportsVertexAmplificationCount:count];
}

- (nullable id<MTLDynamicLibrary>)newDynamicLibrary:(id<MTLLibrary>)library
                                              error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newDynamicLibrary:library error:error];
}

- (nullable id<MTLDynamicLibrary>)newDynamicLibraryWithURL:(NSURL *)url
                                                     error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newDynamicLibraryWithURL:url error:error];
}

- (nullable id<MTLBinaryArchive>)newBinaryArchiveWithDescriptor:(MTLBinaryArchiveDescriptor *)descriptor
                                                          error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newBinaryArchiveWithDescriptor:descriptor error:error];
}

- (MTLAccelerationStructureSizes)accelerationStructureSizesWithDescriptor:
    (MTLAccelerationStructureDescriptor *)descriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice accelerationStructureSizesWithDescriptor:descriptor];
}

- (nullable id<MTLAccelerationStructure>)newAccelerationStructureWithSize:(NSUInteger)size
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newAccelerationStructureWithSize:size];
}

- (nullable id<MTLAccelerationStructure>)newAccelerationStructureWithDescriptor:
    (MTLAccelerationStructureDescriptor *)descriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLDevice newAccelerationStructureWithDescriptor:descriptor];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
  id fwd = self.realMTLDevice;
  return [fwd methodSignatureForSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
  SEL aSelector = [invocation selector];

  if([self.realMTLDevice respondsToSelector:aSelector])
    [invocation invokeWithTarget:self.realMTLDevice];
  else
    [super forwardInvocation:invocation];
}

@end
