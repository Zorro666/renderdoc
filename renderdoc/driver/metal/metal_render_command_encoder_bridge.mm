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

#include "metal_render_command_encoder.h"
#include "metal_types_bridge.h"

// Wrapper for MTLRenderCommandEncoder
@implementation ObjCWrappedMTLRenderCommandEncoder

// ObjCWrappedMTLRenderCommandEncoder specific
- (id<MTLRenderCommandEncoder>)real
{
  MTL::RenderCommandEncoder *real = Unwrap<MTL::RenderCommandEncoder *>(self.wrappedCPP);
  return id<MTLRenderCommandEncoder>(real);
}

- (void)dealloc
{
  self.wrappedCPP->Dealloc();
  [super dealloc];
}

// MTLCommandEncoder
- (id<MTLDevice>)device
{
  return id<MTLDevice>(self.wrappedCPP->GetObjCWrappedMTLDevice());
}

- (nullable NSString *)label
{
  return self.real.label;
}

- (void)setLabel:value
{
  self.real.label = value;
}

- (void)endEncoding
{
  self.wrappedCPP->endEncoding();
}

- (void)insertDebugSignpost:(NSString *)string
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real insertDebugSignpost:string];
}

- (void)pushDebugGroup:(NSString *)string
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real pushDebugGroup:string];
}

- (void)popDebugGroup
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real popDebugGroup];
}

// MTLRenderCommandEncoder
- (NSUInteger)tileWidth
{
  if(@available(macOS 11.0, *))
  {
    return self.real.tileWidth;
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
    return self.real.tileHeight;
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
  self.wrappedCPP->setRenderPipelineState((MTL::RenderPipelineState *)pipelineState);
}

- (void)setVertexBytes:(const void *)bytes
                length:(NSUInteger)length
               atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexBytes:bytes length:length atIndex:index];
}

- (void)setVertexBuffer:(nullable id<MTLBuffer>)buffer
                 offset:(NSUInteger)offset
                atIndex:(NSUInteger)index
{
  RDCASSERT([buffer isKindOfClass:[ObjCWrappedMTLBuffer class]]);
  self.wrappedCPP->setVertexBuffer((MTL::Buffer *)buffer, offset, index);
}

- (void)setVertexBufferOffset:(NSUInteger)offset
                      atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexBufferOffset:offset atIndex:index];
}

- (void)setVertexBuffers:(const id<MTLBuffer> __nullable[__nonnull])buffers
                 offsets:(const NSUInteger[__nonnull])offsets
               withRange:(NSRange)range
{
  // TODO: check every buffer id
  RDCASSERT([buffers[0] isKindOfClass:[ObjCWrappedMTLBuffer class]]);
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexBuffers:buffers offsets:offsets withRange:range];
}

- (void)setVertexTexture:(nullable id<MTLTexture>)texture atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexTexture:texture atIndex:index];
}

- (void)setVertexTextures:(const id<MTLTexture> __nullable[__nonnull])textures
                withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexTextures:textures withRange:range];
}

- (void)setVertexSamplerState:(nullable id<MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexSamplerState:sampler atIndex:index];
}

- (void)setVertexSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                     withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexSamplerStates:samplers withRange:range];
}

- (void)setVertexSamplerState:(nullable id<MTLSamplerState>)sampler
                  lodMinClamp:(float)lodMinClamp
                  lodMaxClamp:(float)lodMaxClamp
                      atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexSamplerState:sampler
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
  return [self.real setVertexSamplerStates:samplers
                              lodMinClamps:lodMinClamps
                              lodMaxClamps:lodMaxClamps
                                 withRange:range];
}

- (void)setVertexVisibleFunctionTable:(nullable id<MTLVisibleFunctionTable>)functionTable
                        atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexVisibleFunctionTable:functionTable atBufferIndex:bufferIndex];
}

- (void)setVertexVisibleFunctionTables:
            (const id<MTLVisibleFunctionTable> __nullable[__nonnull])functionTables
                       withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexVisibleFunctionTables:functionTables withBufferRange:range];
}

