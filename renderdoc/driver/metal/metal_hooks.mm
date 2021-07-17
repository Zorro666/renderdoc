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

#include <dlfcn.h>
#include <objc/runtime.h>
#include "common/common.h"
#include "core/core.h"
#include "hooks/hooks.h"
#include "metal_dispatch_table.h"

#define METAL_EXPORT_NAME(function) CONCAT(interposed_, function)

class MetalHook : LibraryHook
{
public:
  MetalHook() {}
  void RegisterHooks();

  // default to RTLD_NEXT if we haven't gotten a more specific library handle
  void *handle = RTLD_NEXT;
} metalhook;

static IMP _original_newDefaultLibrary;

/*
 @protocol Wrapped_MTLDevice <MTLDevice>

@end
*/

@interface Wrapped_MTLDevice : NSObject<MTLDevice>

@property(assign) id<MTLDevice> realMTLDevice;

@end

@implementation Wrapped_MTLDevice

- (void)getDefaultSamplePositions:(nonnull MTLSamplePosition *)positions count:(NSUInteger)count
{
  return [self.realMTLDevice getDefaultSamplePositions:positions count:count];
}

- (MTLSizeAndAlign)heapBufferSizeAndAlignWithLength:(NSUInteger)length
                                            options:(MTLResourceOptions)options
{
  return [self.realMTLDevice heapBufferSizeAndAlignWithLength:length options:options];
}

- (MTLSizeAndAlign)heapTextureSizeAndAlignWithDescriptor:(nonnull MTLTextureDescriptor *)desc
{
  return [self.realMTLDevice heapTextureSizeAndAlignWithDescriptor:desc];
}

- (NSUInteger)minimumLinearTextureAlignmentForPixelFormat:(MTLPixelFormat)format
{
  return [self.realMTLDevice minimumLinearTextureAlignmentForPixelFormat:format];
}

- (NSUInteger)minimumTextureBufferAlignmentForPixelFormat:(MTLPixelFormat)format
{
  return [self.realMTLDevice minimumTextureBufferAlignmentForPixelFormat:format];
}

- (nullable id<MTLArgumentEncoder>)newArgumentEncoderWithArguments:
    (nonnull NSArray<MTLArgumentDescriptor *> *)arguments
{
  return [self.realMTLDevice newArgumentEncoderWithArguments:arguments];
}

- (nullable id<MTLBuffer>)newBufferWithBytes:(nonnull const void *)pointer
                                      length:(NSUInteger)length
                                     options:(MTLResourceOptions)options
{
  NSLog(@"Wrapped_MTLDevice newBufferWithBytes length %lu options 0x%lX", length,
        (unsigned long)options);
  return [self.realMTLDevice newBufferWithBytes:pointer length:length options:options];
}

- (nullable id<MTLBuffer>)newBufferWithBytesNoCopy:(nonnull void *)pointer
                                            length:(NSUInteger)length
                                           options:(MTLResourceOptions)options
                                       deallocator:(void (^_Nullable)(void *_Nonnull,
                                                                      NSUInteger))deallocator
{
  return [self.realMTLDevice newBufferWithBytesNoCopy:pointer
                                               length:length
                                              options:options
                                          deallocator:deallocator];
}

- (nullable id<MTLBuffer>)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options
{
  return [self.realMTLDevice newBufferWithLength:length options:options];
}

- (nullable id<MTLCommandQueue>)newCommandQueue
{
  NSLog(@"Wrapped_MTLDevice newCommandQueue");
  return [self.realMTLDevice newCommandQueue];
}

- (nullable id<MTLCommandQueue>)newCommandQueueWithMaxCommandBufferCount:(NSUInteger)maxCommandBufferCount
{
  return [self.realMTLDevice newCommandQueueWithMaxCommandBufferCount:maxCommandBufferCount];
}

- (void)newComputePipelineStateWithDescriptor:(nonnull MTLComputePipelineDescriptor *)descriptor
                                      options:(MTLPipelineOption)options
                            completionHandler:
                                (nonnull MTLNewComputePipelineStateWithReflectionCompletionHandler)
                                    completionHandler
{
  return [self.realMTLDevice newComputePipelineStateWithDescriptor:descriptor
                                                           options:options
                                                 completionHandler:completionHandler];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithDescriptor:(nonnull MTLComputePipelineDescriptor *)descriptor
                              options:(MTLPipelineOption)options
                           reflection:(MTLAutoreleasedComputePipelineReflection *_Nullable)reflection
                                error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newComputePipelineStateWithDescriptor:descriptor
                                                           options:options
                                                        reflection:reflection
                                                             error:error];
}

