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

#include "metal_render_command_encoder_bridge.h"
#include "core/core.h"
#include "metal_buffer.h"
#include "metal_buffer_bridge.h"
#include "metal_render_command_encoder.h"
#include "metal_render_pipeline_state.h"
#include "metal_render_pipeline_state_bridge.h"
#include "metal_texture_bridge.h"

static ObjCWrappedMTLRenderCommandEncoder *GetObjC(id_MTLRenderCommandEncoder encoder)
{
  RDCASSERT([encoder isKindOfClass:[ObjCWrappedMTLRenderCommandEncoder class]]);
  ObjCWrappedMTLRenderCommandEncoder *objC = (ObjCWrappedMTLRenderCommandEncoder *)encoder;
  return objC;
}

id_MTLRenderCommandEncoder GetReal(id_MTLRenderCommandEncoder encoder)
{
  ObjCWrappedMTLRenderCommandEncoder *objC = GetObjC(encoder);
  id_MTLRenderCommandEncoder realEncoder = objC.realMTLRenderCommandEncoder;
  return realEncoder;
}

WrappedMTLRenderCommandEncoder *GetWrapped(id_MTLRenderCommandEncoder encoder)
{
  ObjCWrappedMTLRenderCommandEncoder *objC = GetObjC(encoder);
  return [objC wrappedMTLRenderCommandEncoder];
}

id_MTLRenderCommandEncoder WrappedMTLRenderCommandEncoder::CreateObjCWrappedMTLRenderCommandEncoder()
{
  ObjCWrappedMTLRenderCommandEncoder *objCWrappedMTLRenderCommandEncoder =
      [ObjCWrappedMTLRenderCommandEncoder new];
  objCWrappedMTLRenderCommandEncoder.wrappedMTLRenderCommandEncoder = this;
  return objCWrappedMTLRenderCommandEncoder;
}

void WrappedMTLRenderCommandEncoder::real_setRenderPipelineState(id_MTLRenderPipelineState pipelineState)
{
  id_MTLRenderPipelineState realRenderPipelineState = GetReal(pipelineState);
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder = Unwrap<id_MTLRenderCommandEncoder>(this);
  [realMTLRenderCommandEncoder setRenderPipelineState:realRenderPipelineState];
}

void WrappedMTLRenderCommandEncoder::real_setVertexBuffer(id_MTLBuffer buffer, NSUInteger offset,
                                                          NSUInteger index)
{
  id_MTLBuffer realBuffer = GetReal(buffer);
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder = Unwrap<id_MTLRenderCommandEncoder>(this);
  [realMTLRenderCommandEncoder setVertexBuffer:realBuffer offset:offset atIndex:index];
}

void WrappedMTLRenderCommandEncoder::real_setFragmentBuffer(id_MTLBuffer buffer, NSUInteger offset,
                                                            NSUInteger index)
{
  id_MTLBuffer realBuffer = GetReal(buffer);
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder = Unwrap<id_MTLRenderCommandEncoder>(this);
  [realMTLRenderCommandEncoder setFragmentBuffer:realBuffer offset:offset atIndex:index];
}

void WrappedMTLRenderCommandEncoder::real_setFragmentTexture(id_MTLTexture texture, NSUInteger index)
{
  id_MTLTexture realTexture = GetReal(texture);
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder = Unwrap<id_MTLRenderCommandEncoder>(this);
  [realMTLRenderCommandEncoder setFragmentTexture:realTexture atIndex:index];
}

void WrappedMTLRenderCommandEncoder::real_setViewport(MTLViewport &viewport)
{
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder = Unwrap<id_MTLRenderCommandEncoder>(this);
  [realMTLRenderCommandEncoder setViewport:viewport];
}

void WrappedMTLRenderCommandEncoder::real_drawPrimitives(MTLPrimitiveType primitiveType,
                                                         NSUInteger vertexStart,
                                                         NSUInteger vertexCount,
                                                         NSUInteger instanceCount)
{
  MTLPrimitiveType primType = (MTLPrimitiveType)((NSUInteger)primitiveType);
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder = Unwrap<id_MTLRenderCommandEncoder>(this);
  [realMTLRenderCommandEncoder drawPrimitives:primType
                                  vertexStart:vertexStart
                                  vertexCount:vertexCount
                                instanceCount:instanceCount];
}

