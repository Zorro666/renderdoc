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

#include "metal_texture_descriptor.h"
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
void DoSerialise(SerialiserType &ser, MTLTextureDescriptor *&el)
{
  MTLTextureType_objc textureType;
  MTLPixelFormat_objc pixelFormat;
  NSUInteger_objc width;
  NSUInteger_objc height;
  NSUInteger_objc depth;
  NSUInteger_objc mipmapLevelCount;
  NSUInteger_objc sampleCount;
  NSUInteger_objc arrayLength;
  MTLResourceOptions_objc resourceOptions;
  MTLCPUCacheMode_objc cpuCacheMode;
  MTLStorageMode_objc storageMode;
  MTLHazardTrackingMode_objc hazardTrackingMode;
  MTLTextureUsage_objc usage;
  bool allowGPUOptimizedContents;
  MTLTextureSwizzleChannels swizzle;

  if(ser.IsWriting())
  {
    textureType = MTL_GET(el, textureType);
    pixelFormat = MTL_GET(el, pixelFormat);
    width = MTL_GET(el, width);
    height = MTL_GET(el, height);
    depth = MTL_GET(el, depth);
    mipmapLevelCount = MTL_GET(el, mipmapLevelCount);
    sampleCount = MTL_GET(el, sampleCount);
    arrayLength = MTL_GET(el, arrayLength);
    resourceOptions = MTL_GET(el, resourceOptions);
    cpuCacheMode = MTL_GET(el, cpuCacheMode);
    storageMode = MTL_GET(el, storageMode);
    hazardTrackingMode = MTL_GET(el, hazardTrackingMode);
    usage = MTL_GET(el, usage);
    allowGPUOptimizedContents = MTL_GET(el, allowGPUOptimizedContents);
    swizzle = MTL_GET(el, swizzle);
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
    el = MTL::NewMTLTextureDescriptor();
    MTL_SET(el, textureType, textureType);
    MTL_SET(el, pixelFormat, pixelFormat);
    MTL_SET(el, width, width);
    MTL_SET(el, height, height);
    MTL_SET(el, depth, depth);
    MTL_SET(el, mipmapLevelCount, mipmapLevelCount);
    MTL_SET(el, sampleCount, sampleCount);
    MTL_SET(el, arrayLength, arrayLength);
    MTL_SET(el, resourceOptions, resourceOptions);
    MTL_SET(el, cpuCacheMode, cpuCacheMode);
    MTL_SET(el, storageMode, storageMode);
    MTL_SET(el, hazardTrackingMode, hazardTrackingMode);
    MTL_SET(el, usage, usage);
    MTL_SET(el, allowGPUOptimizedContents, allowGPUOptimizedContents);
    MTL_SET(el, swizzle, swizzle);
  }
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLTextureSwizzleChannels &el)
{
  SERIALISE_MEMBER_TYPED(MTLTextureSwizzle_objc, red);
  SERIALISE_MEMBER_TYPED(MTLTextureSwizzle_objc, green);
  SERIALISE_MEMBER_TYPED(MTLTextureSwizzle_objc, blue);
  SERIALISE_MEMBER_TYPED(MTLTextureSwizzle_objc, alpha);
}

INSTANTIATE_SERIALISE_TYPE(MTLTextureDescriptor *);
INSTANTIATE_SERIALISE_TYPE(MTLTextureSwizzleChannels);
