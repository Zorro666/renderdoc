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

#include "metal_render_pipeline_descriptor.h"
#import <Metal/MTLRenderPipeline.h>

// MTLRenderPipelineDescriptor
MTLRenderPipelineDescriptor *MTL::NewMTLRenderPipelineDescriptor()
{
  return [MTLRenderPipelineDescriptor new];
}

void MTL::Set_label(MTLRenderPipelineDescriptor *descriptor, NSString *label)
{
  descriptor.label = label;
}

void MTL::Set_vertexFunction(MTLRenderPipelineDescriptor *descriptor, id_MTLFunction vertexFunction)
{
  descriptor.vertexFunction = vertexFunction;
}

void MTL::Set_fragmentFunction(MTLRenderPipelineDescriptor *descriptor,
                               id_MTLFunction fragmentFunction)
{
  descriptor.fragmentFunction = fragmentFunction;
}

// TODO: vertexDescriptor : MTLVertexDescriptor
// TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
// TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
// TODO: colorAttachments : MTLRenderPipelineColorAttachmentDescriptorArray *

void MTL::Set_depthAttachmentPixelFormat(MTLRenderPipelineDescriptor *descriptor,
                                         MTLPixelFormat_objc depthAttachmentPixelFormat)
{
  descriptor.depthAttachmentPixelFormat = (MTLPixelFormat)depthAttachmentPixelFormat;
}

void MTL::Set_stencilAttachmentPixelFormat(MTLRenderPipelineDescriptor *descriptor,
                                           MTLPixelFormat_objc stencilAttachmentPixelFormat)
{
  descriptor.stencilAttachmentPixelFormat = (MTLPixelFormat)stencilAttachmentPixelFormat;
}

void MTL::Set_sampleCount(MTLRenderPipelineDescriptor *descriptor, NSUInteger_objc sampleCount)
{
  descriptor.sampleCount = sampleCount;
}

void MTL::Set_alphaToCoverageEnabled(MTLRenderPipelineDescriptor *descriptor,
                                     bool alphaToCoverageEnabled)
{
  descriptor.alphaToCoverageEnabled = alphaToCoverageEnabled;
}

void MTL::Set_alphaToOneEnabled(MTLRenderPipelineDescriptor *descriptor, bool alphaToOneEnabled)
{
  descriptor.alphaToOneEnabled = alphaToOneEnabled;
}

void MTL::Set_rasterizationEnabled(MTLRenderPipelineDescriptor *descriptor, bool rasterizationEnabled)
{
  descriptor.rasterizationEnabled = rasterizationEnabled;
}

void MTL::Set_inputPrimitiveTopology(MTLRenderPipelineDescriptor *descriptor,
                                     MTLPrimitiveTopologyClass_objc inputPrimitiveTopology)
{
  descriptor.inputPrimitiveTopology = (MTLPrimitiveTopologyClass)inputPrimitiveTopology;
}

void MTL::Set_rasterSampleCount(MTLRenderPipelineDescriptor *descriptor,
                                NSUInteger_objc rasterSampleCount)
{
  descriptor.rasterSampleCount = rasterSampleCount;
}

void MTL::Set_maxTessellationFactor(MTLRenderPipelineDescriptor *descriptor,
                                    NSUInteger_objc maxTessellationFactor)
{
  descriptor.maxTessellationFactor = maxTessellationFactor;
}

void MTL::Set_tessellationFactorScaleEnabled(MTLRenderPipelineDescriptor *descriptor,
                                             bool tessellationFactorScaleEnabled)
{
  descriptor.tessellationFactorScaleEnabled = tessellationFactorScaleEnabled;
}

void MTL::Set_tessellationFactorFormat(MTLRenderPipelineDescriptor *descriptor,
                                       MTLTessellationFactorFormat_objc tessellationFactorFormat)
{
  descriptor.tessellationFactorFormat = (MTLTessellationFactorFormat)tessellationFactorFormat;
}

