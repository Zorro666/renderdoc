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

#include "metal_render_pass_descriptor.h"
#import <Metal/MTLRenderPass.h>

// MTLRenderPassAttachmentDescriptor

void MTL::Set_texture(MTLRenderPassAttachmentDescriptor *descriptor, id_MTLTexture texture)
{
  descriptor.texture = texture;
}

void MTL::Set_level(MTLRenderPassAttachmentDescriptor *descriptor, NSUInteger_objc level)
{
  descriptor.level = level;
}

void MTL::Set_slice(MTLRenderPassAttachmentDescriptor *descriptor, NSUInteger_objc slice)
{
  descriptor.slice = slice;
}

void MTL::Set_depthPlane(MTLRenderPassAttachmentDescriptor *descriptor, NSUInteger_objc depthPlane)
{
  descriptor.depthPlane = depthPlane;
}

void MTL::Set_resolveTexture(MTLRenderPassAttachmentDescriptor *descriptor,
                             id_MTLTexture resolveTexture)
{
  descriptor.resolveTexture = resolveTexture;
}

void MTL::Set_resolveLevel(MTLRenderPassAttachmentDescriptor *descriptor, NSUInteger_objc resolveLevel)
{
  descriptor.resolveLevel = resolveLevel;
}

void MTL::Set_resolveSlice(MTLRenderPassAttachmentDescriptor *descriptor, NSUInteger_objc resolveSlice)
{
  descriptor.resolveSlice = resolveSlice;
}

void MTL::Set_resolveDepthPlane(MTLRenderPassAttachmentDescriptor *descriptor,
                                NSUInteger_objc resolveDepthPlane)
{
  descriptor.resolveDepthPlane = resolveDepthPlane;
}

void MTL::Set_loadAction(MTLRenderPassAttachmentDescriptor *descriptor, MTLLoadAction loadAction)
{
  descriptor.loadAction = loadAction;
}

void MTL::Set_storeAction(MTLRenderPassAttachmentDescriptor *descriptor, MTLStoreAction storeAction)
{
  descriptor.storeAction = storeAction;
}

void MTL::Set_storeActionOptions(MTLRenderPassAttachmentDescriptor *descriptor,
                                 MTLStoreActionOptions storeActionOptions)
{
  descriptor.storeActionOptions = storeActionOptions;
}

NSUInteger_objc MTL::Get_level(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.level;
}

id_MTLTexture MTL::Get_texture(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return descriptor.texture;
}

NSUInteger_objc MTL::Get_slice(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.slice;
}

NSUInteger_objc MTL::Get_depthPlane(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.depthPlane;
}

id_MTLTexture MTL::Get_resolveTexture(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return descriptor.resolveTexture;
}

NSUInteger_objc MTL::Get_resolveLevel(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.resolveLevel;
}

NSUInteger_objc MTL::Get_resolveSlice(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.resolveSlice;
}

NSUInteger_objc MTL::Get_resolveDepthPlane(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.resolveDepthPlane;
}

MTLLoadAction MTL::Get_loadAction(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return descriptor.loadAction;
}

MTLStoreAction MTL::Get_storeAction(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return descriptor.storeAction;
}

MTLStoreActionOptions MTL::Get_storeActionOptions(MTLRenderPassAttachmentDescriptor *descriptor)
{
  return descriptor.storeActionOptions;
}

// MTLRenderPassColorAttachmentDescriptor
void MTL::Set_clearColor(MTLRenderPassColorAttachmentDescriptor *descriptor, MTLClearColor clearColor)
{
  descriptor.clearColor = clearColor;
}

MTLClearColor MTL::Get_clearColor(MTLRenderPassColorAttachmentDescriptor *descriptor)
{
  return descriptor.clearColor;
}

// MTLRenderPassDepthAttachmentDescriptor
void MTL::Set_clearDepth(MTLRenderPassDepthAttachmentDescriptor *descriptor, double clearDepth)
{
  descriptor.clearDepth = clearDepth;
}

double MTL::Get_clearDepth(MTLRenderPassDepthAttachmentDescriptor *descriptor)
{
  return descriptor.clearDepth;
}

// TODO: MTLMultisampleDepthResolveFilter depthResolveFilter;

// MTLRenderPassStencilAttachmentDescriptor
void MTL::Set_clearStencil(MTLRenderPassStencilAttachmentDescriptor *descriptor, uint32_t clearStencil)
{
  descriptor.clearStencil = clearStencil;
}

uint32_t MTL::Get_clearStencil(MTLRenderPassStencilAttachmentDescriptor *descriptor)
{
  return descriptor.clearStencil;
}

// TODO: MTLMultisampleStencilResolveFilter stencilResolveFilter;

// MTLRenderPassDescriptor
MTLRenderPassDescriptor *MTL::NewMTLRenderPassDescriptor()
{
  return [MTLRenderPassDescriptor renderPassDescriptor];
}

/*
id_MTLBuffer MTL::Set_visibilityResultBuffer(MTLRenderPassDescriptor *descriptor)
{
  return descriptor.visibilityResultBuffer;
}
*/

void MTL::Set_renderTargetArrayLength(MTLRenderPassDescriptor *descriptor,
                                      NSUInteger_objc renderTargetArrayLength)
{
  descriptor.renderTargetArrayLength = renderTargetArrayLength;
}

void MTL::Set_imageblockSampleLength(MTLRenderPassDescriptor *descriptor,
                                     NSUInteger_objc imageblockSampleLength)
{
  if(@available(macOS 11.0, *))
  {
    descriptor.imageblockSampleLength = imageblockSampleLength;
  }
}