- (void)newComputePipelineStateWithFunction:(nonnull id<MTLFunction>)computeFunction
                          completionHandler:
                              (nonnull MTLNewComputePipelineStateCompletionHandler)completionHandler
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                               completionHandler:completionHandler];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(nonnull id<MTLFunction>)computeFunction
                              error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction error:error];
}

- (void)newComputePipelineStateWithFunction:(nonnull id<MTLFunction>)computeFunction
                                    options:(MTLPipelineOption)options
                          completionHandler:
                              (nonnull MTLNewComputePipelineStateWithReflectionCompletionHandler)
                                  completionHandler
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                                         options:options
                                               completionHandler:completionHandler];
}

- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(nonnull id<MTLFunction>)computeFunction
                            options:(MTLPipelineOption)options
                         reflection:(MTLAutoreleasedComputePipelineReflection *_Nullable)reflection
                              error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newComputePipelineStateWithFunction:computeFunction
                                                         options:options
                                                      reflection:reflection
                                                           error:error];
}

- (nullable id<MTLCounterSampleBuffer>)
newCounterSampleBufferWithDescriptor:(nonnull MTLCounterSampleBufferDescriptor *)descriptor
                               error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newCounterSampleBufferWithDescriptor:descriptor error:error];
}

- (nullable id<MTLLibrary>)newDefaultLibraryWithBundle:(nonnull NSBundle *)bundle
                                                 error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newDefaultLibraryWithBundle:bundle error:error];
}

- (nullable id<MTLDepthStencilState>)newDepthStencilStateWithDescriptor:
    (nonnull MTLDepthStencilDescriptor *)descriptor
{
  return [self.realMTLDevice newDepthStencilStateWithDescriptor:descriptor];
}

- (nullable id<MTLEvent>)newEvent
{
  return [self.realMTLDevice newEvent];
}

- (nullable id<MTLFence>)newFence
{
  return [self.realMTLDevice newFence];
}

- (nullable id<MTLHeap>)newHeapWithDescriptor:(nonnull MTLHeapDescriptor *)descriptor
{
  return [self.realMTLDevice newHeapWithDescriptor:descriptor];
}

- (nullable id<MTLIndirectCommandBuffer>)
newIndirectCommandBufferWithDescriptor:(nonnull MTLIndirectCommandBufferDescriptor *)descriptor
                       maxCommandCount:(NSUInteger)maxCount
                               options:(MTLResourceOptions)options
{
  return [self.realMTLDevice newIndirectCommandBufferWithDescriptor:descriptor
                                                    maxCommandCount:maxCount
                                                            options:options];
}

- (nullable id<MTLLibrary>)newLibraryWithData:(nonnull dispatch_data_t)data
                                        error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newLibraryWithData:data error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithFile:(nonnull NSString *)filepath
                                        error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newLibraryWithFile:filepath error:error];
}

- (void)newLibraryWithSource:(nonnull NSString *)source
                     options:(nullable MTLCompileOptions *)options
           completionHandler:(nonnull MTLNewLibraryCompletionHandler)completionHandler
{
  return [self.realMTLDevice newLibraryWithSource:source
                                          options:options
                                completionHandler:completionHandler];
}

- (nullable id<MTLLibrary>)newLibraryWithSource:(nonnull NSString *)source
                                        options:(nullable MTLCompileOptions *)options
                                          error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newLibraryWithSource:source options:options error:error];
}

- (nullable id<MTLLibrary>)newLibraryWithURL:(nonnull NSURL *)url
                                       error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newLibraryWithURL:url error:error];
}

- (void)newRenderPipelineStateWithDescriptor:(nonnull MTLRenderPipelineDescriptor *)descriptor
                           completionHandler:
                               (nonnull MTLNewRenderPipelineStateCompletionHandler)completionHandler
{
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                completionHandler:completionHandler];
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithDescriptor:(nonnull MTLRenderPipelineDescriptor *)descriptor
                               error:(NSError *_Nullable *_Nullable)error
{
  NSLog(@"Wrapped_MTLDevice newRenderPipelineStateWithDescriptor descriptor:%@", descriptor);
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor error:error];
}