void MTL::Set_tessellationControlPointIndexType(
    MTLRenderPipelineDescriptor *descriptor,
    MTLTessellationControlPointIndexType_objc tessellationControlPointIndexType)
{
  descriptor.tessellationControlPointIndexType =
      (MTLTessellationControlPointIndexType)tessellationControlPointIndexType;
}

void MTL::Set_tessellationFactorStepFunction(
    MTLRenderPipelineDescriptor *descriptor,
    MTLTessellationFactorStepFunction_objc tessellationFactorStepFunction)
{
  descriptor.tessellationFactorStepFunction =
      (MTLTessellationFactorStepFunction)tessellationFactorStepFunction;
}

void MTL::Set_tessellationOutputWindingOrder(MTLRenderPipelineDescriptor *descriptor,
                                             MTLWinding_objc tessellationOutputWindingOrder)
{
  descriptor.tessellationOutputWindingOrder = (MTLWinding)tessellationOutputWindingOrder;
}

void MTL::Set_tessellationPartitionMode(MTLRenderPipelineDescriptor *descriptor,
                                        MTLTessellationPartitionMode_objc tessellationPartitionMode)
{
  descriptor.tessellationPartitionMode = (MTLTessellationPartitionMode)tessellationPartitionMode;
}

void MTL::Set_supportIndirectCommandBuffers(MTLRenderPipelineDescriptor *descriptor,
                                            bool supportIndirectCommandBuffers)
{
  descriptor.supportIndirectCommandBuffers = supportIndirectCommandBuffers;
}

void MTL::Set_maxVertexAmplificationCount(MTLRenderPipelineDescriptor *descriptor,
                                          NSUInteger_objc maxVertexAmplificationCount)
{
  if(@available(macOS 10.15.4, *))
  {
    descriptor.maxVertexAmplificationCount = maxVertexAmplificationCount;
  }
}

// TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>

NSString *MTL::Get_label(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.label;
}

id_MTLFunction MTL::Get_vertexFunction(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.vertexFunction;
}

id_MTLFunction MTL::Get_fragmentFunction(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.fragmentFunction;
}

// TODO: vertexFunction : MTLFunction
// TODO: fragmentFunction : MTLFunction
// TODO: vertexDescriptor : MTLVertexDescriptor
// TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
// TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *

MTLRenderPipelineColorAttachmentDescriptor *MTL::Get_colorAttachment(
    MTLRenderPipelineDescriptor *descriptor, uint32_t idx)
{
  return descriptor.colorAttachments[idx];
}

MTLPixelFormat_objc MTL::Get_depthAttachmentPixelFormat(MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLPixelFormat_objc)descriptor.depthAttachmentPixelFormat;
}

MTLPixelFormat_objc MTL::Get_stencilAttachmentPixelFormat(MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLPixelFormat_objc)descriptor.stencilAttachmentPixelFormat;
}

NSUInteger_objc MTL::Get_sampleCount(MTLRenderPipelineDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.sampleCount;
}

bool MTL::Get_alphaToCoverageEnabled(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.alphaToCoverageEnabled;
}

bool MTL::Get_alphaToOneEnabled(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.alphaToOneEnabled;
}

bool MTL::Get_rasterizationEnabled(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.rasterizationEnabled;
}

MTLPrimitiveTopologyClass_objc MTL::Get_inputPrimitiveTopology(MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLPrimitiveTopologyClass_objc)descriptor.inputPrimitiveTopology;
}

NSUInteger_objc MTL::Get_rasterSampleCount(MTLRenderPipelineDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.rasterSampleCount;
}

NSUInteger_objc MTL::Get_maxTessellationFactor(MTLRenderPipelineDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.maxTessellationFactor;
}

bool MTL::Get_tessellationFactorScaleEnabled(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.tessellationFactorScaleEnabled;
}

MTLTessellationFactorFormat_objc MTL::Get_tessellationFactorFormat(MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLTessellationFactorFormat_objc)descriptor.tessellationFactorFormat;
}

