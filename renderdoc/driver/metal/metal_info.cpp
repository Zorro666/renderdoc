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

#include "metal_info.h"
#include "metal_common.h"
#include "metal_function.h"
#include "metal_manager.h"
#include "metal_texture.h"

void MetalCreationInfo::Pipeline::Init(const RDMTL::RenderPipelineDescriptor &descriptor)
{
  label = descriptor.label;
  vertexFunction = GetResID(descriptor.vertexFunction);
  fragmentFunction = GetResID(descriptor.fragmentFunction);

  int countVertexAttributes = descriptor.vertexDescriptor.attributes.count();
  vertexDescriptor.attributes.resize(countVertexAttributes);
  for(int i = 0; i < countVertexAttributes; ++i)
  {
    vertexDescriptor.attributes[i].format = descriptor.vertexDescriptor.attributes[i].format;
    vertexDescriptor.attributes[i].offset = descriptor.vertexDescriptor.attributes[i].offset;
    vertexDescriptor.attributes[i].bufferIndex =
        descriptor.vertexDescriptor.attributes[i].bufferIndex;
  }

  int countVertexLayouts = descriptor.vertexDescriptor.layouts.count();
  vertexDescriptor.layouts.resize(countVertexLayouts);
  for(int i = 0; i < countVertexLayouts; ++i)
  {
    vertexDescriptor.layouts[i].stride = descriptor.vertexDescriptor.layouts[i].stride;
    vertexDescriptor.layouts[i].stepFunction = descriptor.vertexDescriptor.layouts[i].stepFunction;
    vertexDescriptor.layouts[i].stepRate = descriptor.vertexDescriptor.layouts[i].stepRate;
  }

  sampleCount = descriptor.sampleCount;
  rasterSampleCount = descriptor.rasterSampleCount;
  alphaToCoverageEnabled = descriptor.alphaToCoverageEnabled;
  alphaToOneEnabled = descriptor.alphaToOneEnabled;
  rasterizationEnabled = descriptor.rasterizationEnabled;
  maxVertexAmplificationCount = descriptor.maxVertexAmplificationCount;

  colorAttachments.resize(descriptor.colorAttachments.size());
  for(int i = 0; i < descriptor.colorAttachments.count(); ++i)
  {
    const RDMTL::RenderPipelineColorAttachmentDescriptor &srcAttachment =
        descriptor.colorAttachments[i];
    Attachment &dstAttachment = colorAttachments[i];
    dstAttachment.pixelFormat = srcAttachment.pixelFormat;
    dstAttachment.blendingEnabled = srcAttachment.blendingEnabled;
    dstAttachment.sourceRGBBlendFactor = srcAttachment.sourceAlphaBlendFactor;
    dstAttachment.destinationRGBBlendFactor = srcAttachment.destinationRGBBlendFactor;
    dstAttachment.rgbBlendOperation = srcAttachment.rgbBlendOperation;
    dstAttachment.sourceAlphaBlendFactor = srcAttachment.sourceAlphaBlendFactor;
    dstAttachment.destinationAlphaBlendFactor = srcAttachment.destinationAlphaBlendFactor;
    dstAttachment.alphaBlendOperation = srcAttachment.alphaBlendOperation;
    dstAttachment.writeMask = srcAttachment.writeMask;
  }

  depthAttachmentPixelFormat = descriptor.depthAttachmentPixelFormat;
  stencilAttachmentPixelFormat = descriptor.stencilAttachmentPixelFormat;
  inputPrimitiveTopology = descriptor.inputPrimitiveTopology;
  tessellationPartitionMode = descriptor.tessellationPartitionMode;
  maxTessellationFactor = descriptor.maxTessellationFactor;
  tessellationFactorScaleEnabled = descriptor.tessellationFactorScaleEnabled;
  tessellationFactorFormat = descriptor.tessellationFactorFormat;
  tessellationControlPointIndexType = descriptor.tessellationControlPointIndexType;
  tessellationFactorStepFunction = descriptor.tessellationFactorStepFunction;
  tessellationOutputWindingOrder = descriptor.tessellationOutputWindingOrder;

  vertexBuffers.resize(descriptor.vertexBuffers.size());
  for(int i = 0; i < descriptor.vertexBuffers.count(); ++i)
  {
    vertexBuffers[i].mutability = descriptor.vertexBuffers[i].mutability;
  }
  fragmentBuffers.resize(descriptor.fragmentBuffers.size());
  for(int i = 0; i < descriptor.fragmentBuffers.count(); ++i)
  {
    fragmentBuffers[i].mutability = descriptor.fragmentBuffers[i].mutability;
  }
  supportIndirectCommandBuffers = descriptor.supportIndirectCommandBuffers;
  // TODO: rdcarray<MTL::BinaryArchive*> binaryArchives;
  // TODO: rdcarray<MTL::DynamicLibrary*> vertexPreloadedLibraries;
  // TODO: rdcarray<MTL::DynamicLibrary*> fragmentPreloadedLibraries;
  // TODO: LinkedFunctions vertexLinkedFunctions;
  // TODO: LinkedFunctions fragmentLinkedFunctions;
  supportAddingVertexBinaryFunctions = descriptor.supportAddingVertexBinaryFunctions;
  supportAddingFragmentBinaryFunctions = descriptor.supportAddingFragmentBinaryFunctions;
  maxVertexCallStackDepth = descriptor.maxVertexCallStackDepth;
  maxFragmentCallStackDepth = descriptor.maxFragmentCallStackDepth;
}

