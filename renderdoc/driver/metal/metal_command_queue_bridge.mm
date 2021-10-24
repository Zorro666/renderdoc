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

#include "metal_command_queue.h"
#include "metal_types_bridge.h"

// Wrapper for MTLCommandQueue
@implementation ObjCWrappedMTLCommandQueue

// ObjCWrappedMTLCommandQueue specific
- (id<MTLCommandQueue>)real
{
  MTL::CommandQueue *real = Unwrap<MTL::CommandQueue *>(self.wrappedCPP);
  return id<MTLCommandQueue>(real);
}

// MTLCommandQueue
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

- (nullable id<MTLCommandBuffer>)commandBuffer
{
  return id<MTLCommandBuffer>(self.wrappedCPP->commandBuffer());
}

- (nullable id<MTLCommandBuffer>)commandBufferWithDescriptor:(MTLCommandBufferDescriptor *)descriptor
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real commandBufferWithDescriptor:descriptor];
}

- (nullable id<MTLCommandBuffer>)commandBufferWithUnretainedReferences
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real commandBufferWithUnretainedReferences];
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
- (void)insertDebugCaptureBoundary
    API_DEPRECATED("Use MTLCaptureScope instead", macos(10.11, 10.13), ios(8.0, 11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real insertDebugCaptureBoundary];
}
#pragma clang diagnostic pop

@end
