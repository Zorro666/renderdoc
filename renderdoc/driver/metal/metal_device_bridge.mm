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
#include <Availability.h>
#include "metal_types_bridge.h"

// Define Mac SDK versions when compiling with earlier SDKs
#ifndef __MAC_12_0
#define __MAC_12_0 120000
#endif

// Wrapper for MTLDevice
@implementation ObjCWrappedMTLDevice

// ObjCWrappedMTLDevice specific
- (id<MTLDevice>)real
{
  MTL::Device *real = Unwrap<MTL::Device *>(self.wrappedCPP);
  return id<MTLDevice>(real);
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
  id fwd = self.real;
  return [fwd methodSignatureForSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
  SEL aSelector = [invocation selector];

  if([self.real respondsToSelector:aSelector])
    [invocation invokeWithTarget:self.real];
  else
    [super forwardInvocation:invocation];
}

// MTLDevice : based on the protocol defined in
// Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk/System/Library/Frameworks/Metal.framework/Headers/MTLDevice.h

/*!
 @property name
 @abstract The full name of the vendor device.
 */
- (NSString *)name
{
  return self.real.name;
}

/*!
 @property registryID
 @abstract Returns the IORegistry ID for the Metal device
 @discussion The registryID value for a Metal device is global to all tasks, and may be used
 to identify the GPU across task boundaries.
*/
- (uint64_t)registryID API_AVAILABLE(macos(10.13), ios(11.0))
{
  return self.real.registryID;
}

/*!
 @property maxThreadsPerThreadgroup
 @abstract The maximum number of threads along each dimension.
 */
- (MTLSize)maxThreadsPerThreadgroup API_AVAILABLE(macos(10.11), ios(9.0))
{
  return self.real.maxThreadsPerThreadgroup;
}

/*!
 @property lowPower
 @abstract On systems that support automatic graphics switching, this will return YES for the the
 low power device.
 */
- (BOOL)isLowPower API_AVAILABLE(macos(10.11), macCatalyst(13.0))API_UNAVAILABLE(ios)
{
  return self.real.lowPower;
}

/*!
 @property headless
 @abstract On systems that include more that one GPU, this will return YES for any device that does
 not support any displays.  Only available on Mac OS X.
 */
- (BOOL)isHeadless API_AVAILABLE(macos(10.11), macCatalyst(13.0))API_UNAVAILABLE(ios)
{
  return self.real.headless;
}

/*!
 @property removable
 @abstract If this GPU is removable, this property will return YES.
 @discussion If a GPU is is removed without warning, APIs may fail even with good input, even before
 a notification can get posted informing
 the application that the device has been removed.
 */
- (BOOL)isRemovable API_AVAILABLE(macos(10.13), macCatalyst(13.0))API_UNAVAILABLE(ios)
{
  return self.real.removable;
}

/*!
 @property hasUnifiedMemory
 @abstract Returns YES if this GPU shares its memory with the rest of the machine (CPU, etc.)
 @discussion Some GPU architectures do not have dedicated local memory and instead only use the same
 memory shared with the rest
 of the machine.  This property will return YES for GPUs that fall into that category.
*/
- (BOOL)hasUnifiedMemory API_AVAILABLE(macos(10.15), ios(13.0))
{
  return self.real.hasUnifiedMemory;
}

/*!
 @property recommendedMaxWorkingSetSize
 @abstract Returns an approximation of how much memory this device can use with good performance.
 @discussion Performance may be improved by keeping the total size of all resources (texture and
 buffers)
 and heaps less than this threshold, beyond which the device is likely to be overcommitted and incur
 a
 performance penalty. */
- (uint64_t)recommendedMaxWorkingSetSize API_AVAILABLE(macos(10.12), macCatalyst(13.0))
    API_UNAVAILABLE(ios)
{
  return self.real.recommendedMaxWorkingSetSize;
}

/*!
 @property location
 @abstract Returns an enum that indicates where the GPU is located relative to the host computer.
 @discussion The returned value indicates if the GPU is built into the computer, inserted into
 a slot internal to the computer, or external to the computer. Otherwise it is Unspecified */
- (MTLDeviceLocation)location API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.location;
}

/*!
 @property locationNumber
 @abstract Returns a value that further specifies the GPU's location
 @discussion The returned value indicates which slot or Thunderbolt port the GPU is attached
 to. For Built-in GPUs, if LowPower this value is 0, otherwise it is 1.  It is possible for multiple
 GPUs to have
 the same location and locationNumber; e.g.: A PCI card with multiple GPUs, or an eGPU
 daisy-chained off of another eGPU attached to a host Thunderbolt port. */
- (NSUInteger)locationNumber API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.locationNumber;
}

/*!
 @property maxTransferRate
 @abstract Upper bound of System RAM <=> VRAM transfer rate in bytes/sec
 @discussion The returned value indicates the theoretical maximum data rate in bytes/second
 from host memory to the GPU's VRAM. This is derived from the raw data clock rate and as
 such may not be reachable under real-world conditions. For Built-in GPUs this value is 0. */
- (uint64_t)maxTransferRate API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.maxTransferRate;
}