MTLTessellationControlPointIndexType_objc MTL::Get_tessellationControlPointIndexType(
    MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLTessellationControlPointIndexType_objc)descriptor.tessellationControlPointIndexType;
}

MTLTessellationFactorStepFunction_objc MTL::Get_tessellationFactorStepFunction(
    MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLTessellationFactorStepFunction_objc)descriptor.tessellationFactorStepFunction;
}

MTLWinding_objc MTL::Get_tessellationOutputWindingOrder(MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLWinding_objc)descriptor.tessellationOutputWindingOrder;
}

MTLTessellationPartitionMode_objc MTL::Get_tessellationPartitionMode(
    MTLRenderPipelineDescriptor *descriptor)
{
  return (MTLTessellationPartitionMode_objc)descriptor.tessellationPartitionMode;
}

bool MTL::Get_supportIndirectCommandBuffers(MTLRenderPipelineDescriptor *descriptor)
{
  return descriptor.supportIndirectCommandBuffers;
}

NSUInteger_objc MTL::Get_maxVertexAmplificationCount(MTLRenderPipelineDescriptor *descriptor)
{
  if(@available(macOS 10.15.4, *))
  {
    return (NSUInteger_objc)descriptor.maxVertexAmplificationCount;
  }
  else
  {
    // Fallback on earlier versions
    return NSUInteger_Zero;
  }
}

// TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>

// MTLRenderPipelineColorAttachmentDescriptor
void MTL::Set_pixelFormat(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                          MTLPixelFormat pixelFormat)
{
  descriptor.pixelFormat = pixelFormat;
}

void MTL::Set_blendingEnabled(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                              bool blendingEnabled)
{
  descriptor.blendingEnabled = blendingEnabled;
}

void MTL::Set_sourceRGBBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                                   MTLBlendFactor sourceRGBBlendFactor)
{
  descriptor.sourceRGBBlendFactor = sourceRGBBlendFactor;
}

void MTL::Set_destinationRGBBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                                        MTLBlendFactor destinationRGBBlendFactor)
{
  descriptor.destinationRGBBlendFactor = destinationRGBBlendFactor;
}

void MTL::Set_rgbBlendOperation(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                                MTLBlendOperation rgbBlendOperation)
{
  descriptor.rgbBlendOperation = rgbBlendOperation;
}

void MTL::Set_sourceAlphaBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                                     MTLBlendFactor sourceAlphaBlendFactor)
{
  descriptor.sourceAlphaBlendFactor = sourceAlphaBlendFactor;
}

void MTL::Set_destinationAlphaBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                                          MTLBlendFactor destinationAlphaBlendFactor)
{
  descriptor.destinationAlphaBlendFactor = destinationAlphaBlendFactor;
}

void MTL::Set_alphaBlendOperation(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                                  MTLBlendOperation alphaBlendOperation)
{
  descriptor.alphaBlendOperation = alphaBlendOperation;
}

void MTL::Set_writeMask(MTLRenderPipelineColorAttachmentDescriptor *descriptor,
                        MTLColorWriteMask writeMask)
{
  descriptor.writeMask = writeMask;
}

MTLPixelFormat MTL::Get_pixelFormat(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.pixelFormat;
}

bool MTL::Get_blendingEnabled(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.blendingEnabled;
}

MTLBlendFactor MTL::Get_sourceRGBBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.sourceRGBBlendFactor;
}

MTLBlendFactor MTL::Get_destinationRGBBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.destinationRGBBlendFactor;
}

MTLBlendOperation MTL::Get_rgbBlendOperation(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.rgbBlendOperation;
}

MTLBlendFactor MTL::Get_sourceAlphaBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.sourceAlphaBlendFactor;
}

MTLBlendFactor MTL::Get_destinationAlphaBlendFactor(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.destinationAlphaBlendFactor;
}

MTLBlendOperation MTL::Get_alphaBlendOperation(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.alphaBlendOperation;
}

MTLColorWriteMask MTL::Get_writeMask(MTLRenderPipelineColorAttachmentDescriptor *descriptor)
{
  return descriptor.writeMask;
}