- (void)setVertexIntersectionFunctionTable:
            (nullable id<MTLIntersectionFunctionTable>)intersectionFunctionTable
                             atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexIntersectionFunctionTable:intersectionFunctionTable
                                         atBufferIndex:bufferIndex];
}

- (void)setVertexIntersectionFunctionTables:
            (const id<MTLIntersectionFunctionTable> __nullable[__nonnull])intersectionFunctionTable
                            withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexIntersectionFunctionTables:intersectionFunctionTable
                                        withBufferRange:range];
}

- (void)setVertexAccelerationStructure:(nullable id<MTLAccelerationStructure>)accelerationStructure
                         atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexAccelerationStructure:accelerationStructure atBufferIndex:bufferIndex];
}

- (void)setViewport:(MTLViewport)viewport
{
  return self.wrappedCPP->setViewport((MTL::Viewport &)viewport);
}

- (void)setViewports:(const MTLViewport[__nonnull])viewports
               count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(12.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setViewports:viewports count:count];
}

- (void)setFrontFacingWinding:(MTLWinding)frontFacingWinding
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFrontFacingWinding:frontFacingWinding];
}

- (void)setVertexAmplificationCount:(NSUInteger)count
                       viewMappings:(nullable const MTLVertexAmplificationViewMapping *)viewMappings
    API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVertexAmplificationCount:count viewMappings:viewMappings];
}

- (void)setCullMode:(MTLCullMode)cullMode
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setCullMode:cullMode];
}

- (void)setDepthClipMode:(MTLDepthClipMode)depthClipMode API_AVAILABLE(macos(10.11), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setDepthClipMode:depthClipMode];
}

- (void)setDepthBias:(float)depthBias slopeScale:(float)slopeScale clamp:(float)clamp
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setDepthBias:depthBias slopeScale:slopeScale clamp:clamp];
}

- (void)setScissorRect:(MTLScissorRect)rect
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setScissorRect:rect];
}

- (void)setScissorRects:(const MTLScissorRect[__nonnull])scissorRects
                  count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(12.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setScissorRects:scissorRects count:count];
}

- (void)setTriangleFillMode:(MTLTriangleFillMode)fillMode
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTriangleFillMode:fillMode];
}

- (void)setFragmentBytes:(const void *)bytes
                  length:(NSUInteger)length
                 atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentBytes:bytes length:length atIndex:index];
}

- (void)setFragmentBuffer:(nullable id<MTLBuffer>)buffer
                   offset:(NSUInteger)offset
                  atIndex:(NSUInteger)index
{
  RDCASSERT([buffer isKindOfClass:[ObjCWrappedMTLBuffer class]]);
  self.wrappedCPP->setFragmentBuffer((MTL::Buffer *)buffer, offset, index);
}

- (void)setFragmentBufferOffset:(NSUInteger)offset
                        atIndex:(NSUInteger)index API_AVAILABLE(macos(10.11), ios(8.3))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentBufferOffset:offset atIndex:index];
}

- (void)setFragmentBuffers:(const id<MTLBuffer> __nullable[__nonnull])buffers
                   offsets:(const NSUInteger[__nonnull])offsets
                 withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentBuffers:buffers offsets:offsets withRange:range];
}

- (void)setFragmentTexture:(nullable id<MTLTexture>)texture atIndex:(NSUInteger)index
{
  RDCASSERT([texture isKindOfClass:[ObjCWrappedMTLTexture class]]);
  self.wrappedCPP->setFragmentTexture((MTL::Texture *)texture, index);
}

- (void)setFragmentTextures:(const id<MTLTexture> __nullable[__nonnull])textures
                  withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentTextures:textures withRange:range];
}

- (void)setFragmentSamplerState:(nullable id<MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentSamplerState:sampler atIndex:index];
}

- (void)setFragmentSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                       withRange:(NSRange)range
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentSamplerStates:samplers withRange:range];
}

- (void)setFragmentSamplerState:(nullable id<MTLSamplerState>)sampler
                    lodMinClamp:(float)lodMinClamp
                    lodMaxClamp:(float)lodMaxClamp
                        atIndex:(NSUInteger)index
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentSamplerState:sampler
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
  return [self.real setFragmentSamplerStates:samplers
                                lodMinClamps:lodMinClamps
                                lodMaxClamps:lodMaxClamps
                                   withRange:range];
}