/*!
 @property depth24Stencil8PixelFormatSupported
 @abstract If YES, device supports MTLPixelFormatDepth24Unorm_Stencil8.
 */
- (BOOL)isDepth24Stencil8PixelFormatSupported API_AVAILABLE(macos(10.11), macCatalyst(13.0))
    API_UNAVAILABLE(ios)
{
  return self.real.depth24Stencil8PixelFormatSupported;
}

/*!
 @property readWriteTextureSupport
 @abstract Query support tier for read-write texture formats.
 @return MTLReadWriteTextureTier enum value.
 */
- (MTLReadWriteTextureTier)readWriteTextureSupport API_AVAILABLE(macos(10.13), ios(11.0))
{
  return self.real.readWriteTextureSupport;
}

/*!
 @property argumentBuffersSupport
 @abstract Query support tier for Argument Buffers.
 @return MTLArgumentBuffersTier enum value.
 */
- (MTLArgumentBuffersTier)argumentBuffersSupport API_AVAILABLE(macos(10.13), ios(11.0))
{
  return self.real.argumentBuffersSupport;
}

/*!
 @property rasterOrderGroupsSupported
 @abstract Query device for raster order groups support.
 @return BOOL value. If YES, the device supports raster order groups. If NO, the device does not.
 */
- (BOOL)areRasterOrderGroupsSupported API_AVAILABLE(macos(10.13), ios(11.0))
{
  return self.real.areRasterOrderGroupsSupported;
}

/*!
 @property supports32BitFloatFiltering
 @abstract Query device for 32-bit Float texture filtering support. Specifically, R32Float,
 RG32Float, and RGBA32Float.
 @return BOOL value. If YES, the device supports filtering 32-bit Float textures. If NO, the device
 does not.
 */
- (BOOL)supports32BitFloatFiltering API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supports32BitFloatFiltering;
}

/*!
 @property supports32BitMSAA
 @abstract Query device for 32-bit MSAA texture support. Specifically, added support for allocating
 32-bit Integer format textures (R32Uint, R32Sint, RG32Uint, RG32Sint, RGBA32Uint, and RGBA32Sint)
 and resolving 32-bit Float format textures (R32Float, RG32Float, and RGBA32Float).
 @return BOOL value. If YES, the device supports these additional 32-bit MSAA texture capabilities.
 If NO, the devices does not.
 */
- (BOOL)supports32BitMSAA API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supports32BitMSAA;
}

/*!
@property supportsQueryTextureLOD
@abstract Query device for whether it supports the `calculate_clampled_lod` and
`calculate_unclamped_lod` Metal shading language functionality.
@return BOOL value. If YES, the device supports the calculate LOD functionality. If NO, the device
does not.
*/
- (BOOL)supportsQueryTextureLOD API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supportsQueryTextureLOD;
}

/*!
 @property supportsBCTextureCompression
 @abstract Query device for BC Texture format support
 @return BOOL value. If YES, the device supports compressed BC Texture formats. If NO, the device
 does not.
 */
- (BOOL)supportsBCTextureCompression API_AVAILABLE(macos(11.0))API_UNAVAILABLE(ios)
{
  return self.real.supportsBCTextureCompression;
}

/*!
 @property supportsPullModelInterpolation
 @abstract Query device for pull model interpolation support which allows a fragment shader to
 compute multiple interpolations (at center, at centroid, at offset, at sample) of a fragment input.
 @return BOOL value. If YES, the device supports pull model interpolation. If NO, the device does
 not.
 */
- (BOOL)supportsPullModelInterpolation API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supportsPullModelInterpolation;
}

/*!
 @property barycentricsSupported
 @abstract Query device for Barycentric coordinates support; deprecated, use
 supportsShaderBarycentricCoordinates
 @return BOOL value. If YES, the device barycentric coordinates
 */
- (BOOL)areBarycentricCoordsSupported API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.barycentricCoordsSupported;
}

/*!
 @property supportsShaderBarycentricCoordinates
 @abstract Query device for Barycentric Coordinates support.
 @return BOOL value. If YES, the device supports barycentric coordinates. If NO, the device does
 not.
 */
- (BOOL)supportsShaderBarycentricCoordinates API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.supportsShaderBarycentricCoordinates;
}

/*!
 @property currentAllocatedSize
 @abstract The current size in bytes of all resources allocated by this device
 */
- (NSUInteger)currentAllocatedSize API_AVAILABLE(macos(10.13), ios(11.0))
{
  return self.real.currentAllocatedSize;
}

/*!
 @method newCommandQueue
 @brief Create and return a new command queue.   Command Queues created via this method will only
 allow up to 64 non-completed command buffers.
 @return The new command queue object
 */
- (nullable id<MTLCommandQueue>)newCommandQueue
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newCommandQueue];
}

/*!
 @method newCommandQueueWithMaxCommandBufferCount
 @brief Create and return a new command queue with a given upper bound on non-completed command
 buffers.
 @return The new command queue object
 */
