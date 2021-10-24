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

#include "metal_texture.h"
#include "metal_types_bridge.h"

// Wrapper for MTLTexture
@implementation ObjCWrappedMTLTexture

// ObjCWrappedMTLTexture specific
- (id<MTLTexture>)real
{
  MTL::Texture *real = Unwrap<MTL::Texture *>(self.wrappedCPP);
  return id<MTLTexture>(real);
}

- (void)dealloc
{
  self.wrappedCPP->Dealloc();
  [super dealloc];
}

// MTLResource
- (nullable NSString *)label
{
  return self.real.label;
}

- (void)setLabel:value
{
  self.real.label = value;
}

- (id<MTLDevice>)device
{
  return id<MTLDevice>(self.wrappedCPP->GetObjCWrappedMTLDevice());
}

- (MTLCPUCacheMode)cpuCacheMode
{
  return self.real.cpuCacheMode;
}

- (MTLStorageMode)storageMode
{
  return self.real.storageMode;
}

- (MTLHazardTrackingMode)hazardTrackingMode
{
  return self.real.hazardTrackingMode;
}

- (MTLResourceOptions)resourceOptions
{
  return self.real.resourceOptions;
}

- (id<MTLHeap>)heap
{
  return self.real.heap;
}

- (NSUInteger)heapOffset
{
  return self.real.heapOffset;
}

- (NSUInteger)allocatedSize
{
  return self.real.allocatedSize;
}

- (MTLPurgeableState)setPurgeableState:(MTLPurgeableState)state
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setPurgeableState:state];
}

- (void)makeAliasable API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real makeAliasable];
}

- (BOOL)isAliasable API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real isAliasable];
}
// MTLTexture
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
- (id<MTLResource>)rootResource
{
  return self.real.rootResource;
}
#pragma clang diagnostic pop

- (id<MTLTexture>)parentTexture API_AVAILABLE(macos(10.11), ios(9.0))
{
  return self.real.parentTexture;
}

- (NSUInteger)parentRelativeLevel API_AVAILABLE(macos(10.11), ios(9.0))
{
  return self.real.parentRelativeLevel;
}

- (NSUInteger)parentRelativeSlice API_AVAILABLE(macos(10.11), ios(9.0))
{
  return self.real.parentRelativeSlice;
}

- (id<MTLBuffer>)buffer API_AVAILABLE(macos(10.12), ios(9.0))
{
  return self.real.buffer;
}

- (NSUInteger)bufferOffset API_AVAILABLE(macos(10.12), ios(9.0))
{
  return self.real.bufferOffset;
}

- (NSUInteger)bufferBytesPerRow API_AVAILABLE(macos(10.12), ios(9.0))
{
  return self.real.bufferBytesPerRow;
}

- (IOSurfaceRef)iosurface API_AVAILABLE(macos(10.11), ios(11.0))
{
  return self.real.iosurface;
}

- (NSUInteger)iosurfacePlane API_AVAILABLE(macos(10.11), ios(11.0))
{
  return self.real.iosurfacePlane;
}

- (MTLTextureType)textureType
{
  return self.real.textureType;
}

- (MTLPixelFormat)pixelFormat
{
  return self.real.pixelFormat;
}

- (NSUInteger)width
{
  return self.real.width;
}

- (NSUInteger)height
{
  return self.real.height;
}

- (NSUInteger)depth
{
  return self.real.depth;
}

- (NSUInteger)mipmapLevelCount
{
  return self.real.mipmapLevelCount;
}

- (NSUInteger)sampleCount
{
  return self.real.sampleCount;
}

- (NSUInteger)arrayLength
{
  return self.real.arrayLength;
}

- (MTLTextureUsage)usage
{
  return self.real.usage;
}

- (BOOL)isShareable API_AVAILABLE(macos(10.14), ios(13.0))
{
  return self.real.isShareable;
}

- (BOOL)isFramebufferOnly
{
  return self.real.isFramebufferOnly;
}

- (NSUInteger)firstMipmapInTail API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  if(@available(macOS 11.0, *))
  {
    return self.real.firstMipmapInTail;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (NSUInteger)tailSizeInBytes API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  if(@available(macOS 11.0, *))
  {
    return self.real.tailSizeInBytes;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (BOOL)isSparse API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  if(@available(macOS 11.0, *))
  {
    return self.real.isSparse;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)allowGPUOptimizedContents API_AVAILABLE(macos(10.14), ios(12.0))
{
  return self.real.allowGPUOptimizedContents;
}

- (void)getBytes:(void *)pixelBytes
      bytesPerRow:(NSUInteger)bytesPerRow
    bytesPerImage:(NSUInteger)bytesPerImage
       fromRegion:(MTLRegion)region
      mipmapLevel:(NSUInteger)level
            slice:(NSUInteger)slice
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  [self.real getBytes:pixelBytes
          bytesPerRow:bytesPerRow
        bytesPerImage:bytesPerImage
           fromRegion:region
          mipmapLevel:level
                slice:slice];
}

- (void)replaceRegion:(MTLRegion)region
          mipmapLevel:(NSUInteger)level
                slice:(NSUInteger)slice
            withBytes:(const void *)pixelBytes
          bytesPerRow:(NSUInteger)bytesPerRow
        bytesPerImage:(NSUInteger)bytesPerImage
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  [self.real replaceRegion:region
               mipmapLevel:level
                     slice:slice
                 withBytes:pixelBytes
               bytesPerRow:bytesPerRow
             bytesPerImage:bytesPerImage];
}

- (void)getBytes:(void *)pixelBytes
     bytesPerRow:(NSUInteger)bytesPerRow
      fromRegion:(MTLRegion)region
     mipmapLevel:(NSUInteger)level
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  [self.real getBytes:pixelBytes bytesPerRow:bytesPerRow fromRegion:region mipmapLevel:level];
}

- (void)replaceRegion:(MTLRegion)region
          mipmapLevel:(NSUInteger)level
            withBytes:(const void *)pixelBytes
          bytesPerRow:(NSUInteger)bytesPerRow
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  [self.real replaceRegion:region mipmapLevel:level withBytes:pixelBytes bytesPerRow:bytesPerRow];
}

- (nullable id<MTLTexture>)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newTextureViewWithPixelFormat:pixelFormat];
}

- (nullable id<MTLTexture>)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
                                             textureType:(MTLTextureType)textureType
                                                  levels:(NSRange)levelRange
                                                  slices:(NSRange)sliceRange
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newTextureViewWithPixelFormat:pixelFormat
                                      textureType:textureType
                                           levels:levelRange
                                           slices:sliceRange];
}

- (nullable MTLSharedTextureHandle *)newSharedTextureHandle API_AVAILABLE(macos(10.14), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newSharedTextureHandle];
}

- (id<MTLTexture>)remoteStorageTexture API_AVAILABLE(macos(10.15))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real remoteStorageTexture];
}

- (nullable id<MTLTexture>)newRemoteTextureViewForDevice:(id<MTLDevice>)device
    API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRemoteTextureViewForDevice:device];
}

- (MTLTextureSwizzleChannels)swizzle API_AVAILABLE(macos(10.15), ios(13.0))
{
  return self.real.swizzle;
}

- (nullable id<MTLTexture>)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
                                             textureType:(MTLTextureType)textureType
                                                  levels:(NSRange)levelRange
                                                  slices:(NSRange)sliceRange
                                                 swizzle:(MTLTextureSwizzleChannels)swizzle
    API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newTextureViewWithPixelFormat:pixelFormat
                                      textureType:textureType
                                           levels:levelRange
                                           slices:sliceRange
                                          swizzle:swizzle];
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

@end