void MTL::Set_threadgroupMemoryLength(MTLRenderPassDescriptor *descriptor,
                                      NSUInteger_objc threadgroupMemoryLength)
{
  if(@available(macOS 11.0, *))
  {
    descriptor.threadgroupMemoryLength = threadgroupMemoryLength;
  }
}

void MTL::Set_tileWidth(MTLRenderPassDescriptor *descriptor, NSUInteger_objc tileWidth)
{
  if(@available(macOS 11.0, *))
  {
    descriptor.tileWidth = tileWidth;
  }
}

void MTL::Set_tileHeight(MTLRenderPassDescriptor *descriptor, NSUInteger_objc tileHeight)
{
  if(@available(macOS 11.0, *))
  {
    descriptor.tileHeight = tileHeight;
  }
}

void MTL::Set_defaultRasterSampleCount(MTLRenderPassDescriptor *descriptor,
                                       NSUInteger_objc defaultRasterSampleCount)
{
  descriptor.defaultRasterSampleCount = defaultRasterSampleCount;
}

void MTL::Set_renderTargetWidth(MTLRenderPassDescriptor *descriptor, NSUInteger_objc renderTargetWidth)
{
  descriptor.renderTargetWidth = renderTargetWidth;
}

void MTL::Set_renderTargetHeight(MTLRenderPassDescriptor *descriptor,
                                 NSUInteger_objc renderTargetHeight)
{
  descriptor.renderTargetHeight = renderTargetHeight;
}

/*
void MTL::Set_samplePositions(MTLRenderPassDescriptor *descriptor, MTLSamplePosition*
positions, NSUInteger_objc count)
{
  [descriptor setSamplePositions:positions count:count];
}
*/

// TODO: id_MTLRasterizationRateMap
// MTL::Set_rasterizationRateMap(MTLRenderPassDescriptor* descriptor) { }

/*
API_AVAILABLE(macos(11.0))
void MTL::Set_sampleBufferAttachments(
    MTLRenderPassDescriptor *descriptor, MTLRenderPassSampleBufferAttachmentDescriptorArray
*sampleBufferAttachments)
{
  descriptor.sampleBufferAttachments = sampleBufferAttachments;
}
 */

MTLRenderPassColorAttachmentDescriptor *MTL::Get_colorAttachment(MTLRenderPassDescriptor *descriptor,
                                                                 uint32_t idx)
{
  return descriptor.colorAttachments[idx];
}

MTLRenderPassDepthAttachmentDescriptor *MTL::Get_depthAttachment(MTLRenderPassDescriptor *descriptor)
{
  return descriptor.depthAttachment;
}

MTLRenderPassStencilAttachmentDescriptor *MTL::Get_stencilAttachment(MTLRenderPassDescriptor *descriptor)
{
  return descriptor.stencilAttachment;
}

id_MTLBuffer MTL::Get_visibilityResultBuffer(MTLRenderPassDescriptor *descriptor)
{
  return descriptor.visibilityResultBuffer;
}

NSUInteger_objc MTL::Get_renderTargetArrayLength(MTLRenderPassDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.renderTargetArrayLength;
}

NSUInteger_objc MTL::Get_imageblockSampleLength(MTLRenderPassDescriptor *descriptor)
{
  if(@available(macOS 11.0, *))
  {
    return (NSUInteger_objc)descriptor.imageblockSampleLength;
  }
  else
  {
    // Fallback on earlier versions
    return NSUInteger_Zero;
  }
}

NSUInteger_objc MTL::Get_threadgroupMemoryLength(MTLRenderPassDescriptor *descriptor)
{
  if(@available(macOS 11.0, *))
  {
    return (NSUInteger_objc)descriptor.threadgroupMemoryLength;
  }
  else
  {
    // Fallback on earlier versions
    return NSUInteger_Zero;
  }
}

NSUInteger_objc MTL::Get_tileWidth(MTLRenderPassDescriptor *descriptor)
{
  if(@available(macOS 11.0, *))
  {
    return (NSUInteger_objc)descriptor.tileWidth;
  }
  else
  {
    // Fallback on earlier versions
    return NSUInteger_Zero;
  }
}

NSUInteger_objc MTL::Get_tileHeight(MTLRenderPassDescriptor *descriptor)
{
  if(@available(macOS 11.0, *))
  {
    return (NSUInteger_objc)descriptor.tileHeight;
  }
  else
  {
    // Fallback on earlier versions
    return NSUInteger_Zero;
  }
}

NSUInteger_objc MTL::Get_defaultRasterSampleCount(MTLRenderPassDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.defaultRasterSampleCount;
}

NSUInteger_objc MTL::Get_renderTargetWidth(MTLRenderPassDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.renderTargetWidth;
}

NSUInteger_objc MTL::Get_renderTargetHeight(MTLRenderPassDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.renderTargetHeight;
}

/*
MTLSamplePosition *MTL::Get_samplePositions_positions(MTLRenderPassDescriptor
*descriptor)
{
  MTLSamplePosition *positions = NULL;
  NSUInteger count = 0;
  [descriptor getSamplePositions:positions count:count];
  return positions;
}

NSUInteger_objc MTL::Get_samplePositions_count(MTLRenderPassDescriptor *descriptor)
{
  MTLSamplePosition *positions = NULL;
  NSUInteger count = 0;
  [descriptor getSamplePositions:positions count:count];
  return (NSUInteger_objc)count;
}

// TODO: id_MTLRasterizationRateMap
// MTL::Get_rasterizationRateMap(MTLRenderPassDescriptor* descriptor) { }

API_AVAILABLE(macos(11.0))
MTLRenderPassSampleBufferAttachmentDescriptorArray *MTL::Get_sampleBufferAttachments(
    MTLRenderPassDescriptor *descriptor)
{
    return descriptor.sampleBufferAttachments;
}
*/