- (nullable id<MTLCommandQueue>)newCommandQueueWithMaxCommandBufferCount:(NSUInteger)maxCommandBufferCount
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newCommandQueueWithMaxCommandBufferCount:maxCommandBufferCount];
}

/*!
 @method heapTextureSizeAndAlignWithDescriptor:
 @abstract Determine the byte size of textures when sub-allocated from a heap.
 @discussion This method can be used to help determine the required heap size.
 */
- (MTLSizeAndAlign)heapTextureSizeAndAlignWithDescriptor:(MTLTextureDescriptor *)desc
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real heapTextureSizeAndAlignWithDescriptor:desc];
}

/*!
 @method heapBufferSizeAndAlignWithLength:options:
 @abstract Determine the byte size of buffers when sub-allocated from a heap.
 @discussion This method can be used to help determine the required heap size.
 */
- (MTLSizeAndAlign)heapBufferSizeAndAlignWithLength:(NSUInteger)length
                                            options:(MTLResourceOptions)options
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real heapBufferSizeAndAlignWithLength:length options:options];
}

/*!
 @method newHeapWithDescriptor:
 @abstract Create a new heap with the given descriptor.
 */
- (nullable id<MTLHeap>)newHeapWithDescriptor:(MTLHeapDescriptor *)descriptor
    API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newHeapWithDescriptor:descriptor];
}

/*!
 @method newBufferWithLength:options:
 @brief Create a buffer by allocating new memory.
 */
- (nullable id<MTLBuffer>)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newBufferWithLength:length options:options];
}

/*!
 @method newBufferWithBytes:length:options:
 @brief Create a buffer by allocating new memory and specifing the initial contents to be copied
 into it.
 */
- (nullable id<MTLBuffer>)newBufferWithBytes:(const void *)pointer
                                      length:(NSUInteger)length
                                     options:(MTLResourceOptions)options
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newBufferWithBytes:pointer length:length options:options];
}

/*!
 @method newBufferWithBytesNoCopy:length:options:deallocator:
 @brief Create a buffer by wrapping an existing part of the address space.
 */
- (nullable id<MTLBuffer>)newBufferWithBytesNoCopy:(void *)pointer
                                            length:(NSUInteger)length
                                           options:(MTLResourceOptions)options
                                       deallocator:(void (^__nullable)(void *pointer,
                                                                       NSUInteger length))deallocator
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newBufferWithBytesNoCopy:pointer
                                      length:length
                                     options:options
                                 deallocator:deallocator];
}

/*!
 @method newDepthStencilStateWithDescriptor:
 @brief Create a depth/stencil test state object.
 */
- (nullable id<MTLDepthStencilState>)newDepthStencilStateWithDescriptor:
    (MTLDepthStencilDescriptor *)descriptor
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newDepthStencilStateWithDescriptor:descriptor];
}

/*!
 @method newTextureWithDescriptor:
 @abstract Allocate a new texture with privately owned storage.
 */
- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newTextureWithDescriptor:descriptor];
}

/*!
 @method newTextureWithDescriptor:iosurface:plane
 @abstract Create a new texture from an IOSurface.
 @param descriptor A description of the properties for the new texture.
 @param iosurface The IOSurface to use as storage for the new texture.
 @param plane The plane within the IOSurface to use.
 @return A new texture object.
 */
- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
                                          iosurface:(IOSurfaceRef)iosurface
                                              plane:(NSUInteger)plane
    API_AVAILABLE(macos(10.11), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newTextureWithDescriptor:descriptor iosurface:iosurface plane:plane];
}

/*!
 @method newSharedTextureWithDescriptor
 @abstract Create a new texture that can be shared across process boundaries.
 @discussion This texture can be shared between process boundaries
 but not between different GPUs, by passing its MTLSharedTextureHandle.
 @param descriptor A description of the properties for the new texture.
 @return A new texture object.
 */
- (nullable id<MTLTexture>)newSharedTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
    API_AVAILABLE(macos(10.14), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newSharedTextureWithDescriptor:descriptor];
}

/*!
 @method newSharedTextureWithHandle
 @abstract Recreate shared texture from received texture handle.
 @discussion This texture was shared between process boundaries by other
 process using MTLSharedTextureHandle. Current process will now share
 it with other processes and will be able to interact with it (but still
 in scope of the same GPUs).
 @param sharedHandle Handle to shared texture in this process space.
 @return A new texture object.
 */
- (nullable id<MTLTexture>)newSharedTextureWithHandle:(MTLSharedTextureHandle *)sharedHandle
    API_AVAILABLE(macos(10.14), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newSharedTextureWithHandle:sharedHandle];
}

/*!
 @method newSamplerStateWithDescriptor:
 @abstract Create a new sampler.
*/
- (nullable id<MTLSamplerState>)newSamplerStateWithDescriptor:(MTLSamplerDescriptor *)descriptor
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newSamplerStateWithDescriptor:descriptor];
}

/*!
 @method newDefaultLibrary
 @abstract Returns the default library for the main bundle.
 @discussion use newDefaultLibraryWithBundle:error: to get an NSError in case of failure.
 */
- (nullable id<MTLLibrary>)newDefaultLibrary
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newDefaultLibrary];
}

