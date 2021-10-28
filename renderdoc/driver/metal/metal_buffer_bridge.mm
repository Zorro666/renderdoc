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

#include "metal_buffer_bridge.h"
#include "metal_buffer.h"

static ObjCWrappedMTLBuffer *GetObjC(id_MTLBuffer buffer)
{
  if(buffer == NULL)
  {
    return NULL;
  }
  RDCASSERT([buffer isKindOfClass:[ObjCWrappedMTLBuffer class]]);
  ObjCWrappedMTLBuffer *objC = (ObjCWrappedMTLBuffer *)buffer;
  return objC;
}

id_MTLBuffer GetReal(id_MTLBuffer buffer)
{
  ObjCWrappedMTLBuffer *objC = GetObjC(buffer);
  id_MTLBuffer realBuffer = objC.realMTLBuffer;
  return realBuffer;
}

WrappedMTLBuffer *GetWrapped(id_MTLBuffer buffer)
{
  ObjCWrappedMTLBuffer *objC = GetObjC(buffer);
  return [objC wrappedMTLBuffer];
}

id_MTLBuffer WrappedMTLBuffer::CreateObjWrappedMTLBuffer()
{
  ObjCWrappedMTLBuffer *objCWrappedMTLBuffer = [ObjCWrappedMTLBuffer new];
  objCWrappedMTLBuffer.wrappedMTLBuffer = this;
  return objCWrappedMTLBuffer;
}

void *WrappedMTLBuffer::real_contents()
{
  id_MTLBuffer realMTLBuffer = Unwrap<id_MTLBuffer>(this);
  return [realMTLBuffer contents];
}

NSUInteger WrappedMTLBuffer::real_length()
{
  id_MTLBuffer realMTLBuffer = Unwrap<id_MTLBuffer>(this);
  return [realMTLBuffer length];
}

void WrappedMTLBuffer::real_didModifyRange(NSRange &range)
{
  id_MTLBuffer realMTLBuffer = Unwrap<id_MTLBuffer>(this);
  return [realMTLBuffer didModifyRange:range];
}

// Wrapper for MTLBuffer
@implementation ObjCWrappedMTLBuffer

// ObjCWrappedMTLBuffer specific
- (id<MTLBuffer>)realMTLBuffer
{
  id_MTLBuffer realMTLBuffer = Unwrap<id_MTLBuffer>(self.wrappedMTLBuffer);
  return realMTLBuffer;
}

// MTLResource
- (nullable NSString *)label
{
  return self.realMTLBuffer.label;
}

- (void)setLabel:value
{
  self.realMTLBuffer.label = value;
}

- (id<MTLDevice>)device
{
  return self.wrappedMTLBuffer->GetObjCWrappedMTLDevice();
}

- (MTLCPUCacheMode)cpuCacheMode
{
  return self.realMTLBuffer.cpuCacheMode;
}

- (MTLStorageMode)storageMode
{
  return self.realMTLBuffer.storageMode;
}

- (MTLHazardTrackingMode)hazardTrackingMode
{
  return self.realMTLBuffer.hazardTrackingMode;
}

- (MTLResourceOptions)resourceOptions
{
  return self.realMTLBuffer.resourceOptions;
}

- (id<MTLHeap>)heap
{
  return self.realMTLBuffer.heap;
}

- (NSUInteger)heapOffset
{
  return self.realMTLBuffer.heapOffset;
}

- (NSUInteger)allocatedSize
{
  return self.realMTLBuffer.allocatedSize;
}

- (MTLPurgeableState)setPurgeableState:(MTLPurgeableState)state
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLBuffer setPurgeableState:state];
}

- (void)makeAliasable API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLBuffer makeAliasable];
}

- (BOOL)isAliasable API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLBuffer isAliasable];
}

// MTLBuffer
- (NSUInteger)length
{
  return self.realMTLBuffer.length;
}

- (id<MTLBuffer>)remoteStorageBuffer
{
  // TODO: This should be the wrapped MTLBuffer
  return self.realMTLBuffer.remoteStorageBuffer;
}

- (void *)contents NS_RETURNS_INNER_POINTER
{
  return self.wrappedMTLBuffer->contents();
}

- (void)didModifyRange:(NSRange)range API_AVAILABLE(macos(10.11), macCatalyst(13.0))
                           API_UNAVAILABLE(ios)
{
  return self.wrappedMTLBuffer->didModifyRange(range);
}

- (nullable id<MTLTexture>)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor
                                             offset:(NSUInteger)offset
                                        bytesPerRow:(NSUInteger)bytesPerRow
    API_AVAILABLE(macos(10.13), ios(8.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLBuffer newTextureWithDescriptor:descriptor
                                               offset:offset
                                          bytesPerRow:bytesPerRow];
}

- (void)addDebugMarker:(NSString *)marker
                 range:(NSRange)range API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLBuffer addDebugMarker:marker range:range];
}

- (void)removeAllDebugMarkers API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLBuffer removeAllDebugMarkers];
}

- (nullable id<MTLBuffer>)newRemoteBufferViewForDevice:(id<MTLDevice>)device
    API_AVAILABLE(macos(10.15))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLBuffer newRemoteBufferViewForDevice:device];
}

@end