void MetalCreationInfo::Pipeline::CopyTo(MetalResourceManager *rm,
                                         RDMTL::RenderPipelineDescriptor &descriptor) const
{
  descriptor.label = label;
  descriptor.vertexFunction = rm->GetCurrentResourceTyped<MTL::Function *>(vertexFunction);
  descriptor.fragmentFunction = rm->GetCurrentResourceTyped<MTL::Function *>(fragmentFunction);

  int countVertexAttributes = vertexDescriptor.attributes.count();
  descriptor.vertexDescriptor.attributes.resize(countVertexAttributes);
  for(int i = 0; i < countVertexAttributes; ++i)
  {
    descriptor.vertexDescriptor.attributes[i].format = vertexDescriptor.attributes[i].format;
    descriptor.vertexDescriptor.attributes[i].offset = vertexDescriptor.attributes[i].offset;
    descriptor.vertexDescriptor.attributes[i].bufferIndex =
        vertexDescriptor.attributes[i].bufferIndex;
  }

  int countVertexLayouts = vertexDescriptor.layouts.count();
  descriptor.vertexDescriptor.layouts.resize(countVertexLayouts);
  for(int i = 0; i < countVertexLayouts; ++i)
  {
    descriptor.vertexDescriptor.layouts[i].stride = vertexDescriptor.layouts[i].stride;
    descriptor.vertexDescriptor.layouts[i].stepFunction = vertexDescriptor.layouts[i].stepFunction;
    descriptor.vertexDescriptor.layouts[i].stepRate = vertexDescriptor.layouts[i].stepRate;
  }

  descriptor.sampleCount = sampleCount;
  descriptor.rasterSampleCount = rasterSampleCount;
  descriptor.alphaToCoverageEnabled = alphaToCoverageEnabled;
  descriptor.alphaToOneEnabled = alphaToOneEnabled;
  descriptor.rasterizationEnabled = rasterizationEnabled;
  descriptor.maxVertexAmplificationCount = maxVertexAmplificationCount;

  descriptor.colorAttachments.resize(colorAttachments.size());
  for(int i = 0; i < colorAttachments.count(); ++i)
  {
    const Attachment &srcAttachment = colorAttachments[i];
    RDMTL::RenderPipelineColorAttachmentDescriptor &dstAttachment = descriptor.colorAttachments[i];
    dstAttachment.pixelFormat = srcAttachment.pixelFormat;
    dstAttachment.blendingEnabled = srcAttachment.blendingEnabled;
    dstAttachment.sourceRGBBlendFactor = srcAttachment.sourceAlphaBlendFactor;
    dstAttachment.destinationRGBBlendFactor = srcAttachment.destinationRGBBlendFactor;
    dstAttachment.rgbBlendOperation = srcAttachment.rgbBlendOperation;
    dstAttachment.sourceAlphaBlendFactor = srcAttachment.sourceAlphaBlendFactor;
    dstAttachment.destinationAlphaBlendFactor = srcAttachment.destinationAlphaBlendFactor;
    dstAttachment.alphaBlendOperation = srcAttachment.alphaBlendOperation;
    dstAttachment.writeMask = srcAttachment.writeMask;
  }

  descriptor.depthAttachmentPixelFormat = depthAttachmentPixelFormat;
  descriptor.stencilAttachmentPixelFormat = stencilAttachmentPixelFormat;
  descriptor.inputPrimitiveTopology = inputPrimitiveTopology;
  descriptor.tessellationPartitionMode = tessellationPartitionMode;
  descriptor.maxTessellationFactor = maxTessellationFactor;
  descriptor.tessellationFactorScaleEnabled = tessellationFactorScaleEnabled;
  descriptor.tessellationFactorFormat = tessellationFactorFormat;
  descriptor.tessellationControlPointIndexType = tessellationControlPointIndexType;
  descriptor.tessellationFactorStepFunction = tessellationFactorStepFunction;
  descriptor.tessellationOutputWindingOrder = tessellationOutputWindingOrder;

  descriptor.vertexBuffers.resize(vertexBuffers.size());
  for(int i = 0; i < vertexBuffers.count(); ++i)
  {
    descriptor.vertexBuffers[i].mutability = vertexBuffers[i].mutability;
  }
  descriptor.fragmentBuffers.resize(fragmentBuffers.size());
  for(int i = 0; i < fragmentBuffers.count(); ++i)
  {
    descriptor.fragmentBuffers[i].mutability = fragmentBuffers[i].mutability;
  }
  descriptor.supportIndirectCommandBuffers = supportIndirectCommandBuffers;
  // TODO: rdcarray<MTL::BinaryArchive*> binaryArchives;
  // TODO: rdcarray<MTL::DynamicLibrary*> vertexPreloadedLibraries;
  // TODO: rdcarray<MTL::DynamicLibrary*> fragmentPreloadedLibraries;
  // TODO: LinkedFunctions vertexLinkedFunctions;
  // TODO: LinkedFunctions fragmentLinkedFunctions;
  descriptor.supportAddingVertexBinaryFunctions = supportAddingVertexBinaryFunctions;
  descriptor.supportAddingFragmentBinaryFunctions = supportAddingFragmentBinaryFunctions;
  descriptor.maxVertexCallStackDepth = maxVertexCallStackDepth;
  descriptor.maxFragmentCallStackDepth = maxFragmentCallStackDepth;
}