/*
 @method newDefaultLibraryWithBundle:error:
 @abstract Returns the default library for a given bundle.
 @return A pointer to the library, nil if an error occurs.
*/
- (nullable id<MTLLibrary>)newDefaultLibraryWithBundle:(NSBundle *)bundle
                                                 error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newDefaultLibraryWithBundle:bundle error:error];
}

/*!
 @method newLibraryWithFile:
 @abstract Load a MTLLibrary from a metallib file.
 */
- (nullable id<MTLLibrary>)newLibraryWithFile:(NSString *)filepath
                                        error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newLibraryWithFile:filepath error:error];
}

/*!
 @method newLibraryWithURL:
 @abstract Load a MTLLibrary from a metallib file.
 */
- (nullable id<MTLLibrary>)newLibraryWithURL:(NSURL *)url
                                       error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newLibraryWithURL:url error:error];
}

/*!
 @method newLibraryWithData:
 @abstract Load a MTLLibrary from a dispatch_data_t
 @param data A metallib file already loaded as data in the form of dispatch_data_t.
 @param error An error if we fail to open the metallib data.
 */
- (nullable id<MTLLibrary>)newLibraryWithData:(dispatch_data_t)data
                                        error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newLibraryWithData:data error:error];
}

/*!
 @method newLibraryWithSource:options:error:
 @abstract Load a MTLLibrary from source.
 */
- (nullable id<MTLLibrary>)newLibraryWithSource:(NSString *)source
                                        options:(nullable MTLCompileOptions *)options
                                          error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newLibraryWithSource:source options:options error:error];
}

/*!
 @method newLibraryWithSource:options:completionHandler:
 @abstract Load a MTLLibrary from source.
 */
- (void)newLibraryWithSource:(NSString *)source
                     options:(nullable MTLCompileOptions *)options
           completionHandler:(MTLNewLibraryCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return
      [self.real newLibraryWithSource:source options:options completionHandler:completionHandler];
}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_12_0
/*!
 @method newLibraryWithStitchedDescriptor:error:
 @abstract Returns a library generated using the graphs in the descriptor.
 */
- (nullable id<MTLLibrary>)newLibraryWithStitchedDescriptor:(MTLStitchedLibraryDescriptor *)descriptor
                                                      error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newLibraryWithStitchedDescriptor:descriptor error:error];
}
#endif

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_12_0
/*!
 @method newLibraryWithStitchedDescriptor:completionHandler:
 @abstract Generates a new library using the graphs in the descriptor.
 */
- (void)newLibraryWithStitchedDescriptor:(MTLStitchedLibraryDescriptor *)descriptor
                       completionHandler:(MTLNewLibraryCompletionHandler)completionHandler
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  [self.real newLibraryWithStitchedDescriptor:descriptor completionHandler:completionHandler];
}
#endif

/*!
 @method newRenderPipelineStateWithDescriptor:error:
 @abstract Create and compile a new MTLRenderPipelineState object synchronously.
 */
- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                               error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRenderPipelineStateWithDescriptor:descriptor error:error];
}

/*!
 @method newRenderPipelineStateWithDescriptor:options:reflection:error:
 @abstract Create and compile a new MTLRenderPipelineState object synchronously and returns
 additional reflection information.
 */
- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                             options:(MTLPipelineOption)options
                          reflection:(MTLAutoreleasedRenderPipelineReflection *__nullable)reflection
                               error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRenderPipelineStateWithDescriptor:descriptor
                                                 options:options
                                              reflection:reflection
                                                   error:error];
}

/*!
 @method newRenderPipelineState:completionHandler:
 @abstract Create and compile a new MTLRenderPipelineState object asynchronously.
 */
- (void)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                           completionHandler:(MTLNewRenderPipelineStateCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRenderPipelineStateWithDescriptor:descriptor
                                       completionHandler:completionHandler];
}

/*!
 @method newRenderPipelineState:options:completionHandler:
 @abstract Create and compile a new MTLRenderPipelineState object asynchronously and returns
 additional reflection information
 */
- (void)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                                     options:(MTLPipelineOption)options
                           completionHandler:
                               (MTLNewRenderPipelineStateWithReflectionCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRenderPipelineStateWithDescriptor:descriptor
                                                 options:options
                                       completionHandler:completionHandler];
}

/*!
 @method newComputePipelineStateWithDescriptor:error:
 @abstract Create and compile a new MTLComputePipelineState object synchronously.
 */
- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                              error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newComputePipelineStateWithFunction:computeFunction error:error];
}

/*!
 @method newComputePipelineStateWithDescriptor:options:reflection:error:
 @abstract Create and compile a new MTLComputePipelineState object synchronously.
 */
- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                            options:(MTLPipelineOption)options
                         reflection:(MTLAutoreleasedComputePipelineReflection *__nullable)reflection
                              error:(__autoreleasing NSError **)error
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newComputePipelineStateWithFunction:computeFunction
                                                options:options
                                             reflection:reflection
                                                  error:error];
}

/*!
 @method newComputePipelineStateWithDescriptor:completionHandler:
 @abstract Create and compile a new MTLComputePipelineState object asynchronously.
 */