- (void)newRenderPipelineStateWithDescriptor:(nonnull MTLRenderPipelineDescriptor *)descriptor
                                     options:(MTLPipelineOption)options
                           completionHandler:
                               (nonnull MTLNewRenderPipelineStateWithReflectionCompletionHandler)
                                   completionHandler
{
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                          options:options
                                                completionHandler:completionHandler];
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithDescriptor:(nonnull MTLRenderPipelineDescriptor *)descriptor
                             options:(MTLPipelineOption)options
                          reflection:(MTLAutoreleasedRenderPipelineReflection *_Nullable)reflection
                               error:(NSError *_Nullable *_Nullable)error
{
  return [self.realMTLDevice newRenderPipelineStateWithDescriptor:descriptor
                                                          options:options
                                                       reflection:reflection
                                                            error:error];
}

- (nullable id<MTLSamplerState>)newSamplerStateWithDescriptor:(nonnull MTLSamplerDescriptor *)descriptor
{
  return [self.realMTLDevice newSamplerStateWithDescriptor:descriptor];
}

- (nullable id<MTLSharedEvent>)newSharedEvent
{
  return [self.realMTLDevice newSharedEvent];
}

- (nullable id<MTLSharedEvent>)newSharedEventWithHandle:(nonnull MTLSharedEventHandle *)sharedEventHandle
{
  return [self.realMTLDevice newSharedEventWithHandle:sharedEventHandle];
}

- (nullable id<MTLTexture>)newSharedTextureWithDescriptor:(nonnull MTLTextureDescriptor *)descriptor
{
  return [self.realMTLDevice newSharedTextureWithDescriptor:descriptor];
}

