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

#include "metal_command_buffer_bridge.h"
#include "metal_command_buffer.h"
#include "metal_render_command_encoder.h"
#include "metal_render_command_encoder_bridge.h"
#include "metal_texture_bridge.h"

#import <Metal/MTLBlitCommandEncoder.h>

void MTL::CopyTexture(WrappedMTLCommandBuffer *commandBuffer, id_MTLTexture source,
                      id_MTLTexture destination)
{
  id_MTLCommandBuffer realMTLCommandBuffer = Unwrap<id_MTLCommandBuffer>(commandBuffer);
  id<MTLBlitCommandEncoder> blitCommandEncoder = [realMTLCommandBuffer blitCommandEncoder];

  id_MTLTexture realSource = GetReal(source);
  id_MTLTexture realDestination = GetReal(destination);
  [blitCommandEncoder copyFromTexture:realSource toTexture:realDestination];
  [blitCommandEncoder endEncoding];
}

WrappedMTLCommandBuffer *GetWrapped(id_MTLCommandBuffer commandBuffer)
{
  RDCASSERT([commandBuffer isKindOfClass:[ObjCWrappedMTLCommandBuffer class]]);

  ObjCWrappedMTLCommandBuffer *objCWrappedMTLCommandBuffer =
      (ObjCWrappedMTLCommandBuffer *)commandBuffer;
  return [objCWrappedMTLCommandBuffer wrappedMTLCommandBuffer];
}

id_MTLCommandBuffer WrappedMTLCommandBuffer::CreateObjCWrappedMTLCommandBuffer()
{
  ObjCWrappedMTLCommandBuffer *objCWrappedMTLCommandBuffer = [ObjCWrappedMTLCommandBuffer new];
  objCWrappedMTLCommandBuffer.wrappedMTLCommandBuffer = this;
  return objCWrappedMTLCommandBuffer;
}

id_MTLRenderCommandEncoder WrappedMTLCommandBuffer::real_renderCommandEncoderWithDescriptor(
    MTLRenderPassDescriptor *descriptor)
{
  id_MTLCommandBuffer realMTLCommandBuffer = Unwrap<id_MTLCommandBuffer>(this);

  MTLRenderPassDescriptor *realDescriptor = [descriptor copy];

  // The source descriptor contains wrapped MTLTexture resources
  // Need to unwrap them to the real resource before calling real API
  for(uint32_t i = 0; i < MAX_RENDER_PASS_COLOR_ATTACHMENTS; ++i)
  {
    id_MTLTexture wrappedTexture = descriptor.colorAttachments[i].texture;
    if(wrappedTexture != NULL)
    {
      if([wrappedTexture isKindOfClass:[ObjCWrappedMTLTexture class]])
      {
        realDescriptor.colorAttachments[i].texture = GetReal(wrappedTexture);
      }
    }
  }

  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder =
      [realMTLCommandBuffer renderCommandEncoderWithDescriptor:realDescriptor];

  [realDescriptor dealloc];

  return realMTLRenderCommandEncoder;
}

void WrappedMTLCommandBuffer::real_presentDrawable(id_MTLDrawable drawable)
{
  id_MTLCommandBuffer realMTLCommandBuffer = Unwrap<id_MTLCommandBuffer>(this);
  [realMTLCommandBuffer presentDrawable:drawable];
}

void WrappedMTLCommandBuffer::real_enqueue()
{
  id_MTLCommandBuffer realMTLCommandBuffer = Unwrap<id_MTLCommandBuffer>(this);
  [realMTLCommandBuffer enqueue];
}

void WrappedMTLCommandBuffer::real_label(const char *label)
{
  id_MTLCommandBuffer realMTLCommandBuffer = Unwrap<id_MTLCommandBuffer>(this);
  NSString *nsLabel = MTL::NewNSStringFromUTF8(label);
  [realMTLCommandBuffer setLabel:nsLabel];
}

void WrappedMTLCommandBuffer::real_waitUntilCompleted()
{
  id_MTLCommandBuffer realMTLCommandBuffer = Unwrap<id_MTLCommandBuffer>(this);
  [realMTLCommandBuffer waitUntilCompleted];
}

void WrappedMTLCommandBuffer::real_commit()
{
  id_MTLCommandBuffer realMTLCommandBuffer = Unwrap<id_MTLCommandBuffer>(this);
  [realMTLCommandBuffer commit];
}

// Wrapper for MTLCommandBuffer
@implementation ObjCWrappedMTLCommandBuffer

// ObjCWrappedMTLCommandBuffer specific
- (id<MTLCommandBuffer>)realMTLCommandBuffer
{
  id_MTLCommandBuffer realMTLCommandBuffer =
      Unwrap<id_MTLCommandBuffer>(self.wrappedMTLCommandBuffer);
  return realMTLCommandBuffer;
}

// MTLCommandBuffer
- (id<MTLDevice>)device
{
  return self.wrappedMTLCommandBuffer->GetObjCWrappedMTLDevice();
}