- (void)newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                          completionHandler:(MTLNewComputePipelineStateCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newComputePipelineStateWithFunction:computeFunction
                                      completionHandler:completionHandler];
}

/*!
 @method newComputePipelineStateWithDescriptor:options:completionHandler:
 @abstract Create and compile a new MTLComputePipelineState object asynchronously.
 */
- (void)newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction
                                    options:(MTLPipelineOption)options
                          completionHandler:
                              (MTLNewComputePipelineStateWithReflectionCompletionHandler)completionHandler
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newComputePipelineStateWithFunction:computeFunction
                                                options:options
                                      completionHandler:completionHandler];
}

/*!
 @method newComputePipelineStateWithDescriptor:options:reflection:error:
 @abstract Create and compile a new MTLComputePipelineState object synchronously.
 */
- (nullable id<MTLComputePipelineState>)
newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor *)descriptor
                              options:(MTLPipelineOption)options
                           reflection:(MTLAutoreleasedComputePipelineReflection *__nullable)reflection
                                error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newComputePipelineStateWithDescriptor:descriptor
                                                  options:options
                                               reflection:reflection
                                                    error:error];
}

/*!
 @method newComputePipelineStateWithDescriptor:options:completionHandler:
 @abstract Create and compile a new MTLComputePipelineState object asynchronously.
 */
- (void)newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor *)descriptor
                                      options:(MTLPipelineOption)options
                            completionHandler:
                                (MTLNewComputePipelineStateWithReflectionCompletionHandler)completionHandler
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newComputePipelineStateWithDescriptor:descriptor
                                                  options:options
                                        completionHandler:completionHandler];
}

/*!
 @method newFence
 @abstract Create a new MTLFence object
 */
- (nullable id<MTLFence>)newFence API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newFence];
}

/*!
 @method supportsFeatureSet:
 @abstract Returns TRUE if the feature set is supported by this MTLDevice.
 */
- (BOOL)supportsFeatureSet:(MTLFeatureSet)featureSet
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real supportsFeatureSet:featureSet];
}

/*!
 @method supportsFamily:
 @abstract Returns TRUE if the GPU Family is supported by this MTLDevice.
 */
- (BOOL)supportsFamily:(MTLGPUFamily)gpuFamily API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real supportsFamily:gpuFamily];
}

/*!
 @method supportsTextureSampleCount:
 @brief Query device if it support textures with a given sampleCount.
 @return BOOL value. If YES, device supports the given sampleCount for textures. If NO, device does
 not support the given sampleCount.
 */
- (BOOL)supportsTextureSampleCount:(NSUInteger)sampleCount API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real supportsTextureSampleCount:sampleCount];
}

/*!
 @method minimumLinearTextureAlignmentForPixelFormat:
 @abstract Returns the minimum alignment required for offset and rowBytes when creating a linear
 texture. An error is thrown for queries with invalid pixel formats (depth, stencil, or compressed
 formats).
 */
- (NSUInteger)minimumLinearTextureAlignmentForPixelFormat:(MTLPixelFormat)format
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real minimumLinearTextureAlignmentForPixelFormat:format];
}

/*!
 @method minimumTextureBufferAlignmentForPixelFormat:
 @abstract Returns the minimum alignment required for offset and rowBytes when creating a texture
 buffer from a buffer.
 */
- (NSUInteger)minimumTextureBufferAlignmentForPixelFormat:(MTLPixelFormat)format
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real minimumTextureBufferAlignmentForPixelFormat:format];
}

/*!
 @method newRenderPipelineStateWithTileDescriptor:options:reflection:error:
 @abstract Create and compile a new MTLRenderPipelineState object synchronously given a
 MTLTileRenderPipelineDescriptor.
 */
- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithTileDescriptor:(MTLTileRenderPipelineDescriptor *)descriptor
                                 options:(MTLPipelineOption)options
                              reflection:(MTLAutoreleasedRenderPipelineReflection *__nullable)reflection
                                   error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRenderPipelineStateWithTileDescriptor:descriptor
                                                     options:options
                                                  reflection:reflection
                                                       error:error];
}

/*!
 @method newRenderPipelineStateWithTileDescriptor:options:completionHandler:
 @abstract Create and compile a new MTLRenderPipelineState object asynchronously given a
 MTLTileRenderPipelineDescriptor.
 */
- (void)newRenderPipelineStateWithTileDescriptor:(MTLTileRenderPipelineDescriptor *)descriptor
                                         options:(MTLPipelineOption)options
                               completionHandler:
                                   (MTLNewRenderPipelineStateWithReflectionCompletionHandler)completionHandler
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRenderPipelineStateWithTileDescriptor:descriptor
                                                     options:options
                                           completionHandler:completionHandler];
}

/*!
 @property maxThreadgroupMemoryLength
 @abstract The maximum threadgroup memory available, in bytes.
 */
- (NSUInteger)maxThreadgroupMemoryLength API_AVAILABLE(macos(10.13), ios(11.0))
{
  return self.real.maxThreadgroupMemoryLength;
}

