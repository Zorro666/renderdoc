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

#include "core/core.h"
#include "metal_manager.h"
#include "metal_resources.h"

// MTLTextureDescriptor
// MTLTextureType textureType;
// MTLPixelFormat pixelFormat;
// NSUInteger width;
// NSUInteger height;
// NSUInteger depth;
// NSUInteger mipmapLevelCount;
// NSUInteger sampleCount;
// NSUInteger arrayLength;
// MTLResourceOptions resourceOptions;
// MTLCPUCacheMode cpuCacheMode
// MTLStorageMode storageMode
// MTLHazardTrackingMode hazardTrackingMode
// MTLTextureUsage usage
// BOOL allowGPUOptimizedContents
// MTLTextureSwizzleChannels swizzle

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTL::TextureDescriptor *&el)
{
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

  if(ser.IsWriting())
  {
    textureType = el->textureType();
    pixelFormat = el->pixelFormat();
    width = el->width();
    height = el->height();
    depth = el->depth();
    mipmapLevelCount = el->mipmapLevelCount();
    sampleCount = el->sampleCount();
    arrayLength = el->arrayLength();
    resourceOptions = el->resourceOptions();
    cpuCacheMode = el->cpuCacheMode();
    storageMode = el->storageMode();
    hazardTrackingMode = el->hazardTrackingMode();
    usage = el->usage();
    allowGPUOptimizedContents = el->allowGPUOptimizedContents();
    swizzle = el->swizzle();
  }

  SERIALISE_ELEMENT(textureType);
  SERIALISE_ELEMENT(pixelFormat);
  SERIALISE_ELEMENT(width);
  SERIALISE_ELEMENT(height);
  SERIALISE_ELEMENT(depth);
  SERIALISE_ELEMENT(mipmapLevelCount);
  SERIALISE_ELEMENT(sampleCount);
  SERIALISE_ELEMENT(arrayLength);
  SERIALISE_ELEMENT(resourceOptions);
  SERIALISE_ELEMENT(cpuCacheMode);
  SERIALISE_ELEMENT(storageMode);
  SERIALISE_ELEMENT(hazardTrackingMode);
  SERIALISE_ELEMENT(usage);
  SERIALISE_ELEMENT(allowGPUOptimizedContents);
  SERIALISE_ELEMENT(swizzle);

  if(ser.IsReading())
  {
    RDCASSERT(el == NULL);
    el = MTL::TextureDescriptor::alloc();
    el->setTextureType(textureType);
    el->setPixelFormat(pixelFormat);
    el->setWidth(width);
    el->setHeight(height);
    el->setDepth(depth);
    el->setMipmapLevelCount(mipmapLevelCount);
    el->setSampleCount(sampleCount);
    el->setArrayLength(arrayLength);
    el->setResourceOptions(resourceOptions);
    el->setCpuCacheMode(cpuCacheMode);
    el->setStorageMode(storageMode);
    el->setHazardTrackingMode(hazardTrackingMode);
    el->setUsage(usage);
    el->setAllowGPUOptimizedContents(allowGPUOptimizedContents);
    el->setSwizzle(swizzle);
  }
}

INSTANTIATE_SERIALISE_TYPE(MTL::TextureDescriptor *);
