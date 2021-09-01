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
#include "core/core.h"
#include "metal_function.h"
#include "metal_manager.h"
#include "metal_resources.h"

// MTLRenderPipeline.h
template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLRenderPipelineDescriptor *&el)
{
  if(ser.IsReading())
  {
    RDCASSERT(el == NULL);
    el = MTL::NewMTLRenderPipelineDescriptor();
  }

  NSString *label = NULL;
  WrappedMTLFunction *vertexFunction_ = NULL;
  WrappedMTLFunction *fragmentFunction_ = NULL;
  // TODO: vertexDescriptor : MTLVertexDescriptor
  // TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
  // TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
  MTLRenderPipelineColorAttachmentDescriptor *colorAttachments[MAX_RENDER_PASS_COLOR_ATTACHMENTS];
  MTLPixelFormat_objc depthAttachmentPixelFormat;
  MTLPixelFormat_objc stencilAttachmentPixelFormat;
  NSUInteger_objc sampleCount;
  bool alphaToCoverageEnabled_ = false;
  bool alphaToOneEnabled_ = false;
  bool rasterizationEnabled_ = false;
  MTLPrimitiveTopologyClass_objc inputPrimitiveTopology;
  NSUInteger_objc rasterSampleCount = NSUInteger_Zero;
  NSUInteger_objc maxTessellationFactor = NSUInteger_Zero;
  bool tessellationFactorScaleEnabled_ = false;
  MTLTessellationFactorFormat_objc tessellationFactorFormat;
  MTLTessellationControlPointIndexType_objc tessellationControlPointIndexType;
  MTLTessellationFactorStepFunction_objc tessellationFactorStepFunction;
  MTLWinding_objc tessellationOutputWindingOrder;
  MTLTessellationPartitionMode_objc tessellationPartitionMode;
  bool supportIndirectCommandBuffers_ = false;
  NSUInteger_objc maxVertexAmplificationCount = NSUInteger_Zero;
  // TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>

  for(uint32_t i = 0; i < MAX_RENDER_PASS_COLOR_ATTACHMENTS; ++i)
  {
    colorAttachments[i] = MTL_GET(el, colorAttachment, i);
  }

  if(ser.IsWriting())
  {
    label = MTL_GET(el, label);
    vertexFunction_ = GetWrapped(MTL_GET(el, vertexFunction));
    fragmentFunction_ = GetWrapped(MTL_GET(el, fragmentFunction));
    // TODO: vertexDescriptor : MTLVertexDescriptor
    // TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
    // TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
    // TODO: colorAttachments : MTLRenderPipelineColorAttachmentDescriptorArray *
    depthAttachmentPixelFormat = MTL_GET(el, depthAttachmentPixelFormat);
    stencilAttachmentPixelFormat = MTL_GET(el, stencilAttachmentPixelFormat);
    sampleCount = MTL_GET(el, sampleCount);
    alphaToCoverageEnabled_ = MTL_GET(el, alphaToCoverageEnabled);
    alphaToOneEnabled_ = MTL_GET(el, alphaToOneEnabled);
    rasterizationEnabled_ = MTL_GET(el, rasterizationEnabled);
    inputPrimitiveTopology = MTL_GET(el, inputPrimitiveTopology);
    rasterSampleCount = MTL_GET(el, rasterSampleCount);
    maxTessellationFactor = MTL_GET(el, maxTessellationFactor);
    tessellationFactorScaleEnabled_ = MTL_GET(el, tessellationFactorScaleEnabled);
    tessellationFactorFormat = MTL_GET(el, tessellationFactorFormat);
    tessellationControlPointIndexType = MTL_GET(el, tessellationControlPointIndexType);
    tessellationFactorStepFunction = MTL_GET(el, tessellationFactorStepFunction);
    tessellationOutputWindingOrder = MTL_GET(el, tessellationOutputWindingOrder);
    tessellationPartitionMode = MTL_GET(el, tessellationPartitionMode);
    supportIndirectCommandBuffers_ = MTL_GET(el, supportIndirectCommandBuffers);
    maxVertexAmplificationCount = MTL_GET(el, maxVertexAmplificationCount);
    // TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>
  }
  SERIALISE_ELEMENT(label);
  SERIALISE_ELEMENT_LOCAL(vertexFunction, GetResID(vertexFunction_)).TypedAs("MTLFunction"_lit);
  SERIALISE_ELEMENT_LOCAL(fragmentFunction, GetResID(fragmentFunction_)).TypedAs("MTLFunction"_lit);
  // TODO: vertexDescriptor : MTLVertexDescriptor
  // TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
  // TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
  SERIALISE_ELEMENT(colorAttachments);
  SERIALISE_ELEMENT(depthAttachmentPixelFormat);
  SERIALISE_ELEMENT(stencilAttachmentPixelFormat);
  SERIALISE_ELEMENT(sampleCount);
  SERIALISE_ELEMENT_LOCAL(alphaToCoverageEnabled, alphaToCoverageEnabled_).TypedAs("BOOL"_lit);
  SERIALISE_ELEMENT_LOCAL(alphaToOneEnabled, alphaToOneEnabled_).TypedAs("BOOL"_lit);
  SERIALISE_ELEMENT_LOCAL(rasterizationEnabled, rasterizationEnabled_).TypedAs("BOOL"_lit);
  SERIALISE_ELEMENT(inputPrimitiveTopology);
  SERIALISE_ELEMENT(rasterSampleCount);
  SERIALISE_ELEMENT(maxTessellationFactor);
  SERIALISE_ELEMENT_LOCAL(tessellationFactorScaleEnabled, tessellationFactorScaleEnabled_)
      .TypedAs("BOOL"_lit);
  SERIALISE_ELEMENT(tessellationFactorFormat);
  SERIALISE_ELEMENT(tessellationControlPointIndexType);
  SERIALISE_ELEMENT(tessellationFactorStepFunction);
  SERIALISE_ELEMENT(tessellationOutputWindingOrder);
  SERIALISE_ELEMENT(tessellationPartitionMode);
  SERIALISE_ELEMENT_LOCAL(supportIndirectCommandBuffers, supportIndirectCommandBuffers_)
      .TypedAs("BOOL"_lit);
  SERIALISE_ELEMENT(maxVertexAmplificationCount);
  // TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>
  if(ser.IsReading())
  {
    RDCASSERT(el != NULL);
    MTL_SET(el, label, label);
    MetalResourceManager *rm = (MetalResourceManager *)ser.GetUserData();

    id_MTLFunction objcVertexFunction = GetObjCWrappedResource<id_MTLFunction>(rm, vertexFunction);
    MTL_SET(el, vertexFunction, objcVertexFunction);
    id_MTLFunction objcFragmentFunction =
        GetObjCWrappedResource<id_MTLFunction>(rm, fragmentFunction);
    MTL_SET(el, fragmentFunction, objcFragmentFunction);
    // TODO: vertexDescriptor : MTLVertexDescriptor
    // TODO: vertexBuffers : MTLPipelineBufferDescriptorArray *
    // TODO: fragmentBuffers : MTLPipelineBufferDescriptorArray *
    MTL_SET(el, depthAttachmentPixelFormat, depthAttachmentPixelFormat);
    MTL_SET(el, stencilAttachmentPixelFormat, stencilAttachmentPixelFormat);
    MTL_SET(el, sampleCount, sampleCount);
    MTL_SET(el, alphaToCoverageEnabled, alphaToCoverageEnabled);
    MTL_SET(el, alphaToOneEnabled, alphaToOneEnabled);
    MTL_SET(el, rasterizationEnabled, rasterizationEnabled);
    MTL_SET(el, inputPrimitiveTopology, inputPrimitiveTopology);
    MTL_SET(el, rasterSampleCount, rasterSampleCount);
    MTL_SET(el, maxTessellationFactor, maxTessellationFactor);
    MTL_SET(el, tessellationFactorScaleEnabled, tessellationFactorScaleEnabled);
    MTL_SET(el, tessellationFactorFormat, tessellationFactorFormat);
    MTL_SET(el, tessellationControlPointIndexType, tessellationControlPointIndexType);
    MTL_SET(el, tessellationFactorStepFunction, tessellationFactorStepFunction);
    MTL_SET(el, tessellationOutputWindingOrder, tessellationOutputWindingOrder);
    MTL_SET(el, tessellationPartitionMode, tessellationPartitionMode);
    MTL_SET(el, supportIndirectCommandBuffers, supportIndirectCommandBuffers);
    MTL_SET(el, maxVertexAmplificationCount, maxVertexAmplificationCount);
    // TODO: binaryArchives : NSArray<id<MTLBinaryArchive>>
  }
}