void WrappedMTLRenderCommandEncoder::real_endEncoding()
{
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder = Unwrap<id_MTLRenderCommandEncoder>(this);
  [realMTLRenderCommandEncoder endEncoding];
}

// Wrapper for MTLRenderCommandEncoder
@implementation ObjCWrappedMTLRenderCommandEncoder

// ObjCWrappedMTLRenderCommandEncoder specific
- (id<MTLRenderCommandEncoder>)realMTLRenderCommandEncoder
{
  id_MTLRenderCommandEncoder realMTLRenderCommandEncoder =
      Unwrap<id_MTLRenderCommandEncoder>(self.wrappedMTLRenderCommandEncoder);
  return realMTLRenderCommandEncoder;
}

// MTLCommandEncoder
- (id<MTLDevice>)device
{
  return self.wrappedMTLRenderCommandEncoder->GetObjCWrappedMTLDevice();
}

- (nullable NSString *)label
{
  return self.realMTLRenderCommandEncoder.label;
}

- (void)setLabel:value
{
  self.realMTLRenderCommandEncoder.label = value;
}

- (void)endEncoding
{
  self.wrappedMTLRenderCommandEncoder->endEncoding();
}

- (void)insertDebugSignpost:(NSString *)string
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder insertDebugSignpost:string];
}

- (void)pushDebugGroup:(NSString *)string
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder pushDebugGroup:string];
}

- (void)popDebugGroup
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder popDebugGroup];
}

// MTLRenderCommandEncoder
- (NSUInteger)tileWidth
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLRenderCommandEncoder.tileWidth;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (NSUInteger)tileHeight
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLRenderCommandEncoder.tileHeight;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (void)setRenderPipelineState:(id<MTLRenderPipelineState>)pipelineState
{
  RDCASSERT([pipelineState isKindOfClass:[ObjCWrappedMTLRenderPipelineState class]]);
  self.wrappedMTLRenderCommandEncoder->setRenderPipelineState(pipelineState);
}

- (void)setVertexBytes:(const void *)bytes
                length:(NSUInteger)length
               atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexBytes:bytes length:length atIndex:index];
}

- (void)setVertexBuffer:(nullable id<MTLBuffer>)buffer
                 offset:(NSUInteger)offset
                atIndex:(NSUInteger)index
{
  RDCASSERT([buffer isKindOfClass:[ObjCWrappedMTLBuffer class]]);
  self.wrappedMTLRenderCommandEncoder->setVertexBuffer(buffer, offset, index);
}

- (void)setVertexBufferOffset:(NSUInteger)offset
                      atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexBufferOffset:offset atIndex:index];
}

- (void)setVertexBuffers:(const id<MTLBuffer> __nullable[__nonnull])buffers
                 offsets:(const NSUInteger[__nonnull])offsets
               withRange:(NSRange)range
{
  // TODO: check every buffer id
  RDCASSERT([buffers[0] isKindOfClass:[ObjCWrappedMTLBuffer class]]);
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return
      [self.realMTLRenderCommandEncoder setVertexBuffers:buffers offsets:offsets withRange:range];
}

- (void)setVertexTexture:(nullable id<MTLTexture>)texture atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexTexture:texture atIndex:index];
}

- (void)setVertexTextures:(const id<MTLTexture> __nullable[__nonnull])textures
                withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexTextures:textures withRange:range];
}

- (void)setVertexSamplerState:(nullable id<MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexSamplerState:sampler atIndex:index];
}

- (void)setVertexSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                     withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexSamplerStates:samplers withRange:range];
}

- (void)setVertexSamplerState:(nullable id<MTLSamplerState>)sampler
                  lodMinClamp:(float)lodMinClamp
                  lodMaxClamp:(float)lodMaxClamp
                      atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexSamplerState:sampler
                                                     lodMinClamp:lodMinClamp
                                                     lodMaxClamp:lodMaxClamp
                                                         atIndex:index];
}

- (void)setVertexSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                  lodMinClamps:(const float[__nonnull])lodMinClamps
                  lodMaxClamps:(const float[__nonnull])lodMaxClamps
                     withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexSamplerStates:samplers
                                                     lodMinClamps:lodMinClamps
                                                     lodMaxClamps:lodMaxClamps
                                                        withRange:range];
}

- (void)setVertexVisibleFunctionTable:(nullable id<MTLVisibleFunctionTable>)functionTable
                        atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexVisibleFunctionTable:functionTable
                                                           atBufferIndex:bufferIndex];
}