- (void)setFragmentVisibleFunctionTable:(nullable id<MTLVisibleFunctionTable>)functionTable
                          atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentVisibleFunctionTable:functionTable atBufferIndex:bufferIndex];
}

- (void)setFragmentVisibleFunctionTables:
            (const id<MTLVisibleFunctionTable> __nullable[__nonnull])functionTables
                         withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentVisibleFunctionTables:functionTables withBufferRange:range];
}

- (void)setFragmentIntersectionFunctionTable:
            (nullable id<MTLIntersectionFunctionTable>)intersectionFunctionTable
                               atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentIntersectionFunctionTable:intersectionFunctionTable
                                           atBufferIndex:bufferIndex];
}

- (void)setFragmentIntersectionFunctionTables:
            (const id<MTLIntersectionFunctionTable> __nullable[__nonnull])intersectionFunctionTable
                              withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setFragmentIntersectionFunctionTables:intersectionFunctionTable
                                          withBufferRange:range];
}

- (void)setFragmentAccelerationStructure:(nullable id<MTLAccelerationStructure>)accelerationStructure
                           atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return
      [self.real setFragmentAccelerationStructure:accelerationStructure atBufferIndex:bufferIndex];
}

- (void)setBlendColorRed:(float)red green:(float)green blue:(float)blue alpha:(float)alpha
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setBlendColorRed:red green:green blue:blue alpha:alpha];
}

- (void)setDepthStencilState:(nullable id<MTLDepthStencilState>)depthStencilState
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setDepthStencilState:depthStencilState];
}

- (void)setStencilReferenceValue:(uint32_t)referenceValue
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setStencilReferenceValue:referenceValue];
}

- (void)setStencilFrontReferenceValue:(uint32_t)frontReferenceValue
                   backReferenceValue:(uint32_t)backReferenceValue
    API_AVAILABLE(macos(10.11), ios(9.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setStencilFrontReferenceValue:frontReferenceValue
                               backReferenceValue:backReferenceValue];
}

- (void)setVisibilityResultMode:(MTLVisibilityResultMode)mode offset:(NSUInteger)offset
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setVisibilityResultMode:mode offset:offset];
}

- (void)setColorStoreAction:(MTLStoreAction)storeAction
                    atIndex:(NSUInteger)colorAttachmentIndex API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setColorStoreAction:storeAction atIndex:colorAttachmentIndex];
}

- (void)setDepthStoreAction:(MTLStoreAction)storeAction API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setDepthStoreAction:storeAction];
}

- (void)setStencilStoreAction:(MTLStoreAction)storeAction API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setStencilStoreAction:storeAction];
}

- (void)setColorStoreActionOptions:(MTLStoreActionOptions)storeActionOptions
                           atIndex:(NSUInteger)colorAttachmentIndex
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setColorStoreActionOptions:storeActionOptions atIndex:colorAttachmentIndex];
}

- (void)setDepthStoreActionOptions:(MTLStoreActionOptions)storeActionOptions
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setDepthStoreActionOptions:storeActionOptions];
}