/*!
 @property maxArgumentBufferSamplerCount
 @abstract The maximum number of unique argument buffer samplers per app.
 @discussion This limit is only applicable to samplers that have their supportArgumentBuffers
 property set to true. A MTLSamplerState object is considered unique if the configuration of its
 originating MTLSamplerDescriptor properties is unique. For example, two samplers with equal
 minFilter values but different magFilter values are considered unique.
 */
- (NSUInteger)maxArgumentBufferSamplerCount API_AVAILABLE(macos(10.14), ios(12.0))
{
  return self.real.maxArgumentBufferSamplerCount;
}

/*!
 @property programmableSaplePositionsSupported
 @abstract Query device for programmable sample position support.
 @return BOOL value. If YES, the device supports programmable sample positions. If NO, the device
 does not.
 */
- (BOOL)areProgrammableSamplePositionsSupported API_AVAILABLE(macos(10.13), ios(11.0))
{
  return self.real.programmableSamplePositionsSupported;
}

/*!
 @method getDefaultSamplePositions:count:
 @abstract Retrieve the default sample positions.
 @param positions The destination array for default sample position data.
 @param count Specifies the sample count for which to retrieve the default positions, the length of
 the positions array, and must be set to a valid sample count.
 */
- (void)getDefaultSamplePositions:(MTLSamplePosition *)positions
                            count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real getDefaultSamplePositions:positions count:count];
}

/*!
 * @method newArgumentEncoderWithArguments:count:
 * @abstract Creates an argument encoder for an array of argument descriptors which will be encoded
 * sequentially.
 */
- (nullable id<MTLArgumentEncoder>)newArgumentEncoderWithArguments:
    (NSArray<MTLArgumentDescriptor *> *)arguments API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newArgumentEncoderWithArguments:arguments];
}

/*!
 @method supportsRasterizationRateMapWithLayerCount:
 @abstract Query device for variable rasterization rate support with the given number of layers.
 @param layerCount The number of layers for which to query device support.
 @return YES if the device supports creation of rendering using a MTLRasterizationRateMap with the
 given number of layers.
 */
- (BOOL)supportsRasterizationRateMapWithLayerCount:(NSUInteger)layerCount
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real supportsRasterizationRateMapWithLayerCount:layerCount];
}

/*!
 @method newRasterizationRateMapWithDescriptor:
 @abstract Creates a new variable rasterization rate map with the given descriptor.
 @discussion If '[self supportsRasterizationRateMapWithLayerCount:descriptor.layerCount]' returns
 NO, or descriptor.screenSize describes an empty region, the result will always be nil.
 @return A MTLRasterizationRateMap instance that can be used for rendering on this MTLDevice, or nil
 if the device does not support the combination of parameters stored in the descriptor.
 */
- (nullable id<MTLRasterizationRateMap>)newRasterizationRateMapWithDescriptor:
    (MTLRasterizationRateMapDescriptor *)descriptor
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRasterizationRateMapWithDescriptor:descriptor];
}

/*!
 * @method newIndirectCommandBufferWithDescriptor:maxCommandCount:options
 * @abstract Creates a new indirect command buffer with the given descriptor and count.
 * @param descriptor The descriptor encodes the maximum logical stride of each command.
 * @param maxCount The maximum number of commands that this buffer can contain.
 * @param options The options for the indirect command buffer.
 * @discussion The returned buffer can be safely executed without first encoding into (but is
 * wasteful).
 */
- (nullable id<MTLIndirectCommandBuffer>)
newIndirectCommandBufferWithDescriptor:(MTLIndirectCommandBufferDescriptor *)descriptor
                       maxCommandCount:(NSUInteger)maxCount
                               options:(MTLResourceOptions)options
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newIndirectCommandBufferWithDescriptor:descriptor
                                           maxCommandCount:maxCount
                                                   options:options];
}

/*!
 @method newEvent
 @abstract Returns a new single-device non-shareable Metal event object
*/
- (nullable id<MTLEvent>)newEvent API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newEvent];
}

/*!
 @method newSharedEvent
 @abstract Returns a shareable multi-device event.
 */
- (nullable id<MTLSharedEvent>)newSharedEvent API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newSharedEvent];
}

/*!
 @method newSharedEventWithHandle
 @abstract Creates a shareable multi-device event from an existing shared event handle.
*/
- (nullable id<MTLSharedEvent>)newSharedEventWithHandle:(MTLSharedEventHandle *)sharedEventHandle
    API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newSharedEventWithHandle:sharedEventHandle];
}

/*!
 @property peerGroupID
 @abstract If a device supports peer to peer transfers with another device (or devices), this
 property will return
 a unique 64-bit identifier associated with all devices in the same peer group.
 */
- (uint64_t)peerGroupID API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.peerGroupID;
}

/*!
 @property peerIndex
 @abstract All Metal devices that are part of the same peer group will have a unique index value
 within the group in
 the range from 0 to peerCount - 1.
 */
- (uint32_t)peerIndex API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.peerIndex;
}

/*!
 @property peerCount
 @abstract For Metal devices that are part of a peer group, this property returns the total number
 of devices in that group.
 */
