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

#include "metal_common.h"
#include "metal_device.h"
#include "metal_resources.h"

namespace MTL
{
// MTLRenderPipelineDescriptor
MTLRenderPipelineDescriptor *NewMTLRenderPipelineDescriptor();

GETSET_PROPERTY(MTLRenderPipelineDescriptor, NSString *, label);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, id_MTLFunction, vertexFunction);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, id_MTLFunction, fragmentFunction);
// TODO: vertexDescriptor : MTLVertexDescriptor
// TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
// TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
MTLRenderPipelineColorAttachmentDescriptor *Get_colorAttachment(MTLRenderPipelineDescriptor *descriptor,
                                                                uint32_t idx);

GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLPixelFormat_objc, depthAttachmentPixelFormat);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLPixelFormat_objc, stencilAttachmentPixelFormat);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, NSUInteger_objc, sampleCount);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, bool, alphaToCoverageEnabled);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, bool, alphaToOneEnabled);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, bool, rasterizationEnabled);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLPrimitiveTopologyClass_objc, inputPrimitiveTopology);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, NSUInteger_objc, rasterSampleCount);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, NSUInteger_objc, maxTessellationFactor);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, bool, tessellationFactorScaleEnabled);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLTessellationFactorFormat_objc,
                tessellationFactorFormat);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLTessellationControlPointIndexType_objc,
                tessellationControlPointIndexType);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLTessellationFactorStepFunction_objc,
                tessellationFactorStepFunction);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLWinding_objc, tessellationOutputWindingOrder);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, MTLTessellationPartitionMode_objc,
                tessellationPartitionMode);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, bool, supportIndirectCommandBuffers);
GETSET_PROPERTY(MTLRenderPipelineDescriptor, NSUInteger_objc, maxVertexAmplificationCount);
// TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>

// MTLRenderPipelineColorAttachmentDescriptor
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLPixelFormat, pixelFormat);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, bool, blendingEnabled);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLBlendFactor, sourceRGBBlendFactor);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLBlendFactor,
                destinationRGBBlendFactor);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLBlendOperation, rgbBlendOperation);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLBlendFactor, sourceAlphaBlendFactor);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLBlendFactor,
                destinationAlphaBlendFactor);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLBlendOperation, alphaBlendOperation);
GETSET_PROPERTY(MTLRenderPipelineColorAttachmentDescriptor, MTLColorWriteMask, writeMask);
};