void MetalCreationInfo::RenderPass::Init(const RDMTL::RenderPassDescriptor &descriptor)
{
  colorAttachments.resize(descriptor.colorAttachments.count());
  for(int i = 0; i < descriptor.colorAttachments.count(); ++i)
  {
    const RDMTL::RenderPassColorAttachmentDescriptor &srcAttachment = descriptor.colorAttachments[i];
    MetalCreationInfo::RenderPass::ColorAttachment &dstAttachment = colorAttachments[i];
    dstAttachment.Init(srcAttachment);
    dstAttachment.clearColor = srcAttachment.clearColor;
  }

  {
    const RDMTL::RenderPassDepthAttachmentDescriptor &srcAttachment = descriptor.depthAttachment;
    MetalCreationInfo::RenderPass::DepthAttachment &dstAttachment = depthAttachment;
    dstAttachment.Init(srcAttachment);
    dstAttachment.clearDepth = srcAttachment.clearDepth;
    dstAttachment.depthResolveFilter = srcAttachment.depthResolveFilter;
  }
  {
    const RDMTL::RenderPassStencilAttachmentDescriptor &srcAttachment = descriptor.stencilAttachment;
    MetalCreationInfo::RenderPass::StencilAttachment &dstAttachment = stencilAttachment;
    dstAttachment.Init(srcAttachment);
    dstAttachment.clearStencil = srcAttachment.clearStencil;
    dstAttachment.stencilResolveFilter = srcAttachment.stencilResolveFilter;
  }
  // TODO: WrappedMTLBuffer *visibilityResultBuffer;
  renderTargetArrayLength = descriptor.renderTargetArrayLength;
  imageblockSampleLength = descriptor.imageblockSampleLength;
  threadgroupMemoryLength = descriptor.threadgroupMemoryLength;
  tileWidth = descriptor.tileWidth;
  tileHeight = descriptor.tileHeight;
  defaultRasterSampleCount = descriptor.defaultRasterSampleCount;
  renderTargetWidth = descriptor.renderTargetWidth;
  renderTargetHeight = descriptor.renderTargetHeight;
  // TODO: rdcarray<MTL::SamplePosition> samplePositions;
  // TODO MTL::RasterizationRateMap rasterizationRateMap;
  // TODO: rdcarray<RenderPassSampleBufferAttachmentDescriptor> sampleBufferAttachments;
}