- (void)setVertexVisibleFunctionTables:
            (const id<MTLVisibleFunctionTable> __nullable[__nonnull])functionTables
                       withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexVisibleFunctionTables:functionTables
                                                          withBufferRange:range];
}

- (void)setVertexIntersectionFunctionTable:
            (nullable id<MTLIntersectionFunctionTable>)intersectionFunctionTable
                             atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexIntersectionFunctionTable:intersectionFunctionTable
                                                                atBufferIndex:bufferIndex];
}

- (void)setVertexIntersectionFunctionTables:
            (const id<MTLIntersectionFunctionTable> __nullable[__nonnull])intersectionFunctionTable
                            withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexIntersectionFunctionTables:intersectionFunctionTable
                                                               withBufferRange:range];
}

- (void)setVertexAccelerationStructure:(nullable id<MTLAccelerationStructure>)accelerationStructure
                         atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexAccelerationStructure:accelerationStructure
                                                            atBufferIndex:bufferIndex];
}

- (void)setViewport:(MTLViewport)viewport
{
  return self.wrappedMTLRenderCommandEncoder->setViewport(viewport);
}

- (void)setViewports:(const MTLViewport[__nonnull])viewports
               count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(12.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setViewports:viewports count:count];
}

- (void)setFrontFacingWinding:(MTLWinding)frontFacingWinding
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFrontFacingWinding:frontFacingWinding];
}

- (void)setVertexAmplificationCount:(NSUInteger)count
                       viewMappings:(nullable const MTLVertexAmplificationViewMapping *)viewMappings
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVertexAmplificationCount:count
                                                          viewMappings:viewMappings];
}

- (void)setCullMode:(MTLCullMode)cullMode
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setCullMode:cullMode];
}

- (void)setDepthClipMode:(MTLDepthClipMode)depthClipMode API_AVAILABLE(macos(10.11), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setDepthClipMode:depthClipMode];
}

- (void)setDepthBias:(float)depthBias slopeScale:(float)slopeScale clamp:(float)clamp
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return
      [self.realMTLRenderCommandEncoder setDepthBias:depthBias slopeScale:slopeScale clamp:clamp];
}

- (void)setScissorRect:(MTLScissorRect)rect
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setScissorRect:rect];
}

- (void)setScissorRects:(const MTLScissorRect[__nonnull])scissorRects
                  count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(12.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setScissorRects:scissorRects count:count];
}

- (void)setTriangleFillMode:(MTLTriangleFillMode)fillMode
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTriangleFillMode:fillMode];
}

- (void)setFragmentBytes:(const void *)bytes
                  length:(NSUInteger)length
                 atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentBytes:bytes length:length atIndex:index];
}

- (void)setFragmentBuffer:(nullable id<MTLBuffer>)buffer
                   offset:(NSUInteger)offset
                  atIndex:(NSUInteger)index
{
  RDCASSERT([buffer isKindOfClass:[ObjCWrappedMTLBuffer class]]);
  self.wrappedMTLRenderCommandEncoder->setFragmentBuffer(buffer, offset, index);
}

- (void)setFragmentBufferOffset:(NSUInteger)offset
                        atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentBufferOffset:offset atIndex:index];
}

- (void)setFragmentBuffers:(const id<MTLBuffer> __nullable[__nonnull])buffers
                   offsets:(const NSUInteger[__nonnull])offsets
                 withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return
      [self.realMTLRenderCommandEncoder setFragmentBuffers:buffers offsets:offsets withRange:range];
}

- (void)setFragmentTexture:(nullable id<MTLTexture>)texture atIndex:(NSUInteger)index
{
  RDCASSERT([texture isKindOfClass:[ObjCWrappedMTLTexture class]]);
  self.wrappedMTLRenderCommandEncoder->setFragmentTexture(texture, index);
}

- (void)setFragmentTextures:(const id<MTLTexture> __nullable[__nonnull])textures
                  withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentTextures:textures withRange:range];
}

- (void)setFragmentSamplerState:(nullable id<MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentSamplerState:sampler atIndex:index];
}

- (void)setFragmentSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                       withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentSamplerStates:samplers withRange:range];
}