- (id<MTLCommandQueue>)commandQueue
{
  return self.wrappedMTLCommandBuffer->GetObjCWrappedMTLCommandQueue();
}

- (BOOL)retainedReferences
{
  return self.realMTLCommandBuffer.retainedReferences;
}

- (MTLCommandBufferErrorOption)errorOptions
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLCommandBuffer.errorOptions;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (NSString *)label
{
  return self.realMTLCommandBuffer.label;
}

- (void)setLabel:value
{
  self.realMTLCommandBuffer.label = value;
}

- (CFTimeInterval)kernelStartTime
{
  return self.realMTLCommandBuffer.kernelStartTime;
}

- (CFTimeInterval)kernelEndTime
{
  return self.realMTLCommandBuffer.kernelEndTime;
}

- (id<MTLLogContainer>)logs API_AVAILABLE(macos(11.0))
{
  return self.realMTLCommandBuffer.logs;
}

- (CFTimeInterval)GPUStartTime
{
  return self.realMTLCommandBuffer.GPUStartTime;
}

- (CFTimeInterval)GPUEndTime
{
  return self.realMTLCommandBuffer.GPUEndTime;
}

- (MTLCommandBufferStatus)status
{
  return self.realMTLCommandBuffer.status;
}

- (NSError *)error
{
  return self.realMTLCommandBuffer.error;
}

- (void)enqueue
{
  self.wrappedMTLCommandBuffer->enqueue();
}

- (void)commit
{
  self.wrappedMTLCommandBuffer->commit();
}

- (void)addScheduledHandler:(MTLCommandBufferHandler)block
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer addScheduledHandler:block];
}

- (void)presentDrawable:(id<MTLDrawable>)drawable
{
  self.wrappedMTLCommandBuffer->presentDrawable(drawable);
}

- (void)presentDrawable:(id<MTLDrawable>)drawable atTime:(CFTimeInterval)presentationTime
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer presentDrawable:drawable atTime:presentationTime];
}

- (void)presentDrawable:(id<MTLDrawable>)drawable
    afterMinimumDuration:(CFTimeInterval)duration
    API_AVAILABLE(macos(10.15.4), ios(10.3), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer presentDrawable:drawable afterMinimumDuration:duration];
}

- (void)waitUntilScheduled
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer waitUntilScheduled];
}

- (void)addCompletedHandler:(MTLCommandBufferHandler)block
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer addCompletedHandler:block];
}

- (void)waitUntilCompleted
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer waitUntilCompleted];
}

- (nullable id<MTLBlitCommandEncoder>)blitCommandEncoder
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer blitCommandEncoder];
}

- (nullable id<MTLRenderCommandEncoder>)renderCommandEncoderWithDescriptor:
    (MTLRenderPassDescriptor *)renderPassDescriptor
{
  return self.wrappedMTLCommandBuffer->renderCommandEncoderWithDescriptor(renderPassDescriptor);
}

- (nullable id<MTLComputeCommandEncoder>)computeCommandEncoderWithDescriptor:
    (MTLComputePassDescriptor *)computePassDescriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer computeCommandEncoderWithDescriptor:computePassDescriptor];
}

- (nullable id<MTLBlitCommandEncoder>)blitCommandEncoderWithDescriptor:
    (MTLBlitPassDescriptor *)blitPassDescriptor API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer blitCommandEncoderWithDescriptor:blitPassDescriptor];
}

- (nullable id<MTLComputeCommandEncoder>)computeCommandEncoder
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer computeCommandEncoder];
}

- (nullable id<MTLComputeCommandEncoder>)computeCommandEncoderWithDispatchType:
    (MTLDispatchType)dispatchType API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer computeCommandEncoderWithDispatchType:dispatchType];
}

- (void)encodeWaitForEvent:(id<MTLEvent>)event
                     value:(uint64_t)value API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer encodeWaitForEvent:event value:value];
}

- (void)encodeSignalEvent:(id<MTLEvent>)event
                    value:(uint64_t)value API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer encodeSignalEvent:event value:value];
}

- (nullable id<MTLParallelRenderCommandEncoder>)parallelRenderCommandEncoderWithDescriptor:
    (MTLRenderPassDescriptor *)renderPassDescriptor
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer parallelRenderCommandEncoderWithDescriptor:renderPassDescriptor];
}

- (nullable id<MTLResourceStateCommandEncoder>)resourceStateCommandEncoder
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer resourceStateCommandEncoder];
}

- (nullable id<MTLResourceStateCommandEncoder>)resourceStateCommandEncoderWithDescriptor:
    (MTLResourceStatePassDescriptor *)resourceStatePassDescriptor
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer
      resourceStateCommandEncoderWithDescriptor:resourceStatePassDescriptor];
}

- (nullable id<MTLAccelerationStructureCommandEncoder>)accelerationStructureCommandEncoder
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer accelerationStructureCommandEncoder];
}

- (void)pushDebugGroup:(NSString *)string API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer pushDebugGroup:string];
}

- (void)popDebugGroup API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLCommandBuffer popDebugGroup];
}

@end
