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

void MetalCreationInfo::Texture::Init(MetalResourceManager *rm, MetalCreationInfo &creationInfo,
                                      MTL::TextureDescriptor *descriptor)
{
  type = descriptor->textureType();
  format = descriptor->pixelFormat();
  extent.width = descriptor->width();
  extent.height = descriptor->height();
  extent.depth = descriptor->depth();
  arrayLayers = descriptor->arrayLength();
  mipLevels = descriptor->mipmapLevelCount();
  samples = RDCMAX(1LLU, descriptor->sampleCount());
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
  cube = (type == MTL::TextureTypeCube);
}