- (void)setFragmentSamplerState:(nullable id<MTLSamplerState>)sampler
                    lodMinClamp:(float)lodMinClamp
                    lodMaxClamp:(float)lodMaxClamp
                        atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentSamplerState:sampler
                                                       lodMinClamp:lodMinClamp
                                                       lodMaxClamp:lodMaxClamp
                                                           atIndex:index];
}

- (void)setFragmentSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                    lodMinClamps:(const float[__nonnull])lodMinClamps
                    lodMaxClamps:(const float[__nonnull])lodMaxClamps
                       withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentSamplerStates:samplers
                                                       lodMinClamps:lodMinClamps
                                                       lodMaxClamps:lodMaxClamps
                                                          withRange:range];
}

- (void)setFragmentVisibleFunctionTable:(nullable id<MTLVisibleFunctionTable>)functionTable
                          atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentVisibleFunctionTable:functionTable
                                                             atBufferIndex:bufferIndex];
}

- (void)setFragmentVisibleFunctionTables:
            (const id<MTLVisibleFunctionTable> __nullable[__nonnull])functionTables
                         withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentVisibleFunctionTables:functionTables
                                                            withBufferRange:range];
}

- (void)setFragmentIntersectionFunctionTable:
            (nullable id<MTLIntersectionFunctionTable>)intersectionFunctionTable
                               atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder
      setFragmentIntersectionFunctionTable:intersectionFunctionTable
                             atBufferIndex:bufferIndex];
}

- (void)setFragmentIntersectionFunctionTables:
            (const id<MTLIntersectionFunctionTable> __nullable[__nonnull])intersectionFunctionTable
                              withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder
      setFragmentIntersectionFunctionTables:intersectionFunctionTable
                            withBufferRange:range];
}

- (void)setFragmentAccelerationStructure:(nullable id<MTLAccelerationStructure>)accelerationStructure
                           atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setFragmentAccelerationStructure:accelerationStructure
                                                              atBufferIndex:bufferIndex];
}

- (void)setBlendColorRed:(float)red green:(float)green blue:(float)blue alpha:(float)alpha
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setBlendColorRed:red green:green blue:blue alpha:alpha];
}

- (void)setDepthStencilState:(nullable id<MTLDepthStencilState>)depthStencilState
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setDepthStencilState:depthStencilState];
}

- (void)setStencilReferenceValue:(uint32_t)referenceValue
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setStencilReferenceValue:referenceValue];
}

- (void)setStencilFrontReferenceValue:(uint32_t)frontReferenceValue
                   backReferenceValue:(uint32_t)backReferenceValue
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setStencilFrontReferenceValue:frontReferenceValue
                                                      backReferenceValue:backReferenceValue];
}

- (void)setVisibilityResultMode:(MTLVisibilityResultMode)mode offset:(NSUInteger)offset
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setVisibilityResultMode:mode offset:offset];
}

- (void)setColorStoreAction:(MTLStoreAction)storeAction
                    atIndex:(NSUInteger)colorAttachmentIndex API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setColorStoreAction:storeAction
                                                       atIndex:colorAttachmentIndex];
}

- (void)setDepthStoreAction:(MTLStoreAction)storeAction API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setDepthStoreAction:storeAction];
}

- (void)setStencilStoreAction:(MTLStoreAction)storeAction API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setStencilStoreAction:storeAction];
}

- (void)setColorStoreActionOptions:(MTLStoreActionOptions)storeActionOptions
                           atIndex:(NSUInteger)colorAttachmentIndex
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setColorStoreActionOptions:storeActionOptions
                                                              atIndex:colorAttachmentIndex];
}

- (void)setDepthStoreActionOptions:(MTLStoreActionOptions)storeActionOptions
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setDepthStoreActionOptions:storeActionOptions];
}

