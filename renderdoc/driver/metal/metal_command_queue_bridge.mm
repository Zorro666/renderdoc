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

#include "metal_command_queue_bridge.h"
#include "metal_command_buffer.h"
#include "metal_command_buffer_bridge.h"
#include "metal_command_queue.h"

id_MTLCommandQueue WrappedMTLCommandQueue::CreateObjCWrappedMTLCommandQueue()
{
  ObjCWrappedMTLCommandQueue *objCWrappedMTLCommandQueue = [ObjCWrappedMTLCommandQueue alloc];
  objCWrappedMTLCommandQueue.wrappedMTLCommandQueue = this;
  return objCWrappedMTLCommandQueue;
}

id_MTLCommandBuffer WrappedMTLCommandQueue::real_commandBuffer()
{
  id_MTLCommandQueue realMTLCommandQueue = Unwrap<id_MTLCommandQueue>(this);
  id_MTLCommandBuffer realMTLCommandBuffer = [realMTLCommandQueue commandBuffer];
  return realMTLCommandBuffer;
}

// Wrapper for MTLCommandQueue
@implementation ObjCWrappedMTLCommandQueue

// ObjCWrappedMTLCommandQueue specific
- (id<MTLCommandQueue>)realMTLCommandQueue
{
  id_MTLCommandQueue realMTLCommandQueue = Unwrap<id_MTLCommandQueue>(self.wrappedMTLCommandQueue);
  return realMTLCommandQueue;
}

// MTLCommandQueue
- (nullable NSString *)label
{
  return self.realMTLCommandQueue.label;
}

- (void)setLabel:value
{
  self.realMTLCommandQueue.label = value;
}

- (id<MTLDevice>)device
{
  return self.wrappedMTLCommandQueue->GetObjCWrappedMTLDevice();
}

- (nullable id<MTLCommandBuffer>)commandBuffer
{
  return self.wrappedMTLCommandQueue->commandBuffer();
}

- (nullable id<MTLCommandBuffer>)commandBufferWithDescriptor:(MTLCommandBufferDescriptor *)descriptor
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLCommandQueue commandBufferWithDescriptor:descriptor];
}

- (nullable id<MTLCommandBuffer>)commandBufferWithUnretainedReferences
{
  return [self.realMTLCommandQueue commandBufferWithUnretainedReferences];
}

- (void)insertDebugCaptureBoundary
    API_DEPRECATED("Use MTLCaptureScope instead", macos(10.11, 10.13), ios(8.0, 11.0))
{
  return [self.realMTLCommandQueue insertDebugCaptureBoundary];
}

@end