- (nullable id<MTLTexture>)newSharedTextureWithHandle:(nonnull MTLSharedTextureHandle *)sharedHandle
{
  return [self.realMTLDevice newSharedTextureWithHandle:sharedHandle];
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(nonnull MTLTextureDescriptor *)descriptor
{
  return [self.realMTLDevice newTextureWithDescriptor:descriptor];
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(nonnull MTLTextureDescriptor *)descriptor
                                          iosurface:(nonnull IOSurfaceRef)iosurface
                                              plane:(NSUInteger)plane
{
  return [self.realMTLDevice newTextureWithDescriptor:descriptor iosurface:iosurface plane:plane];
}

- (void)sampleTimestamps:(nonnull MTLTimestamp *)cpuTimestamp
            gpuTimestamp:(nonnull MTLTimestamp *)gpuTimestamp
{
  return [self.realMTLDevice sampleTimestamps:cpuTimestamp gpuTimestamp:gpuTimestamp];
}

- (BOOL)supportsFamily:(MTLGPUFamily)gpuFamily
{
  return [self.realMTLDevice supportsFamily:gpuFamily];
}

- (BOOL)supportsFeatureSet:(MTLFeatureSet)featureSet
{
  return [self.realMTLDevice supportsFeatureSet:featureSet];
}

- (BOOL)supportsTextureSampleCount:(NSUInteger)sampleCount
{
  return [self.realMTLDevice supportsTextureSampleCount:sampleCount];
}

- (nullable id<MTLLibrary>)newDefaultLibrary
{
  NSLog(@"Wrapped_MTLDevice newDefaultLibrary");
  return [self.realMTLDevice newDefaultLibrary];
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

- (MTLSize)sparseTileSizeWithTextureType:(MTLTextureType)textureType
                             pixelFormat:(MTLPixelFormat)pixelFormat
                             sampleCount:(NSUInteger)sampleCount
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  return [self.realMTLDevice sparseTileSizeWithTextureType:textureType
                                               pixelFormat:pixelFormat
                                               sampleCount:sampleCount];
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

@synthesize argumentBuffersSupport;

@synthesize depth24Stencil8PixelFormatSupported;

@synthesize hasUnifiedMemory;

@synthesize headless;

@synthesize location;

@synthesize locationNumber;

@synthesize lowPower;

@synthesize maxThreadsPerThreadgroup;

@synthesize maxTransferRate;

@synthesize name;

@synthesize rasterOrderGroupsSupported;

@synthesize readWriteTextureSupport;

@synthesize removable;

@synthesize recommendedMaxWorkingSetSize;

@synthesize registryID;

@synthesize supports32BitFloatFiltering;

@synthesize supports32BitMSAA;

@synthesize barycentricCoordsSupported;

@synthesize counterSets;

@synthesize currentAllocatedSize;

@synthesize maxArgumentBufferSamplerCount;

@synthesize hash;

@synthesize maxBufferLength;

@synthesize maxThreadgroupMemoryLength;

@synthesize peerCount;

@synthesize peerGroupID;

@synthesize peerIndex;

@synthesize programmableSamplePositionsSupported;

@synthesize sparseTileSizeInBytes;

@synthesize description;

@synthesize superclass;

@synthesize supportsBCTextureCompression;

@synthesize supportsDynamicLibraries;

@synthesize supportsFunctionPointers;

@synthesize supportsPullModelInterpolation;

@synthesize supportsQueryTextureLOD;

@synthesize supportsRaytracing;

@synthesize supportsShaderBarycentricCoordinates;

@end

/*
 id<MTLLibrary> _hooked_newDefaultLibrary(id self, SEL _cmd)
{
  NSLog(@"hooked newDefaultLibrary");
  id<MTLLibrary> returnValue = ((id<MTLLibrary>(*)(id, SEL))_original_newDefaultLibrary)(self,
_cmd);
  return returnValue;
}
 */

id<MTLDevice> METAL_EXPORT_NAME(MTLCreateSystemDefaultDevice)(void)
{
  if(RenderDoc::Inst().IsReplayApp())
  {
    if(!METAL.MTLCreateSystemDefaultDevice)
      METAL.PopulateForReplay();

    return METAL.MTLCreateSystemDefaultDevice();
  }

  auto mtlDevice = METAL.MTLCreateSystemDefaultDevice();
  Wrapped_MTLDevice *wrappedDevice = [Wrapped_MTLDevice alloc];
  /*
    Method m = class_getInstanceMethod([mtlDevice class], @selector(newDefaultLibrary));
    _original_newDefaultLibrary = method_setImplementation(m, (IMP)_hooked_newDefaultLibrary);
  */

  wrappedDevice.realMTLDevice = mtlDevice;
  return wrappedDevice;
}

#define DECL_HOOK_EXPORT(function)                                               \
  __attribute__((used)) static struct                                            \
  {                                                                              \
    const void *replacment;                                                      \
    const void *replacee;                                                        \
  } _interpose_def_##function __attribute__((section("__DATA,__interpose"))) = { \
      (const void *)(unsigned long)&METAL_EXPORT_NAME(function),                 \
      (const void *)(unsigned long)&function,                                    \
  };

DECL_HOOK_EXPORT(MTLCreateSystemDefaultDevice);

static void MetalHooked(void *handle)
{
  RDCDEBUG("Metal library hooked");

  // store the handle for any pass-through implementations that need to look up their onward
  // pointers
  metalhook.handle = handle;

  // as a hook callback this is only called while capturing
  RDCASSERT(!RenderDoc::Inst().IsReplayApp());

// fetch non-hooked functions into our dispatch table
#define METAL_FETCH(func) METAL.func = &func;
  METAL_NONHOOKED_SYMBOLS(METAL_FETCH)
#undef METAL_FETCH
}

void MetalHook::RegisterHooks()
{
  RDCLOG("Registering Metal hooks");

  // Library hooks : this should be framework hooks?
  LibraryHooks::RegisterLibraryHook(
      "/System/Library/Frameworks/Metal.framework/Versions/Current/Metal", &MetalHooked);

// Function hooks
#define METAL_REGISTER(func)                                                                      \
  LibraryHooks::RegisterFunctionHook("Metal", FunctionHook(STRINGIZE(func), (void **)&METAL.func, \
                                                           (void *)&METAL_EXPORT_NAME(func)));
  METAL_HOOKED_SYMBOLS(METAL_REGISTER)
#undef METAL_REGISTER
}

MetalDispatchTable METAL = {};

bool MetalDispatchTable::PopulateForReplay()
{
  return false;
}