- (void)setStencilStoreActionOptions:(MTLStoreActionOptions)storeActionOptions
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setStencilStoreActionOptions:storeActionOptions];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
         instanceCount:(NSUInteger)instanceCount
{
  self.wrappedMTLRenderCommandEncoder->drawPrimitives(primitiveType, vertexStart, vertexCount,
                                                      instanceCount);
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawPrimitives:primitiveType
                                              vertexStart:vertexStart
                                              vertexCount:vertexCount];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer>)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
                instanceCount:(NSUInteger)instanceCount
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawIndexedPrimitives:primitiveType
                                                      indexCount:indexCount
                                                       indexType:indexType
                                                     indexBuffer:indexBuffer
                                               indexBufferOffset:indexBufferOffset
                                                   instanceCount:instanceCount];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer>)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawIndexedPrimitives:primitiveType
                                                      indexCount:indexCount
                                                       indexType:indexType
                                                     indexBuffer:indexBuffer
                                               indexBufferOffset:indexBufferOffset];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
         instanceCount:(NSUInteger)instanceCount
          baseInstance:(NSUInteger)baseInstance API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawPrimitives:primitiveType
                                              vertexStart:vertexStart
                                              vertexCount:vertexCount
                                            instanceCount:instanceCount
                                             baseInstance:baseInstance];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer>)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
                instanceCount:(NSUInteger)instanceCount
                   baseVertex:(NSInteger)baseVertex
                 baseInstance:(NSUInteger)baseInstance API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawIndexedPrimitives:primitiveType
                                                      indexCount:indexCount
                                                       indexType:indexType
                                                     indexBuffer:indexBuffer
                                               indexBufferOffset:indexBufferOffset
                                                   instanceCount:instanceCount
                                                      baseVertex:baseVertex
                                                    baseInstance:baseInstance];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
          indirectBuffer:(id<MTLBuffer>)indirectBuffer
    indirectBufferOffset:(NSUInteger)indirectBufferOffset API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawPrimitives:primitiveType
                                           indirectBuffer:indirectBuffer
                                     indirectBufferOffset:indirectBufferOffset];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer>)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
               indirectBuffer:(id<MTLBuffer>)indirectBuffer
         indirectBufferOffset:(NSUInteger)indirectBufferOffset API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawIndexedPrimitives:primitiveType
                                                       indexType:indexType
                                                     indexBuffer:indexBuffer
                                               indexBufferOffset:indexBufferOffset
                                                  indirectBuffer:indirectBuffer
                                            indirectBufferOffset:indirectBufferOffset];
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
- (void)textureBarrier
    API_DEPRECATED_WITH_REPLACEMENT("memoryBarrierWithScope:MTLBarrierScopeRenderTargets",
                                    macos(10.11, 10.14))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder textureBarrier];
}
#pragma clang diagnostic pop

- (void)updateFence:(id<MTLFence>)fence
        afterStages:(MTLRenderStages)stages API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder updateFence:fence afterStages:stages];
}

- (void)waitForFence:(id<MTLFence>)fence
        beforeStages:(MTLRenderStages)stages API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder waitForFence:fence beforeStages:stages];
}

- (void)setTessellationFactorBuffer:(nullable id<MTLBuffer>)buffer
                             offset:(NSUInteger)offset
                     instanceStride:(NSUInteger)instanceStride API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTessellationFactorBuffer:buffer
                                                                offset:offset
                                                        instanceStride:instanceStride];
}

- (void)setTessellationFactorScale:(float)scale API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTessellationFactorScale:scale];
}

- (void)drawPatches:(NSUInteger)numberOfPatchControlPoints
                patchStart:(NSUInteger)patchStart
                patchCount:(NSUInteger)patchCount
          patchIndexBuffer:(nullable id<MTLBuffer>)patchIndexBuffer
    patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset
             instanceCount:(NSUInteger)instanceCount
              baseInstance:(NSUInteger)baseInstance API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawPatches:numberOfPatchControlPoints
                                            patchStart:patchStart
                                            patchCount:patchCount
                                      patchIndexBuffer:patchIndexBuffer
                                patchIndexBufferOffset:patchIndexBufferOffset
                                         instanceCount:instanceCount
                                          baseInstance:baseInstance];
}

- (void)drawPatches:(NSUInteger)numberOfPatchControlPoints
          patchIndexBuffer:(nullable id<MTLBuffer>)patchIndexBuffer
    patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset
            indirectBuffer:(id<MTLBuffer>)indirectBuffer
      indirectBufferOffset:(NSUInteger)indirectBufferOffset
    API_AVAILABLE(macos(10.12), ios(12.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawPatches:numberOfPatchControlPoints
                                      patchIndexBuffer:patchIndexBuffer
                                patchIndexBufferOffset:patchIndexBufferOffset
                                        indirectBuffer:indirectBuffer
                                  indirectBufferOffset:indirectBufferOffset];
}

