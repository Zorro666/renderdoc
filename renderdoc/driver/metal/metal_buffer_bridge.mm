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

#include "metal_buffer.h"
#include "metal_types_bridge.h"

// Wrapper for MTLBuffer
@implementation ObjCWrappedMTLBuffer

// ObjCWrappedMTLBuffer specific
- (id<MTLBuffer>)real
{
  MTL::Buffer *real = Unwrap<MTL::Buffer *>(self.wrappedCPP);
  return id<MTLBuffer>(real);
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

// MTLBuffer
- (NSUInteger)length
{
  return self.real.length;
}

- (id<MTLBuffer>)remoteStorageBuffer
{
  // TODO: This should be the wrapped MTLBuffer
  return self.real.remoteStorageBuffer;
}

- (void *)contents NS_RETURNS_INNER_POINTER
{
  return self.wrappedCPP->contents();
}

- (void)didModifyRange:(NSRange)range API_AVAILABLE(macos(10.11), macCatalyst(13.0))
                           API_UNAVAILABLE(ios)
{
  return self.wrappedCPP->didModifyRange((NS::Range &)range);
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
                                             offset:(NSUInteger)offset
                                        bytesPerRow:(NSUInteger)bytesPerRow
    API_AVAILABLE(macos(10.13), ios(8.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newTextureWithDescriptor:descriptor offset:offset bytesPerRow:bytesPerRow];
}

- (void)addDebugMarker:(NSString *)marker
                 range:(NSRange)range API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real addDebugMarker:marker range:range];
}

- (void)removeAllDebugMarkers API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real removeAllDebugMarkers];
}

- (nullable id<MTLBuffer>)newRemoteBufferViewForDevice:(id<MTLDevice>)device
    API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRemoteBufferViewForDevice:device];
}

@end
