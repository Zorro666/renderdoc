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
#include "core/core.h"
#include "metal_manager.h"
#include "metal_metal.h"
#include "metal_resources.h"

// MTLRenderPassAttachmentDescriptor
// id <MTLTexture> texture;
// NSUInteger level;
// NSUInteger slice;
// NSUInteger depthPlane;
// id <MTLTexture> resolveTexture;
// NSUInteger resolveLevel;
// NSUInteger resolveSlice;
// NSUInteger resolveDepthPlane;
// MTLLoadAction loadAction;
// MTLStoreAction storeAction;
// MTLStoreActionOptions storeActionOptions;

// MTLRenderPassColorAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
// MTLClearColor clearColor;

// MTLRenderPassDepthAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
// double clearDepth;
// MTLMultisampleDepthResolveFilter depthResolveFilter;

// MTLRenderPassStencilAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
// uint32_t clearStencil;
// MTLMultisampleStencilResolveFilter stencilResolveFilter;

// MTLRasterizationRateMap ?????
// MTLRenderPassSampleBufferAttachmentDescriptorArray ???

// MTLRenderPassDescriptor
// MTLRenderPassColorAttachmentDescriptorArray
// MTLRenderPassDepthAttachmentDescriptor
// MTLRenderPassStencilAttachmentDescriptor
// id<MTLBuffer> visibilityResultBuffer
// NSUInteger renderTargetArrayLength
// NSUInteger imageblockSampleLength
// NSUInteger threadgroupMemoryLength
// NSUInteger tileWidth
// NSUInteger tileHeight
// NSUInteger defaultRasterSampleCount
// NSUInteger renderTargetWidth
// NSUInteger renderTargetHeight
// MTLSamplePosition* samplePositions
// MTLRasterizationRateMap
// MTLRenderPassSampleBufferAttachmentDescriptorArray