- (uint32_t)peerCount API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  return self.real.peerCount;
}

/*!
 * @method sparseTileSizeWithTextureType:pixelFormat:sampleCount:
 * @abstract Returns tile size for sparse texture with given type, pixel format and sample count.
 */
- (MTLSize)sparseTileSizeWithTextureType:(MTLTextureType)textureType
                             pixelFormat:(MTLPixelFormat)pixelFormat
                             sampleCount:(NSUInteger)sampleCount
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real sparseTileSizeWithTextureType:textureType
                                      pixelFormat:pixelFormat
                                      sampleCount:sampleCount];
}

/*!
 @property sparseTileSizeInBytes
 @abstract Returns the number of bytes required to map one sparse texture tile.
 */
- (NSUInteger)sparseTileSizeInBytes API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  return self.real.sparseTileSizeInBytes;
}

/*!
 * @method convertSparsePixelRegions:toTileRegions:withTileSize:alignmentMode:numRegions:
 * @abstract Converts regions in pixels to regions in sparse tiles using specified alignment mode.
   Tile size can be obtained from tileSizeWithTextureType:pixelFormat:sampleCount: method.
 */
- (void)convertSparsePixelRegions:(const MTLRegion[_Nonnull])pixelRegions
                    toTileRegions:(MTLRegion[_Nonnull])tileRegions
                     withTileSize:(MTLSize)tileSize
                    alignmentMode:(MTLSparseTextureRegionAlignmentMode)mode
                       numRegions:(NSUInteger)numRegions
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real convertSparsePixelRegions:pixelRegions
                                toTileRegions:tileRegions
                                 withTileSize:tileSize
                                alignmentMode:mode
                                   numRegions:numRegions];
}

/*!
 * @method convertSparseTileRegions:toPixelRegions:withTileSize:numRegions:
 * @abstract Convertes region in sparse tiles to region in pixels
   Tile size can be obtained from tileSizeWithTextureType:pixelFormat:sampleCount: method.
 */
- (void)convertSparseTileRegions:(const MTLRegion[_Nonnull])tileRegions
                  toPixelRegions:(MTLRegion[_Nonnull])pixelRegions
                    withTileSize:(MTLSize)tileSize
                      numRegions:(NSUInteger)numRegions
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real convertSparseTileRegions:tileRegions
                              toPixelRegions:pixelRegions
                                withTileSize:tileSize
                                  numRegions:numRegions];
}

- (NSUInteger)maxBufferLength API_AVAILABLE(macos(10.14), ios(12.0))
{
  return self.real.maxBufferLength;
}

/*!
 @property counterSets
 @abstract Returns the set of Counter Sets exposed by the device.
 */
- (NSArray<id<MTLCounterSet>> *)counterSets API_AVAILABLE(macos(10.15), ios(14.0))
{
  return self.real.counterSets;
}

/*!
 @method newCounterSampleBufferWithDescriptor:error:
 @abstract Given a counter sample buffer descriptor, allocate a new counter
 sample buffer.
 This may return nil if the counters may not all be collected simultaneously.
 @param descriptor The descriptor to create a sample buffer for
 @param error An error return on failure.
 */
- (nullable id<MTLCounterSampleBuffer>)newCounterSampleBufferWithDescriptor:
                                           (MTLCounterSampleBufferDescriptor *)descriptor
                                                                      error:(NSError **)error
    API_AVAILABLE(macos(10.15), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newCounterSampleBufferWithDescriptor:descriptor error:error];
}

/*!
 @method sampleTimestamps:gpuTimestamp:
 @abstract Sample the CPU and GPU timestamps as closely as possible.
 @param cpuTimestamp The timestamp on the CPU
 @param gpuTimestamp The timestamp on the GPU
 */
- (void)sampleTimestamps:(MTLTimestamp *)cpuTimestamp
            gpuTimestamp:(MTLTimestamp *)gpuTimestamp API_AVAILABLE(macos(10.15), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real sampleTimestamps:cpuTimestamp gpuTimestamp:gpuTimestamp];
}

/*!
 @method supportsCounterSampling:
 @abstract Query device for counter sampling points support.
 @param samplingPoint Query index
 @return BOOL value. If YES, the device supports counter sampling at given point.
*/
- (BOOL)supportsCounterSampling:(MTLCounterSamplingPoint)samplingPoint
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real supportsCounterSampling:samplingPoint];
}

/*!
 @property supportsVertexAmplificationCount:
 @abstract Query device for vertex amplification support.
 @param count The amplification count to check
 @return BOOL value. If YES, the device supports vertex amplification with the given count. If NO,
 the device does not.
 */
- (BOOL)supportsVertexAmplificationCount:(NSUInteger)count
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real supportsVertexAmplificationCount:count];
}

/*!
 @property supportsDynamicLibraries
 @abstract Query device support for creating and using dynamic libraries in a compute pipeline.
 @return BOOL value. If YES, the device supports creating and using dynamic libraries in a compute
 pipeline. If NO, the device does not.
 */
