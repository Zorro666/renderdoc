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

#include "metal_texture_bridge.h"
#import <Metal/MTLTexture.h>
#include "metal_texture.h"

static ObjCWrappedMTLTexture *GetObjC(id_MTLTexture texture)
{
  if(texture == NULL)
  {
    return NULL;
  }
  RDCASSERT([texture isKindOfClass:[ObjCWrappedMTLTexture class]]);
  ObjCWrappedMTLTexture *objC = (ObjCWrappedMTLTexture *)texture;
  return objC;
}

id_MTLTexture GetReal(id_MTLTexture texture)
{
  ObjCWrappedMTLTexture *objC = GetObjC(texture);
  id_MTLTexture realTexture = objC.realMTLTexture;
  return realTexture;
}

ResourceId GetId(id_MTLTexture texture)
{
  WrappedMTLTexture *wrapped = GetWrapped(texture);
  if(wrapped == NULL)
  {
    return ResourceId();
  }
  return wrapped->id;
}

WrappedMTLTexture *GetWrapped(id_MTLTexture texture)
{
  ObjCWrappedMTLTexture *objCWrappedMTLTexture = GetObjC(texture);
  if(objCWrappedMTLTexture == NULL)
  {
    return NULL;
  }
  return [objCWrappedMTLTexture wrappedMTLTexture];
}

id_MTLTexture WrappedMTLTexture::CreateObjCWrappedMTLTexture()
{
  ObjCWrappedMTLTexture *objCWrappedMTLTexture = [ObjCWrappedMTLTexture new];
  objCWrappedMTLTexture.wrappedMTLTexture = this;
  return objCWrappedMTLTexture;
}

// Wrapper for MTLTexture
@implementation ObjCWrappedMTLTexture

// ObjCWrappedMTLTexture specific
- (id<MTLTexture>)realMTLTexture
{
  id_MTLTexture realMTLTexture = Unwrap<id_MTLTexture>(self.wrappedMTLTexture);
  return realMTLTexture;
}

// MTLResource
- (nullable NSString *)label
{
  return self.realMTLTexture.label;
}

- (void)setLabel:value
{
  self.realMTLTexture.label = value;
}

- (id<MTLDevice>)device
{
  return self.wrappedMTLTexture->GetObjCWrappedMTLDevice();
}

- (MTLCPUCacheMode)cpuCacheMode
{
  return self.realMTLTexture.cpuCacheMode;
}

- (MTLStorageMode)storageMode
{
  return self.realMTLTexture.storageMode;
}

- (MTLHazardTrackingMode)hazardTrackingMode
{
  return self.realMTLTexture.hazardTrackingMode;
}

- (MTLResourceOptions)resourceOptions
{
  return self.realMTLTexture.resourceOptions;
}

- (id<MTLHeap>)heap
{
  return self.realMTLTexture.heap;
}

- (NSUInteger)heapOffset
{
  return self.realMTLTexture.heapOffset;
}

- (NSUInteger)allocatedSize
{
  return self.realMTLTexture.allocatedSize;
}

- (MTLPurgeableState)setPurgeableState:(MTLPurgeableState)state
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture setPurgeableState:state];
}

- (void)makeAliasable API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture makeAliasable];
}

- (BOOL)isAliasable API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture isAliasable];
}
// MTLTexture
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
- (id<MTLResource>)rootResource
{
  return self.realMTLTexture.rootResource;
}
#pragma clang diagnostic pop

- (id<MTLTexture>)parentTexture API_AVAILABLE(macos(10.11), ios(9.0))
{
  return self.realMTLTexture.parentTexture;
}

- (NSUInteger)parentRelativeLevel API_AVAILABLE(macos(10.11), ios(9.0))
{
  return self.realMTLTexture.parentRelativeLevel;
}

- (NSUInteger)parentRelativeSlice API_AVAILABLE(macos(10.11), ios(9.0))
{
  return self.realMTLTexture.parentRelativeSlice;
}

- (id<MTLBuffer>)buffer API_AVAILABLE(macos(10.12), ios(9.0))
{
  return self.realMTLTexture.buffer;
}

- (NSUInteger)bufferOffset API_AVAILABLE(macos(10.12), ios(9.0))
{
  return self.realMTLTexture.bufferOffset;
}

- (NSUInteger)bufferBytesPerRow API_AVAILABLE(macos(10.12), ios(9.0))
{
  return self.realMTLTexture.bufferBytesPerRow;
}

- (IOSurfaceRef)iosurface API_AVAILABLE(macos(10.11), ios(11.0))
{
  return self.realMTLTexture.iosurface;
}