- (void)drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints
                       patchStart:(NSUInteger)patchStart
                       patchCount:(NSUInteger)patchCount
                 patchIndexBuffer:(nullable id<MTLBuffer>)patchIndexBuffer
           patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset
          controlPointIndexBuffer:(id<MTLBuffer>)controlPointIndexBuffer
    controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset
                    instanceCount:(NSUInteger)instanceCount
                     baseInstance:(NSUInteger)baseInstance API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawIndexedPatches:numberOfPatchControlPoints
                                                   patchStart:patchStart
                                                   patchCount:patchCount
                                             patchIndexBuffer:patchIndexBuffer
                                       patchIndexBufferOffset:patchIndexBufferOffset
                                      controlPointIndexBuffer:controlPointIndexBuffer
                                controlPointIndexBufferOffset:controlPointIndexBufferOffset
                                                instanceCount:instanceCount
                                                 baseInstance:baseInstance];
}

- (void)drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints
                 patchIndexBuffer:(nullable id<MTLBuffer>)patchIndexBuffer
           patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset
          controlPointIndexBuffer:(id<MTLBuffer>)controlPointIndexBuffer
    controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset
                   indirectBuffer:(id<MTLBuffer>)indirectBuffer
             indirectBufferOffset:(NSUInteger)indirectBufferOffset
    API_AVAILABLE(macos(10.12), ios(12.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder drawIndexedPatches:numberOfPatchControlPoints
                                             patchIndexBuffer:patchIndexBuffer
                                       patchIndexBufferOffset:patchIndexBufferOffset
                                      controlPointIndexBuffer:controlPointIndexBuffer
                                controlPointIndexBufferOffset:controlPointIndexBufferOffset
                                               indirectBuffer:indirectBuffer
                                         indirectBufferOffset:indirectBufferOffset];
}

- (void)setTileBytes:(const void *)bytes
              length:(NSUInteger)length
             atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileBytes:bytes length:length atIndex:index];
}

- (void)setTileBuffer:(nullable id<MTLBuffer>)buffer
               offset:(NSUInteger)offset
              atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileBuffer:buffer offset:offset atIndex:index];
}

- (void)setTileBufferOffset:(NSUInteger)offset
                    atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileBufferOffset:offset atIndex:index];
}

- (void)setTileBuffers:(const id<MTLBuffer> __nullable[__nonnull])buffers
               offsets:(const NSUInteger[__nonnull])offsets
             withRange:(NSRange)range
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileBuffers:buffers offsets:offsets withRange:range];
}

- (void)setTileTexture:(nullable id<MTLTexture>)texture
               atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileTexture:texture atIndex:index];
}

- (void)setTileTextures:(const id<MTLTexture> __nullable[__nonnull])textures
              withRange:(NSRange)range
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileTextures:textures withRange:range];
}

- (void)setTileSamplerState:(nullable id<MTLSamplerState>)sampler
                    atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileSamplerState:sampler atIndex:index];
}

- (void)setTileSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                   withRange:(NSRange)range
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileSamplerStates:samplers withRange:range];
}

- (void)setTileSamplerState:(nullable id<MTLSamplerState>)sampler
                lodMinClamp:(float)lodMinClamp
                lodMaxClamp:(float)lodMaxClamp
                    atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileSamplerState:sampler
                                                   lodMinClamp:lodMinClamp
                                                   lodMaxClamp:lodMaxClamp
                                                       atIndex:index];
}

- (void)setTileSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                lodMinClamps:(const float[__nonnull])lodMinClamps
                lodMaxClamps:(const float[__nonnull])lodMaxClamps
                   withRange:(NSRange)range
    API_AVAILABLE(ios(11.0), tvos(14.5), macos(11.0), macCatalyst(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileSamplerStates:samplers
                                                   lodMinClamps:lodMinClamps
                                                   lodMaxClamps:lodMaxClamps
                                                      withRange:range];
}

- (void)setTileVisibleFunctionTable:(nullable id<MTLVisibleFunctionTable>)functionTable
                      atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileVisibleFunctionTable:functionTable
                                                         atBufferIndex:bufferIndex];
}

- (void)setTileVisibleFunctionTables:(const id<MTLVisibleFunctionTable> __nullable[__nonnull])functionTables
                     withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileVisibleFunctionTables:functionTables
                                                        withBufferRange:range];
}