- (BOOL)supportsDynamicLibraries API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supportsDynamicLibraries;
}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_12_0
/*!
 @property supportsRenderDynamicLibraries
 @abstract Query device support for creating and using dynamic libraries in render pipeline stages.
 @return BOOL value. If YES, the device supports creating and using dynamic libraries in render
 pipeline stages. If NO, the device does not.
 */
- (BOOL)supportsRenderDynamicLibraries API_AVAILABLE(macos(12.0), ios(15.0))
{
  return self.real.supportsRenderDynamicLibraries;
}
#endif

/*!
 @method newDynamicLibrary:error:
 @abstract Creates a MTLDynamicLibrary by compiling the code in a MTLLibrary.
 @see MTLDynamicLibrary
 @param library The MTLLibrary from which to compile code. This library must have .type set to
 MTLLibraryTypeDynamic.
 @param error If an error occurs during creation, this parameter is updated to describe the failure.
 @return On success, the MTLDynamicLibrary containing compiled code. On failure, nil.
 */
- (nullable id<MTLDynamicLibrary>)newDynamicLibrary:(id<MTLLibrary>)library
                                              error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newDynamicLibrary:library error:error];
}

/*!
 @method newDynamicLibraryWithURL:error:
 @abstract Creates a MTLDynamicLibrary by loading compiled code from a file.
 @see MTLDynamicLibrary
 @param url The file URL from which to load. If the file contains no compiled code for this device,
 compilation is attempted as with newDynamicLibrary:error:
 @param error If an error occurs during creation, this parameter is updated to describe the failure.
 @return On success, the MTLDynamicLibrary containing compiled code (either loaded or compiled). On
 failure, nil.
 */
- (nullable id<MTLDynamicLibrary>)newDynamicLibraryWithURL:(NSURL *)url
                                                     error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newDynamicLibraryWithURL:url error:error];
}

/*!
 @method newBinaryArchiveWithDescriptor:error:
 @abstract Creates a MTLBinaryArchive using the configuration in the descriptor.
 @see MTLBinaryArchive
 @param descriptor The descriptor for the configuration of the binary archive to create.
 @param error If an error occurs during creation, this parameter is updated to describe the failure.
 @return On success, the created MTLBinaryArchive. On failure, nil.
 */
- (nullable id<MTLBinaryArchive>)newBinaryArchiveWithDescriptor:(MTLBinaryArchiveDescriptor *)descriptor
                                                          error:(NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newBinaryArchiveWithDescriptor:descriptor error:error];
}

/*!
@property supportsRaytracing
@abstract Query device support for using ray tracing from compute pipelines.
@return BOOL value. If YES, the device supports ray tracing from compute pipelines. If NO, the
device does not.
*/
- (BOOL)supportsRaytracing API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supportsRaytracing;
}

- (MTLAccelerationStructureSizes)accelerationStructureSizesWithDescriptor:
    (MTLAccelerationStructureDescriptor *)descriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real accelerationStructureSizesWithDescriptor:descriptor];
}

- (nullable id<MTLAccelerationStructure>)newAccelerationStructureWithSize:(NSUInteger)size
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newAccelerationStructureWithSize:size];
}

- (nullable id<MTLAccelerationStructure>)newAccelerationStructureWithDescriptor:
    (MTLAccelerationStructureDescriptor *)descriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newAccelerationStructureWithDescriptor:descriptor];
}

/*!
 @property supportsFunctionPointers
 @abstract Query device support for using function pointers from compute pipelines.
 @return BOOL value. If YES, the device supports function pointers from compute pipelines. If NO,
 the device does not.
 */
- (BOOL)supportsFunctionPointers API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supportsFunctionPointers;
}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_12_0
/*!
 @property supportsFunctionPointersFromRender
 @abstract Query device support for using function pointers from render pipeline stages.
 @return BOOL value. If YES, the device supports function pointers from render pipeline stages. If
 NO, the device does not.
 */
- (BOOL)supportsFunctionPointersFromRender API_AVAILABLE(macos(12.0), ios(15.0))
{
  return self.real.supportsFunctionPointersFromRender;
}
#endif

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_12_0
/*!
 @property supportsRaytracingFromRender
 @abstract Query device support for using ray tracing from render pipeline stages.
 @return BOOL value. If YES, the device supports ray tracing from render pipeline stages. If NO, the
 device does not.
 */
- (BOOL)supportsRaytracingFromRender API_AVAILABLE(macos(12.0), ios(15.0))
{
  return self.real.supportsRaytracingFromRender;
}
#endif

// Treat as if the API is available from SDK 12.0
// It is marked as available from SDK 11.0, however it was not present in SDK 11.1 MTLDevice.h
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_12_0
/*!
 @property supportsPrimitiveMotionBlur
 @abstract Query device support for using ray tracing primitive motion blur.
 @return BOOL value. If YES, the device supports the primitive motion blur api. If NO, the device
 does not.
 */
- (BOOL)supportsPrimitiveMotionBlur API_AVAILABLE(macos(11.0), ios(14.0))
{
  return self.real.supportsPrimitiveMotionBlur;
}
#endif

@end
