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

#pragma once

#include <os/availability.h>
#include "metal_common.h"
#include "metal_device.h"
#include "metal_resources.h"

// MTLRenderPass.h
namespace MTL
{
// MTLRenderPassAttachmentDescriptor
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, id_MTLTexture, texture);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, level);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, slice);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, depthPlane);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, id_MTLTexture, resolveTexture);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveLevel);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveSlice);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveDepthPlane);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, MTLLoadAction, loadAction);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, MTLStoreAction, storeAction);
GETSET_PROPERTY(MTLRenderPassAttachmentDescriptor, MTLStoreActionOptions, storeActionOptions);

// MTLRenderPassColorAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          id_MTLTexture, texture);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, level);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, slice);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, depthPlane);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          id_MTLTexture, resolveTexture);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, resolveLevel);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, resolveSlice);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, resolveDepthPlane);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          MTLLoadAction, loadAction);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          MTLStoreAction, storeAction);
GETSET_PROPERTY_INHERITED(MTLRenderPassColorAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          MTLStoreActionOptions, storeActionOptions);
GETSET_PROPERTY(MTLRenderPassColorAttachmentDescriptor, MTLClearColor, clearColor);

// MTLRenderPassDepthAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          id_MTLTexture, texture);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, level);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, slice);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, depthPlane);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          id_MTLTexture, resolveTexture);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, resolveLevel);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, resolveSlice);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          NSUInteger_objc, resolveDepthPlane);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          MTLLoadAction, loadAction);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          MTLStoreAction, storeAction);
GETSET_PROPERTY_INHERITED(MTLRenderPassDepthAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          MTLStoreActionOptions, storeActionOptions);
GETSET_PROPERTY(MTLRenderPassDepthAttachmentDescriptor, double, clearDepth);
// TODO: MTLMultisampleDepthResolveFilter depthResolveFilter;

// MTLRenderPassStencilAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, id_MTLTexture, texture);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, NSUInteger_objc, level);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, NSUInteger_objc, slice);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, NSUInteger_objc, depthPlane);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, id_MTLTexture, resolveTexture);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveLevel);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveSlice);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, NSUInteger_objc, resolveDepthPlane);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, MTLLoadAction, loadAction);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor,
                          MTLRenderPassAttachmentDescriptor, MTLStoreAction, storeAction);
GETSET_PROPERTY_INHERITED(MTLRenderPassStencilAttachmentDescriptor, MTLRenderPassAttachmentDescriptor,
                          MTLStoreActionOptions, storeActionOptions);
GETSET_PROPERTY(MTLRenderPassStencilAttachmentDescriptor, uint32_t, clearStencil);
// TODO: MTLMultisampleStencilResolveFilter stencilResolveFilter;

// MTLRenderPassDescriptor
MTLRenderPassDescriptor *NewMTLRenderPassDescriptor();

MTLRenderPassColorAttachmentDescriptor *Get_colorAttachment(MTLRenderPassDescriptor *descriptor,
                                                            uint32_t idx);

GETSET_PROPERTY(MTLRenderPassDescriptor, MTLRenderPassDepthAttachmentDescriptor *, depthAttachment);
GETSET_PROPERTY(MTLRenderPassDescriptor, MTLRenderPassStencilAttachmentDescriptor *,
                stencilAttachment);
GETSET_PROPERTY(MTLRenderPassDescriptor, id_MTLBuffer, visibilityResultBuffer);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, renderTargetArrayLength);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, imageblockSampleLength);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, threadgroupMemoryLength);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, tileWidth);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, tileHeight);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, defaultRasterSampleCount);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, renderTargetWidth);
GETSET_PROPERTY(MTLRenderPassDescriptor, NSUInteger_objc, renderTargetHeight);
// TODO: GETSET_PROPERTY(MTLRenderPassDescriptor, MTLSamplePosition*, samplePositions);
// TODO: GETSET_PROPERTY(MTLRenderPassDescriptor, id_MTLRasterizationRateMap*,
// rasterizationRateMap);
// TODO: GETSET_PROPERTY(MTLRenderPassDescriptor,
// MTLRenderPassSampleBufferAttachmentDescriptorArray*,
// sampleBufferAttachments);
};