void MetalCreationInfo::RenderPass::CopyTo(MetalResourceManager *rm,
                                           RDMTL::RenderPassDescriptor &descriptor) const
{
  descriptor.colorAttachments.resize(colorAttachments.size());
  for(int i = 0; i < colorAttachments.count(); ++i)
  {
    const MetalCreationInfo::RenderPass::ColorAttachment &srcAttachment = colorAttachments[i];
    RDMTL::RenderPassColorAttachmentDescriptor &dstAttachment = descriptor.colorAttachments[i];
    srcAttachment.CopyTo(rm, dstAttachment);
    dstAttachment.clearColor = srcAttachment.clearColor;
  }
  {
    const MetalCreationInfo::RenderPass::DepthAttachment &srcAttachment = depthAttachment;
    RDMTL::RenderPassDepthAttachmentDescriptor &dstAttachment = descriptor.depthAttachment;
    srcAttachment.CopyTo(rm, dstAttachment);
    dstAttachment.clearDepth = srcAttachment.clearDepth;
    dstAttachment.depthResolveFilter = srcAttachment.depthResolveFilter;
  }
  {
    const MetalCreationInfo::RenderPass::StencilAttachment &srcAttachment = stencilAttachment;
    RDMTL::RenderPassStencilAttachmentDescriptor &dstAttachment = descriptor.stencilAttachment;
    srcAttachment.CopyTo(rm, dstAttachment);
    dstAttachment.clearStencil = srcAttachment.clearStencil;
    dstAttachment.stencilResolveFilter = srcAttachment.stencilResolveFilter;
  }
  // TODO: when WrappedMTLBuffer exists
  // WrappedMTLBuffer *visibilityResultBuffer;
  descriptor.renderTargetArrayLength = renderTargetArrayLength;
  descriptor.imageblockSampleLength = imageblockSampleLength;
  descriptor.threadgroupMemoryLength = threadgroupMemoryLength;
  descriptor.tileWidth = tileWidth;
  descriptor.tileHeight = tileHeight;
  descriptor.defaultRasterSampleCount = defaultRasterSampleCount;
  descriptor.renderTargetWidth = renderTargetWidth;
  descriptor.renderTargetHeight = renderTargetHeight;
  // TODO : rdcarray<MTL::SamplePosition> descriptor.samplePositions;
  // TODO: will MTL::RasterizationRateMap need to be a wrapped resource
  // MTL::RasterizationRateMap rasterizationRateMap;
  // TODO: rdcarray<RenderPassSampleBufferAttachmentDescriptor> descriptor.sampleBufferAttachments;
}

void MetalCreationInfo::RenderPass::Attachment::Init(
    const RDMTL::RenderPassAttachmentDescriptor &attachment)
{
  texture = GetResID(attachment.texture);
  level = attachment.level;
  slice = attachment.slice;
  depthPlane = attachment.depthPlane;
  resolveTexture = GetResID(attachment.resolveTexture);
  resolveLevel = attachment.resolveLevel;
  resolveSlice = attachment.resolveSlice;
  resolveDepthPlane = attachment.resolveDepthPlane;
  loadAction = attachment.loadAction;
  storeAction = attachment.storeAction;
  storeActionOptions = attachment.storeActionOptions;
}

void MetalCreationInfo::RenderPass::Attachment::CopyTo(
    MetalResourceManager *rm, RDMTL::RenderPassAttachmentDescriptor &attachment) const
{
  attachment.texture = rm->GetCurrentResourceTyped<MTL::Texture *>(texture);
  attachment.level = level;
  attachment.slice = slice;
  attachment.depthPlane = depthPlane;
  attachment.resolveTexture = rm->GetCurrentResourceTyped<MTL::Texture *>(resolveTexture);
  attachment.resolveLevel = resolveLevel;
  attachment.resolveSlice = resolveSlice;
  attachment.resolveDepthPlane = resolveDepthPlane;
  attachment.loadAction = loadAction;
  attachment.storeAction = storeAction;
  attachment.storeActionOptions = storeActionOptions;
}

void MetalCreationInfo::Texture::Init(MetalResourceManager *rm, MetalCreationInfo &creationInfo,
                                      const RDMTL::TextureDescriptor &descriptor)
{
  textureType = descriptor.textureType;
  pixelFormat = descriptor.pixelFormat;
  width = descriptor.width;
  height = descriptor.height;
  depth = descriptor.depth;
  mipmapLevelCount = descriptor.mipmapLevelCount;
  sampleCount = RDCMAX(1LLU, descriptor.sampleCount);
  arrayLength = descriptor.arrayLength;
  resourceOptions = descriptor.resourceOptions;
  cpuCacheMode = descriptor.cpuCacheMode;
  storageMode = descriptor.storageMode;
  hazardTrackingMode = descriptor.hazardTrackingMode;
  usage = descriptor.usage;
  allowGPUOptimizedContents = descriptor.allowGPUOptimizedContents;
  swizzle = descriptor.swizzle;

  creationFlags = TextureCategory::NoFlags;

  /*
    if(pCreateInfo->usage & VK_IMAGE_USAGE_SAMPLED_BIT)
      creationFlags |= TextureCategory::ShaderRead;
    if(pCreateInfo->usage &
       (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT))
      creationFlags |= TextureCategory::ColorTarget;
    if(pCreateInfo->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
      creationFlags |= TextureCategory::DepthTarget;
    if(pCreateInfo->usage & VK_IMAGE_USAGE_STORAGE_BIT)
      creationFlags |= TextureCategory::ShaderReadWrite;
   */

  creationFlags |= TextureCategory::ColorTarget;
  cube = (textureType == MTL::TextureTypeCube);
}