- (void)setStencilStoreActionOptions:(MTLStoreActionOptions)storeActionOptions
    API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setStencilStoreActionOptions:storeActionOptions];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
         instanceCount:(NSUInteger)instanceCount
{
  self.wrappedCPP->drawPrimitives((MTL::PrimitiveType)primitiveType, vertexStart, vertexCount,
                                  instanceCount);
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType
           vertexStart:(NSUInteger)vertexStart
           vertexCount:(NSUInteger)vertexCount
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real drawPrimitives:primitiveType vertexStart:vertexStart vertexCount:vertexCount];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType
                   indexCount:(NSUInteger)indexCount
                    indexType:(MTLIndexType)indexType
                  indexBuffer:(id<MTLBuffer>)indexBuffer
            indexBufferOffset:(NSUInteger)indexBufferOffset
                instanceCount:(NSUInteger)instanceCount
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real drawIndexedPrimitives:primitiveType
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
  return [self.real drawIndexedPrimitives:primitiveType
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
  return [self.real drawPrimitives:primitiveType
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
  return [self.real drawIndexedPrimitives:primitiveType
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
  return [self.real drawPrimitives:primitiveType
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
  return [self.real drawIndexedPrimitives:primitiveType
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
  return [self.real textureBarrier];
}
#pragma clang diagnostic pop

- (void)updateFence:(id<MTLFence>)fence
        afterStages:(MTLRenderStages)stages API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real updateFence:fence afterStages:stages];
}

- (void)waitForFence:(id<MTLFence>)fence
        beforeStages:(MTLRenderStages)stages API_AVAILABLE(macos(10.13), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real waitForFence:fence beforeStages:stages];
}

- (void)setTessellationFactorBuffer:(nullable id<MTLBuffer>)buffer
                             offset:(NSUInteger)offset
                     instanceStride:(NSUInteger)instanceStride API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTessellationFactorBuffer:buffer offset:offset instanceStride:instanceStride];
}

- (void)setTessellationFactorScale:(float)scale API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTessellationFactorScale:scale];
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
  return [self.real drawPatches:numberOfPatchControlPoints
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
  return [self.real drawPatches:numberOfPatchControlPoints
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
  return [self.real drawIndexedPatches:numberOfPatchControlPoints
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
  return [self.real drawIndexedPatches:numberOfPatchControlPoints
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
  return [self.real setTileBytes:bytes length:length atIndex:index];
}

- (void)setTileBuffer:(nullable id<MTLBuffer>)buffer
               offset:(NSUInteger)offset
              atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileBuffer:buffer offset:offset atIndex:index];
}

- (void)setTileBufferOffset:(NSUInteger)offset
                    atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileBufferOffset:offset atIndex:index];
}

- (void)setTileBuffers:(const id<MTLBuffer> __nullable[__nonnull])buffers
               offsets:(const NSUInteger[__nonnull])offsets
             withRange:(NSRange)range
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileBuffers:buffers offsets:offsets withRange:range];
}

- (void)setTileTexture:(nullable id<MTLTexture>)texture
               atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileTexture:texture atIndex:index];
}

- (void)setTileTextures:(const id<MTLTexture> __nullable[__nonnull])textures
              withRange:(NSRange)range
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileTextures:textures withRange:range];
}

- (void)setTileSamplerState:(nullable id<MTLSamplerState>)sampler
                    atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileSamplerState:sampler atIndex:index];
}

- (void)setTileSamplerStates:(const id<MTLSamplerState> __nullable[__nonnull])samplers
                   withRange:(NSRange)range
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileSamplerStates:samplers withRange:range];
}

- (void)setTileSamplerState:(nullable id<MTLSamplerState>)sampler
                lodMinClamp:(float)lodMinClamp
                lodMaxClamp:(float)lodMaxClamp
                    atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileSamplerState:sampler
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
  return [self.real setTileSamplerStates:samplers
                            lodMinClamps:lodMinClamps
                            lodMaxClamps:lodMaxClamps
                               withRange:range];
}

- (void)setTileVisibleFunctionTable:(nullable id<MTLVisibleFunctionTable>)functionTable
                      atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileVisibleFunctionTable:functionTable atBufferIndex:bufferIndex];
}

- (void)setTileVisibleFunctionTables:(const id<MTLVisibleFunctionTable> __nullable[__nonnull])functionTables
                     withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileVisibleFunctionTables:functionTables withBufferRange:range];
}

- (void)setTileIntersectionFunctionTable:
            (nullable id<MTLIntersectionFunctionTable>)intersectionFunctionTable
                           atBufferIndex:(NSUInteger)bufferIndex
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileIntersectionFunctionTable:intersectionFunctionTable
                                       atBufferIndex:bufferIndex];
}

- (void)setTileIntersectionFunctionTables:
            (const id<MTLIntersectionFunctionTable> __nullable[__nonnull])intersectionFunctionTable
                          withBufferRange:(NSRange)range API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return
      [self.real setTileIntersectionFunctionTables:intersectionFunctionTable withBufferRange:range];
}

