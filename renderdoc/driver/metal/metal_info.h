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

#pragma once

#include "metal_common.h"

struct MetalCreationInfo
{
  struct Pipeline
  {
    void Init(const RDMTL::RenderPipelineDescriptor &descriptor);
    void CopyTo(MetalResourceManager *rm, RDMTL::RenderPipelineDescriptor &descriptor) const;

    struct Attachment
    {
      MTL::BlendOperation alphaBlendOperation;
      MTL::BlendFactor sourceAlphaBlendFactor;
      MTL::BlendFactor destinationAlphaBlendFactor;

      MTL::BlendOperation rgbBlendOperation;
      MTL::BlendFactor sourceRGBBlendFactor;
      MTL::BlendFactor destinationRGBBlendFactor;

      MTL::PixelFormat pixelFormat;
      MTL::ColorWriteMask writeMask;
      bool blendingEnabled;
    };

    struct PipelineBufferDescriptor
    {
      MTL::Mutability mutability;
    };

    struct VertexDescriptor
    {
      struct VertexBufferLayoutDescriptor
      {
        NS::UInteger stride = 0;
        MTL::VertexStepFunction stepFunction = MTL::VertexStepFunctionPerVertex;
        NS::UInteger stepRate = 1;
      };

      struct VertexAttributeDescriptor
      {
        MTL::VertexFormat format;
        NS::UInteger offset;
        NS::UInteger bufferIndex;
      };

      rdcarray<VertexBufferLayoutDescriptor> layouts;
      rdcarray<VertexAttributeDescriptor> attributes;
    };

    rdcstr label;
    ResourceId vertexFunction;
    ResourceId fragmentFunction;
    VertexDescriptor vertexDescriptor;
    NS::UInteger sampleCount;
    NS::UInteger rasterSampleCount;
    bool alphaToCoverageEnabled;
    bool alphaToOneEnabled;
    bool rasterizationEnabled;
    NS::UInteger maxVertexAmplificationCount;
    rdcarray<Attachment> colorAttachments;
    MTL::PixelFormat depthAttachmentPixelFormat;
    MTL::PixelFormat stencilAttachmentPixelFormat;
    MTL::PrimitiveTopologyClass inputPrimitiveTopology;
    MTL::TessellationPartitionMode tessellationPartitionMode;
    NS::UInteger maxTessellationFactor;
    bool tessellationFactorScaleEnabled;
    MTL::TessellationFactorFormat tessellationFactorFormat;
    MTL::TessellationControlPointIndexType tessellationControlPointIndexType;
    MTL::TessellationFactorStepFunction tessellationFactorStepFunction;
    MTL::Winding tessellationOutputWindingOrder;
    rdcarray<PipelineBufferDescriptor> vertexBuffers;
    rdcarray<PipelineBufferDescriptor> fragmentBuffers;
    bool supportIndirectCommandBuffers;
    // TODO: rdcarray<MTL::BinaryArchive*> binaryArchives;
    // TODO: rdcarray<MTL::DynamicLibrary*> vertexPreloadedLibraries;
    // TODO: rdcarray<MTL::DynamicLibrary*> fragmentPreloadedLibraries;
    // TODO: LinkedFunctions vertexLinkedFunctions;
    // TODO: LinkedFunctions fragmentLinkedFunctions;
    bool supportAddingVertexBinaryFunctions;
    bool supportAddingFragmentBinaryFunctions;
    NS::UInteger maxVertexCallStackDepth;
    NS::UInteger maxFragmentCallStackDepth;
  };

  std::unordered_map<ResourceId, Pipeline> m_Pipeline;

  struct RenderPass
  {
    void Init(const RDMTL::RenderPassDescriptor &descriptor);
    void CopyTo(MetalResourceManager *rm, RDMTL::RenderPassDescriptor &descriptor) const;

    struct Attachment
    {
      void Init(const RDMTL::RenderPassAttachmentDescriptor &attachment);
      void CopyTo(MetalResourceManager *rm, RDMTL::RenderPassAttachmentDescriptor &attachment) const;
      ResourceId texture;
      NS::UInteger level;
      NS::UInteger slice;
      NS::UInteger depthPlane;
      MTL::LoadAction loadAction;
      MTL::StoreAction storeAction;
      MTL::StoreActionOptions storeActionOptions;
      ResourceId resolveTexture;
      NS::UInteger resolveLevel;
      NS::UInteger resolveSlice;
      NS::UInteger resolveDepthPlane;
    };

    struct ColorAttachment : Attachment
    {
      MTL::ClearColor clearColor;
    };

    struct DepthAttachment : Attachment
    {
      double clearDepth;
      MTL::MultisampleDepthResolveFilter depthResolveFilter;
    };

    struct StencilAttachment : Attachment
    {
      uint32_t clearStencil;
      MTL::MultisampleStencilResolveFilter stencilResolveFilter;
    };

    rdcarray<ColorAttachment> colorAttachments;
    DepthAttachment depthAttachment;
    StencilAttachment stencilAttachment;
    // TODO: WrappedMTLBuffer *visibilityResultBuffer;
    NS::UInteger renderTargetArrayLength;
    NS::UInteger imageblockSampleLength;
    NS::UInteger threadgroupMemoryLength;
    NS::UInteger tileWidth;
    NS::UInteger tileHeight;
    NS::UInteger defaultRasterSampleCount;
    NS::UInteger renderTargetWidth;
    NS::UInteger renderTargetHeight;
    // TODO: rdcarray<MTL::SamplePosition> samplePositions;
    // TODO MTL::RasterizationRateMap rasterizationRateMap;
    // TODO: rdcarray<RenderPassSampleBufferAttachmentDescriptor> sampleBufferAttachments;
  };

  std::unordered_map<ResourceId, RenderPass> m_RenderPass;

  struct Texture
  {
    void Init(MetalResourceManager *rm, MetalCreationInfo &creationInfo,
              const RDMTL::TextureDescriptor &descriptor);

    MTL::TextureType textureType;
    MTL::PixelFormat pixelFormat;
    NS::UInteger width;
    NS::UInteger height;
    NS::UInteger depth;
    NS::UInteger mipmapLevelCount;
    NS::UInteger sampleCount;
    NS::UInteger arrayLength;
    MTL::ResourceOptions resourceOptions;
    MTL::CPUCacheMode cpuCacheMode;
    MTL::StorageMode storageMode;
    MTL::HazardTrackingMode hazardTrackingMode;
    MTL::TextureUsage usage;
    bool allowGPUOptimizedContents;
    MTL::TextureSwizzleChannels swizzle;

    TextureCategory creationFlags;
    bool cube;
  };

  std::unordered_map<ResourceId, Texture> m_Texture;
};
