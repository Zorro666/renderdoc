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
    rdcarray<Attachment> attachments;

    bool alphaToCoverageEnabled;
    bool alphaToOneEnabled;
    bool rasterizationEnabled;
  };

  std::unordered_map<ResourceId, Pipeline> m_Pipeline;

  struct RenderPass
  {
    struct Attachment
    {
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
    rdcarray<ColorAttachment> colorAttachments;
  };

  std::unordered_map<ResourceId, RenderPass> m_RenderPass;

  struct Texture
  {
    void Init(MetalResourceManager *rm, MetalCreationInfo &creationInfo,
              RDMTL::TextureDescriptor &descriptor);

    MTL::Size extent;
    TextureCategory creationFlags;
    MTL::TextureType type;
    MTL::PixelFormat format;
    uint32_t arrayLayers;
    uint32_t mipLevels;
    uint32_t samples;
    bool cube;
  };

  std::unordered_map<ResourceId, Texture> m_Texture;
};