- (void)setTileAccelerationStructure:(nullable id<MTLAccelerationStructure>)accelerationStructure
                       atBufferIndex:(NSUInteger)bufferIndex API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setTileAccelerationStructure:accelerationStructure atBufferIndex:bufferIndex];
}

- (void)dispatchThreadsPerTile:(MTLSize)threadsPerTile
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real dispatchThreadsPerTile:threadsPerTile];
}

- (void)setThreadgroupMemoryLength:(NSUInteger)length
                            offset:(NSUInteger)offset
                           atIndex:(NSUInteger)index
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real setThreadgroupMemoryLength:length offset:offset atIndex:index];
}

- (void)useResource:(id<MTLResource>)resource
              usage:(MTLResourceUsage)usage API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useResource:resource usage:usage];
}

- (void)useResources:(const id<MTLResource> __nonnull[__nonnull])resources
               count:(NSUInteger)count
               usage:(MTLResourceUsage)usage API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useResources:resources count:count usage:usage];
}

- (void)useResource:(id<MTLResource>)resource
              usage:(MTLResourceUsage)usage
             stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useResource:resource usage:usage stages:stages];
}

- (void)useResources:(const id<MTLResource> __nonnull[__nonnull])resources
               count:(NSUInteger)count
               usage:(MTLResourceUsage)usage
              stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useResources:resources count:count usage:usage stages:stages];
}

- (void)useHeap:(id<MTLHeap>)heap API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useHeap:heap];
}

- (void)useHeaps:(const id<MTLHeap> __nonnull[__nonnull])heaps
           count:(NSUInteger)count API_AVAILABLE(macos(10.13), ios(11.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useHeaps:heaps count:count];
}

- (void)useHeap:(id<MTLHeap>)heap
         stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useHeap:heap stages:stages];
}

- (void)useHeaps:(const id<MTLHeap> __nonnull[__nonnull])heaps
           count:(NSUInteger)count
          stages:(MTLRenderStages)stages API_AVAILABLE(macos(10.15), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real useHeaps:heaps count:count stages:stages];
}

- (void)executeCommandsInBuffer:(id<MTLIndirectCommandBuffer>)indirectCommandBuffer
                      withRange:(NSRange)executionRange API_AVAILABLE(macos(10.14), ios(12.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real executeCommandsInBuffer:indirectCommandBuffer withRange:executionRange];
}

- (void)executeCommandsInBuffer:(id<MTLIndirectCommandBuffer>)indirectCommandbuffer
                 indirectBuffer:(id<MTLBuffer>)indirectRangeBuffer
           indirectBufferOffset:(NSUInteger)indirectBufferOffset
    API_AVAILABLE(macos(10.14), macCatalyst(13.0), ios(13.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real executeCommandsInBuffer:indirectCommandbuffer
                             indirectBuffer:indirectRangeBuffer
                       indirectBufferOffset:indirectBufferOffset];
}

- (void)memoryBarrierWithScope:(MTLBarrierScope)scope
                   afterStages:(MTLRenderStages)after
                  beforeStages:(MTLRenderStages)before
    API_AVAILABLE(macos(10.14), macCatalyst(13.0))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real memoryBarrierWithScope:scope afterStages:after beforeStages:before];
}

- (void)memoryBarrierWithResources:(const id<MTLResource> __nonnull[__nonnull])resources
                             count:(NSUInteger)count
                       afterStages:(MTLRenderStages)after
                      beforeStages:(MTLRenderStages)before
    API_AVAILABLE(macos(10.14), macCatalyst(13.0))API_UNAVAILABLE(ios)
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real memoryBarrierWithResources:resources
                                         count:count
                                   afterStages:after
                                  beforeStages:before];
}

- (void)sampleCountersInBuffer:(id<MTLCounterSampleBuffer>)sampleBuffer
                 atSampleIndex:(NSUInteger)sampleIndex
                   withBarrier:(BOOL)barrier API_AVAILABLE(macos(10.15), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return
      [self.real sampleCountersInBuffer:sampleBuffer atSampleIndex:sampleIndex withBarrier:barrier];
}

@end