- (void)setTileIntersectionFunctionTable:
            (nullable id<MTLIntersectionFunctionTable>)intersectionFunctionTable
                           atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileIntersectionFunctionTable:intersectionFunctionTable
                                                              atBufferIndex:bufferIndex];
}

- (void)setTileIntersectionFunctionTables:
            (const id<MTLIntersectionFunctionTable> __nullable[__nonnull])intersectionFunctionTable
                          withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileIntersectionFunctionTables:intersectionFunctionTable
                                                             withBufferRange:range];
}

- (void)setTileAccelerationStructure:(nullable id<MTLAccelerationStructure>)accelerationStructure
                       atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setTileAccelerationStructure:accelerationStructure
                                                          atBufferIndex:bufferIndex];
}

- (void)dispatchThreadsPerTile:(MTLSize)threadsPerTile
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder dispatchThreadsPerTile:threadsPerTile];
}

- (void)setThreadgroupMemoryLength:(NSUInteger)length
                            offset:(NSUInteger)offset
                           atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder setThreadgroupMemoryLength:length
                                                               offset:offset
                                                              atIndex:index];
}

- (void)useResource:(id<MTLResource>)resource
              usage:(MTLResourceUsage)usage API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useResource:resource usage:usage];
}

- (void)useResources:(const id<MTLResource> __nonnull[__nonnull])resources
               count:(NSUInteger)count
               usage:(MTLResourceUsage)usage API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useResources:resources count:count usage:usage];
}

- (void)useResource:(id<MTLResource>)resource
              usage:(MTLResourceUsage)usage
             stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useResource:resource usage:usage stages:stages];
}

- (void)useResources:(const id<MTLResource> __nonnull[__nonnull])resources
               count:(NSUInteger)count
               usage:(MTLResourceUsage)usage
              stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useResources:resources
                                                  count:count
                                                  usage:usage
                                                 stages:stages];
}

- (void)useHeap:(id<MTLHeap>)heap API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useHeap:heap];
}

- (void)useHeaps:(const id<MTLHeap> __nonnull[__nonnull])heaps
           count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useHeaps:heaps count:count];
}

- (void)useHeap:(id<MTLHeap>)heap
         stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useHeap:heap stages:stages];
}

- (void)useHeaps:(const id<MTLHeap> __nonnull[__nonnull])heaps
           count:(NSUInteger)count
          stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder useHeaps:heaps count:count stages:stages];
}

- (void)executeCommandsInBuffer:(id<MTLIndirectCommandBuffer>)indirectCommandBuffer
                      withRange:(NSRange)executionRange API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder executeCommandsInBuffer:indirectCommandBuffer
                                                         withRange:executionRange];
}

- (void)executeCommandsInBuffer:(id<MTLIndirectCommandBuffer>)indirectCommandbuffer
                 indirectBuffer:(id<MTLBuffer>)indirectRangeBuffer
           indirectBufferOffset:(NSUInteger)indirectBufferOffset
    API_AVAILABLE(macos(10.14), macCatalyst(13.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder executeCommandsInBuffer:indirectCommandbuffer
                                                    indirectBuffer:indirectRangeBuffer
                                              indirectBufferOffset:indirectBufferOffset];
}

- (void)memoryBarrierWithScope:(MTLBarrierScope)scope
                   afterStages:(MTLRenderStages)after
                  beforeStages:(MTLRenderStages)before
    API_AVAILABLE(macos(10.14), macCatalyst(13.0))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder memoryBarrierWithScope:scope
                                                      afterStages:after
                                                     beforeStages:before];
}

- (void)memoryBarrierWithResources:(const id<MTLResource> __nonnull[__nonnull])resources
                             count:(NSUInteger)count
                       afterStages:(MTLRenderStages)after
                      beforeStages:(MTLRenderStages)before
    API_AVAILABLE(macos(10.14), macCatalyst(13.0))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder memoryBarrierWithResources:resources
                                                                count:count
                                                          afterStages:after
                                                         beforeStages:before];
}

- (void)sampleCountersInBuffer:(id<MTLCounterSampleBuffer>)sampleBuffer
                 atSampleIndex:(NSUInteger)sampleIndex
                   withBarrier:(BOOL)barrier API_AVAILABLE(macos(10.15), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderCommandEncoder sampleCountersInBuffer:sampleBuffer
                                                    atSampleIndex:sampleIndex
                                                      withBarrier:barrier];
}

@end