- (NSUInteger)iosurfacePlane API_AVAILABLE(macos(10.11), ios(11.0))
{
  return self.realMTLTexture.iosurfacePlane;
}

- (MTLTextureType)textureType
{
  return self.realMTLTexture.textureType;
}

- (MTLPixelFormat)pixelFormat
{
  return self.realMTLTexture.pixelFormat;
}

- (NSUInteger)width
{
  return self.realMTLTexture.width;
}

- (NSUInteger)height
{
  return self.realMTLTexture.height;
}

- (NSUInteger)depth
{
  return self.realMTLTexture.depth;
}

- (NSUInteger)mipmapLevelCount
{
  return self.realMTLTexture.mipmapLevelCount;
}

- (NSUInteger)sampleCount
{
  return self.realMTLTexture.sampleCount;
}

- (NSUInteger)arrayLength
{
  return self.realMTLTexture.arrayLength;
}

- (MTLTextureUsage)usage
{
  return self.realMTLTexture.usage;
}

- (BOOL)isShareable API_AVAILABLE(macos(10.14), ios(13.0))
{
  return self.realMTLTexture.isShareable;
}

- (BOOL)isFramebufferOnly
{
  return self.realMTLTexture.isFramebufferOnly;
}

- (NSUInteger)firstMipmapInTail API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLTexture.firstMipmapInTail;
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
    return self.realMTLTexture.tailSizeInBytes;
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
    return self.realMTLTexture.isSparse;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (BOOL)allowGPUOptimizedContents API_AVAILABLE(macos(10.14), ios(12.0))
{
  return self.realMTLTexture.allowGPUOptimizedContents;
}

- (void)getBytes:(void *)pixelBytes
      bytesPerRow:(NSUInteger)bytesPerRow
    bytesPerImage:(NSUInteger)bytesPerImage
       fromRegion:(MTLRegion)region
      mipmapLevel:(NSUInteger)level
            slice:(NSUInteger)slice
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  [self.realMTLTexture getBytes:pixelBytes
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
  [self.realMTLTexture replaceRegion:region
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
  [self.realMTLTexture getBytes:pixelBytes
                    bytesPerRow:bytesPerRow
                     fromRegion:region
                    mipmapLevel:level];
}

- (void)replaceRegion:(MTLRegion)region
          mipmapLevel:(NSUInteger)level
            withBytes:(const void *)pixelBytes
          bytesPerRow:(NSUInteger)bytesPerRow
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  [self.realMTLTexture replaceRegion:region
                         mipmapLevel:level
                           withBytes:pixelBytes
                         bytesPerRow:bytesPerRow];
}

- (nullable id<MTLTexture>)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture newTextureViewWithPixelFormat:pixelFormat];
}

- (nullable id<MTLTexture>)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
                                             textureType:(MTLTextureType)textureType
                                                  levels:(NSRange)levelRange
                                                  slices:(NSRange)sliceRange
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture newTextureViewWithPixelFormat:pixelFormat
                                                textureType:textureType
                                                     levels:levelRange
                                                     slices:sliceRange];
}

- (nullable MTLSharedTextureHandle *)newSharedTextureHandle API_AVAILABLE(macos(10.14), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture newSharedTextureHandle];
}

- (id<MTLTexture>)remoteStorageTexture API_AVAILABLE(macos(10.15))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture remoteStorageTexture];
}

- (nullable id<MTLTexture>)newRemoteTextureViewForDevice:(id<MTLDevice>)device
    API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture newRemoteTextureViewForDevice:device];
}

- (MTLTextureSwizzleChannels)swizzle API_AVAILABLE(macos(10.15), ios(13.0))
{
  return self.realMTLTexture.swizzle;
}

- (nullable id<MTLTexture>)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat
                                             textureType:(MTLTextureType)textureType
                                                  levels:(NSRange)levelRange
                                                  slices:(NSRange)sliceRange
                                                 swizzle:(MTLTextureSwizzleChannels)swizzle
    API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLTexture newTextureViewWithPixelFormat:pixelFormat
                                                textureType:textureType
                                                     levels:levelRange
                                                     slices:sliceRange
                                                    swizzle:swizzle];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
  id fwd = self.realMTLTexture;
  return [fwd methodSignatureForSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
  SEL aSelector = [invocation selector];

  if([self.realMTLTexture respondsToSelector:aSelector])
    [invocation invokeWithTarget:self.realMTLTexture];
  else
    [super forwardInvocation:invocation];
}

@end