// MTLRenderPass.h
template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLRenderPassDescriptor *&el)
{
  if(ser.IsReading())
  {
    el = MTL::NewMTLRenderPassDescriptor();
  }
  MTLRenderPassColorAttachmentDescriptor *colorAttachments[MAX_RENDER_PASS_COLOR_ATTACHMENTS];
  MTLRenderPassDepthAttachmentDescriptor *depthAttachment;
  MTLRenderPassStencilAttachmentDescriptor *stencilAttachment;
  // TODO: id <MTLBuffer> visibilityResultBuffer;
  NSUInteger_objc renderTargetArrayLength;
  NSUInteger_objc imageblockSampleLength;
  NSUInteger_objc threadgroupMemoryLength;
  NSUInteger_objc tileWidth;
  NSUInteger_objc tileHeight;
  NSUInteger_objc defaultRasterSampleCount;
  NSUInteger_objc renderTargetWidth;
  NSUInteger_objc renderTargetHeight;
  // TODO: MTLSamplePosition* samplePositions
  // TODO: id<MTLRasterizationRateMap> rasterizationRateMap;
  // TODO: MTLRenderPassSampleBufferAttachmentDescriptorArray * sampleBufferAttachments;

  for(uint32_t i = 0; i < MAX_RENDER_PASS_COLOR_ATTACHMENTS; ++i)
  {
    colorAttachments[i] = MTL_GET(el, colorAttachment, i);
  }
  depthAttachment = MTL_GET(el, depthAttachment);
  stencilAttachment = MTL_GET(el, stencilAttachment);

  if(ser.IsWriting())
  {
    // TODO: id <MTLBuffer> visibilityResultBuffer;
    renderTargetArrayLength = MTL_GET(el, renderTargetArrayLength);
    imageblockSampleLength = MTL_GET(el, imageblockSampleLength);
    threadgroupMemoryLength = MTL_GET(el, threadgroupMemoryLength);
    tileWidth = MTL_GET(el, tileWidth);
    tileHeight = MTL_GET(el, tileHeight);
    defaultRasterSampleCount = MTL_GET(el, defaultRasterSampleCount);
    renderTargetWidth = MTL_GET(el, renderTargetWidth);
    renderTargetHeight = MTL_GET(el, renderTargetHeight);
    // TODO: getSamplePositions:(MTLSamplePosition *)positions count:(NSUInteger)count;
    // TODO: id<MTLRasterizationRateMap> rasterizationRateMap;
    // TODO: MTLRenderPassSampleBufferAttachmentDescriptorArray * sampleBufferAttachments;
  }
  SERIALISE_ELEMENT(colorAttachments);
  SERIALISE_ELEMENT(depthAttachment);
  SERIALISE_ELEMENT(stencilAttachment);
  // TODO: id <MTLBuffer> visibilityResultBuffer;
  SERIALISE_ELEMENT(renderTargetArrayLength);
  SERIALISE_ELEMENT(imageblockSampleLength);
  SERIALISE_ELEMENT(threadgroupMemoryLength);
  SERIALISE_ELEMENT(tileWidth);
  SERIALISE_ELEMENT(tileHeight);
  SERIALISE_ELEMENT(defaultRasterSampleCount);
  SERIALISE_ELEMENT(renderTargetWidth);
  SERIALISE_ELEMENT(renderTargetHeight);
  // TODO: getSamplePositions:(MTLSamplePosition *)positions count:(NSUInteger)count;
  // TODO: id<MTLRasterizationRateMap> rasterizationRateMap;
  // TODO: MTLRenderPassSampleBufferAttachmentDescriptorArray * sampleBufferAttachments;

  if(ser.IsReading())
  {
    RDCASSERT(el != NULL);
    // TODO: id <MTLBuffer> visibilityResultBuffer;
    MTL_SET(el, renderTargetArrayLength, renderTargetArrayLength);
    MTL_SET(el, imageblockSampleLength, imageblockSampleLength);
    MTL_SET(el, threadgroupMemoryLength, threadgroupMemoryLength);
    MTL_SET(el, tileWidth, tileWidth);
    MTL_SET(el, tileHeight, tileHeight);
    MTL_SET(el, defaultRasterSampleCount, defaultRasterSampleCount);
    MTL_SET(el, renderTargetWidth, renderTargetWidth);
    MTL_SET(el, renderTargetHeight, renderTargetHeight);
    // TODO: getSamplePositions:(MTLSamplePosition *)positions count:(NSUInteger)count;
    // TODO: id<MTLRasterizationRateMap> rasterizationRateMap;
    // TODO: MTLRenderPassSampleBufferAttachmentDescriptorArray * sampleBufferAttachments;
  }
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLRenderPassAttachmentDescriptor *&el)
{
  // MTLRenderPassAttachmentDescriptor
  WrappedMTLTexture *texture_;
  NSUInteger_objc level;
  NSUInteger_objc slice;
  NSUInteger_objc depthPlane;
  WrappedMTLTexture *resolveTexture_;
  NSUInteger_objc resolveLevel;
  NSUInteger_objc resolveSlice;
  NSUInteger_objc resolveDepthPlane;
  MTLLoadAction_objc loadAction;
  MTLStoreAction_objc storeAction;
  MTLStoreActionOptions_objc storeActionOptions;

  if(ser.IsWriting())
  {
    texture_ = GetWrapped(MTL_GET(el, texture));
    level = MTL_GET(el, level);
    slice = MTL_GET(el, slice);
    depthPlane = MTL_GET(el, depthPlane);
    resolveTexture_ = GetWrapped(MTL_GET(el, resolveTexture));
    resolveLevel = MTL_GET(el, resolveLevel);
    resolveSlice = MTL_GET(el, resolveSlice);
    resolveDepthPlane = MTL_GET(el, resolveDepthPlane);
    loadAction = (MTLLoadAction_objc)MTL_GET(el, loadAction);
    storeAction = (MTLStoreAction_objc)MTL_GET(el, storeAction);
    storeActionOptions = (MTLStoreActionOptions_objc)MTL_GET(el, storeActionOptions);
  }

  SERIALISE_ELEMENT_LOCAL(texture, GetResID(texture_)).TypedAs("MTLTexture"_lit);
  SERIALISE_ELEMENT(level);
  SERIALISE_ELEMENT(slice);
  SERIALISE_ELEMENT(depthPlane);
  SERIALISE_ELEMENT_LOCAL(resolveTexture, GetResID(resolveTexture_)).TypedAs("MTLTexture"_lit);
  SERIALISE_ELEMENT(resolveLevel);
  SERIALISE_ELEMENT(resolveSlice);
  SERIALISE_ELEMENT(resolveDepthPlane);
  SERIALISE_ELEMENT(loadAction);
  SERIALISE_ELEMENT(storeAction);
  SERIALISE_ELEMENT(storeActionOptions);

  if(ser.IsReading())
  {
    RDCASSERT(el != NULL);
    MetalResourceManager *rm = (MetalResourceManager *)ser.GetUserData();
    id_MTLTexture objcTexture = GetObjCWrappedResource<id_MTLTexture>(rm, texture);
    texture_ = GetWrapped(objcTexture);
    id_MTLTexture realTexture = Unwrap<id_MTLTexture>(texture_);
    MTL_SET(el, texture, objcTexture);
    MTL_SET(el, level, level);
    MTL_SET(el, slice, slice);
    MTL_SET(el, depthPlane, depthPlane);
    id_MTLTexture objcResolveTexture = GetObjCWrappedResource<id_MTLTexture>(rm, resolveTexture);
    MTL_SET(el, resolveTexture, objcResolveTexture);
    MTL_SET(el, resolveLevel, resolveLevel);
    MTL_SET(el, resolveSlice, resolveSlice);
    MTL_SET(el, resolveDepthPlane, resolveDepthPlane);
    MTL_SET(el, loadAction, (MTLLoadAction)loadAction);
    MTL_SET(el, storeAction, (MTLStoreAction)storeAction);
    MTL_SET(el, storeActionOptions, (MTLStoreActionOptions)storeActionOptions);
  }
}

// MTLRenderPassColorAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
// MTLClearColor clearColor;

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLRenderPassColorAttachmentDescriptor *&el)
{
  MTLRenderPassColorAttachmentDescriptor *colorDescriptor = el;
  MTLRenderPassAttachmentDescriptor *descriptor =
      (MTLRenderPassAttachmentDescriptor *)colorDescriptor;
  MTLClearColor clearColor;

  if(ser.IsWriting())
  {
    clearColor = MTL_GET(el, clearColor);
  }

  DoSerialise(ser, descriptor);
  SERIALISE_ELEMENT(clearColor);

  if(ser.IsReading())
  {
    RDCASSERT(el != NULL);
    MTL_SET(el, clearColor, clearColor);
  }
}

// MTLRenderPassDepthAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
// double clearDepth;
// MTLMultisampleDepthResolveFilter depthResolveFilter;

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLRenderPassDepthAttachmentDescriptor *&el)
{
  MTLRenderPassDepthAttachmentDescriptor *depthDescriptor = el;
  MTLRenderPassAttachmentDescriptor *descriptor =
      (MTLRenderPassAttachmentDescriptor *)depthDescriptor;
  double clearDepth;
  // TODO: MTLMultisampleDepthResolveFilter depthResolveFilter;

  if(ser.IsWriting())
  {
    clearDepth = MTL_GET(el, clearDepth);
  }

  DoSerialise(ser, descriptor);
  SERIALISE_ELEMENT(clearDepth);
  // TODO: TODO: MTLMultisampleDepthResolveFilter depthResolveFilter;

  if(ser.IsReading())
  {
    RDCASSERT(el != NULL);
    MTL_SET(el, clearDepth, clearDepth);
  }
}

// MTLRenderPassStencilAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
// uint32_t clearStencil;
// MTLMultisampleStencilResolveFilter stencilResolveFilter;

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLRenderPassStencilAttachmentDescriptor *&el)
{
  MTLRenderPassStencilAttachmentDescriptor *stencilDescriptor = el;

  MTLRenderPassAttachmentDescriptor *descriptor =
      (MTLRenderPassAttachmentDescriptor *)stencilDescriptor;
  uint32_t clearStencil;
  // TODO: MTLMultisampleStencilResolveFilter stencilResolveFilter;

  if(ser.IsWriting())
  {
    clearStencil = MTL_GET(el, clearStencil);
  }

  DoSerialise(ser, descriptor);
  SERIALISE_ELEMENT(clearStencil);
  // TODO: MTLMultisampleStencilResolveFilter stencilResolveFilter;

  if(ser.IsReading())
  {
    RDCASSERT(el != NULL);
    MTL_SET(el, clearStencil, clearStencil);
  }
}

INSTANTIATE_SERIALISE_TYPE(MTLRenderPassDescriptor *);
INSTANTIATE_SERIALISE_TYPE(MTLRenderPassAttachmentDescriptor *);
INSTANTIATE_SERIALISE_TYPE(MTLRenderPassColorAttachmentDescriptor *);
INSTANTIATE_SERIALISE_TYPE(MTLRenderPassDepthAttachmentDescriptor *);
INSTANTIATE_SERIALISE_TYPE(MTLRenderPassStencilAttachmentDescriptor *);
