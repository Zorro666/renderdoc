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

#include "metal_command_buffer.h"
#include "metal_types_bridge.h"

// Wrapper for MTLCommandBuffer
@implementation ObjCWrappedMTLCommandBuffer

// ObjCWrappedMTLCommandBuffer specific
- (id<MTLCommandBuffer>)real
{
  MTL::CommandBuffer *real = Unwrap<MTL::CommandBuffer *>(self.wrappedCPP);
  return id<MTLCommandBuffer>(real);
}

- (void)dealloc
{
  self.wrappedCPP->Dealloc();
  [super dealloc];
}

// MTLCommandBuffer
- (id<MTLDevice>)device
{
  return id<MTLDevice>(self.wrappedCPP->GetObjCWrappedMTLDevice());
}

- (id<MTLCommandQueue>)commandQueue
{
  return id<MTLCommandQueue>(self.wrappedCPP->GetObjCWrappedMTLCommandQueue());
}

- (BOOL)retainedReferences
{
  return self.real.retainedReferences;
}

- (MTLCommandBufferErrorOption)errorOptions
{
  if(@available(macOS 11.0, *))
  {
    return self.real.errorOptions;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (NSString *)label
{
  return self.real.label;
}

- (void)setLabel:value
{
  self.real.label = value;
}

- (CFTimeInterval)kernelStartTime
{
  return self.real.kernelStartTime;
}

- (CFTimeInterval)kernelEndTime
{
  return self.real.kernelEndTime;
}

- (id<MTLLogContainer>)logs API_AVAILABLE(macos(11.0))
{
  return self.real.logs;
}

- (CFTimeInterval)GPUStartTime
{
  return self.real.GPUStartTime;
}

- (CFTimeInterval)GPUEndTime
{
  return self.real.GPUEndTime;
}

- (MTLCommandBufferStatus)status
{
  return self.real.status;
}

- (NSError *)error
{
  return self.real.error;
}

- (void)enqueue
{
  self.wrappedCPP->enqueue();
}

- (void)commit
{
  self.wrappedCPP->commit();
}

- (void)addScheduledHandler:(MTLCommandBufferHandler)block
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real addScheduledHandler:block];
}

- (void)presentDrawable:(id<MTLDrawable>)drawable
{
  self.wrappedCPP->presentDrawable((MTL::Drawable *)drawable);
}

- (void)presentDrawable:(id<MTLDrawable>)drawable atTime:(CFTimeInterval)presentationTime
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real presentDrawable:drawable atTime:presentationTime];
}

- (void)presentDrawable:(id<MTLDrawable>)drawable
    afterMinimumDuration:(CFTimeInterval)duration
    API_AVAILABLE(macos(10.15.4), ios(10.3), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real presentDrawable:drawable afterMinimumDuration:duration];
}

- (void)waitUntilScheduled
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real waitUntilScheduled];
}

- (void)addCompletedHandler:(MTLCommandBufferHandler)block
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real addCompletedHandler:block];
}

- (void)waitUntilCompleted
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real waitUntilCompleted];
}

- (nullable id<MTLBlitCommandEncoder>)blitCommandEncoder
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real blitCommandEncoder];
}

- (nullable id<MTLRenderCommandEncoder>)renderCommandEncoderWithDescriptor:
    (MTLRenderPassDescriptor *)renderPassDescriptor
{
  return id<MTLRenderCommandEncoder>(self.wrappedCPP->renderCommandEncoderWithDescriptor(
      (MTL::RenderPassDescriptor *)renderPassDescriptor));
}

- (nullable id<MTLComputeCommandEncoder>)computeCommandEncoderWithDescriptor:
    (MTLComputePassDescriptor *)computePassDescriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real computeCommandEncoderWithDescriptor:computePassDescriptor];
}

- (nullable id<MTLBlitCommandEncoder>)blitCommandEncoderWithDescriptor:
    (MTLBlitPassDescriptor *)blitPassDescriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real blitCommandEncoderWithDescriptor:blitPassDescriptor];
}

- (nullable id<MTLComputeCommandEncoder>)computeCommandEncoder
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real computeCommandEncoder];
}

- (nullable id<MTLComputeCommandEncoder>)computeCommandEncoderWithDispatchType:
    (MTLDispatchType)dispatchType API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real computeCommandEncoderWithDispatchType:dispatchType];
}

- (void)encodeWaitForEvent:(id<MTLEvent>)event
                     value:(uint64_t)value API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real encodeWaitForEvent:event value:value];
}

- (void)encodeSignalEvent:(id<MTLEvent>)event
                    value:(uint64_t)value API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real encodeSignalEvent:event value:value];
}

- (nullable id<MTLParallelRenderCommandEncoder>)parallelRenderCommandEncoderWithDescriptor:
    (MTLRenderPassDescriptor *)renderPassDescriptor
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real parallelRenderCommandEncoderWithDescriptor:renderPassDescriptor];
}

- (nullable id<MTLResourceStateCommandEncoder>)resourceStateCommandEncoder
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real resourceStateCommandEncoder];
}

- (nullable id<MTLResourceStateCommandEncoder>)resourceStateCommandEncoderWithDescriptor:
    (MTLResourceStatePassDescriptor *)resourceStatePassDescriptor
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real resourceStateCommandEncoderWithDescriptor:resourceStatePassDescriptor];
}

- (nullable id<MTLAccelerationStructureCommandEncoder>)accelerationStructureCommandEncoder
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real accelerationStructureCommandEncoder];
}

- (void)pushDebugGroup:(NSString *)string API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real pushDebugGroup:string];
}

- (void)popDebugGroup API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real popDebugGroup];
}

@end