// MTLRenderPipelineColorAttachmentDescriptor
// MTLPixelFormat pixelFormat;
// BOOL blendingEnabled;
// MTLBlendFactor sourceRGBBlendFactor;
// MTLBlendFactor destinationRGBBlendFactor;
// MTLBlendOperation rgbBlendOperation;
// MTLBlendFactor sourceAlphaBlendFactor;
// MTLBlendFactor destinationAlphaBlendFactor;
// MTLBlendOperation alphaBlendOperation;
// MTLColorWriteMask writeMask;

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLRenderPipelineColorAttachmentDescriptor *&el)
{
  MTLPixelFormat_objc pixelFormat;
  bool blendingEnabled_;
  MTLBlendFactor_objc sourceRGBBlendFactor;
  MTLBlendFactor_objc destinationRGBBlendFactor;
  MTLBlendOperation_objc rgbBlendOperation;
  MTLBlendFactor_objc sourceAlphaBlendFactor;
  MTLBlendFactor_objc destinationAlphaBlendFactor;
  MTLBlendOperation_objc alphaBlendOperation;
  MTLColorWriteMask_objc writeMask;

  if(ser.IsWriting())
  {
    pixelFormat = (MTLPixelFormat_objc)MTL_GET(el, pixelFormat);
    blendingEnabled_ = MTL_GET(el, blendingEnabled);
    sourceRGBBlendFactor = (MTLBlendFactor_objc)MTL_GET(el, sourceRGBBlendFactor);
    destinationRGBBlendFactor = (MTLBlendFactor_objc)MTL_GET(el, destinationRGBBlendFactor);
    rgbBlendOperation = (MTLBlendOperation_objc)MTL_GET(el, rgbBlendOperation);
    sourceAlphaBlendFactor = (MTLBlendFactor_objc)MTL_GET(el, sourceAlphaBlendFactor);
    destinationAlphaBlendFactor = (MTLBlendFactor_objc)MTL_GET(el, destinationAlphaBlendFactor);
    alphaBlendOperation = (MTLBlendOperation_objc)MTL_GET(el, alphaBlendOperation);
    writeMask = (MTLColorWriteMask_objc)MTL_GET(el, writeMask);
  }

  SERIALISE_ELEMENT(pixelFormat);
  SERIALISE_ELEMENT_LOCAL(blendingEnabled, blendingEnabled_).TypedAs("BOOL"_lit);
  SERIALISE_ELEMENT(sourceRGBBlendFactor);
  SERIALISE_ELEMENT(destinationRGBBlendFactor);
  SERIALISE_ELEMENT(rgbBlendOperation);
  SERIALISE_ELEMENT(sourceAlphaBlendFactor);
  SERIALISE_ELEMENT(destinationAlphaBlendFactor);
  SERIALISE_ELEMENT(alphaBlendOperation);
  SERIALISE_ELEMENT(writeMask);

  if(ser.IsReading())
  {
    RDCASSERT(el != NULL);
    MTL_SET(el, pixelFormat, (MTLPixelFormat)pixelFormat);
    MTL_SET(el, blendingEnabled, blendingEnabled);
    MTL_SET(el, sourceRGBBlendFactor, (MTLBlendFactor)sourceRGBBlendFactor);
    MTL_SET(el, destinationRGBBlendFactor, (MTLBlendFactor)destinationRGBBlendFactor);
    MTL_SET(el, rgbBlendOperation, (MTLBlendOperation)rgbBlendOperation);
    MTL_SET(el, sourceAlphaBlendFactor, (MTLBlendFactor)sourceAlphaBlendFactor);
    MTL_SET(el, destinationAlphaBlendFactor, (MTLBlendFactor)destinationAlphaBlendFactor);
    MTL_SET(el, alphaBlendOperation, (MTLBlendOperation)alphaBlendOperation);
    MTL_SET(el, writeMask, (MTLColorWriteMask)writeMask);
  }
}

INSTANTIATE_SERIALISE_TYPE(MTLRenderPipelineDescriptor *);
INSTANTIATE_SERIALISE_TYPE(MTLRenderPipelineColorAttachmentDescriptor *);
