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
#import <Metal/MTLTexture.h>

// MTLTextureDescriptor
MTLTextureDescriptor *MTL::NewMTLTextureDescriptor()
{
  return [MTLTextureDescriptor new];
}

void MTL::Set_textureType(MTLTextureDescriptor *descriptor, MTLTextureType_objc textureType)
{
  descriptor.textureType = (MTLTextureType)textureType;
}

void MTL::Set_pixelFormat(MTLTextureDescriptor *descriptor, MTLPixelFormat_objc pixelFormat)
{
  descriptor.pixelFormat = (MTLPixelFormat)pixelFormat;
}

void MTL::Set_width(MTLTextureDescriptor *descriptor, NSUInteger_objc width)
{
  descriptor.width = width;
}

void MTL::Set_height(MTLTextureDescriptor *descriptor, NSUInteger_objc height)
{
  descriptor.height = height;
}

void MTL::Set_depth(MTLTextureDescriptor *descriptor, NSUInteger_objc depth)
{
  descriptor.depth = depth;
}

void MTL::Set_mipmapLevelCount(MTLTextureDescriptor *descriptor, NSUInteger_objc mipmapLevelCount)
{
  descriptor.mipmapLevelCount = mipmapLevelCount;
}

void MTL::Set_sampleCount(MTLTextureDescriptor *descriptor, NSUInteger_objc sampleCount)
{
  descriptor.sampleCount = sampleCount;
}

void MTL::Set_arrayLength(MTLTextureDescriptor *descriptor, NSUInteger_objc arrayLength)
{
  descriptor.arrayLength = arrayLength;
}

void MTL::Set_resourceOptions(MTLTextureDescriptor *descriptor,
                              MTLResourceOptions_objc resourceOptions)
{
  descriptor.resourceOptions = resourceOptions;
}

void MTL::Set_cpuCacheMode(MTLTextureDescriptor *descriptor, MTLCPUCacheMode_objc cpuCacheMode)
{
  descriptor.cpuCacheMode = (MTLCPUCacheMode)cpuCacheMode;
}

void MTL::Set_storageMode(MTLTextureDescriptor *descriptor, MTLStorageMode_objc storageMode)
{
  descriptor.storageMode = (MTLStorageMode)storageMode;
}

void MTL::Set_hazardTrackingMode(MTLTextureDescriptor *descriptor,
                                 MTLHazardTrackingMode_objc hazardTrackingMode)
{
  descriptor.hazardTrackingMode = (MTLHazardTrackingMode)hazardTrackingMode;
}

void MTL::Set_usage(MTLTextureDescriptor *descriptor, MTLTextureUsage_objc usage)
{
  descriptor.usage = (MTLTextureUsage_objc)usage;
}

void MTL::Set_allowGPUOptimizedContents(MTLTextureDescriptor *descriptor,
                                        bool allowGPUOptimizedContents)
{
  descriptor.allowGPUOptimizedContents = allowGPUOptimizedContents;
}

void MTL::Set_swizzle(MTLTextureDescriptor *descriptor, MTLTextureSwizzleChannels swizzle)
{
  descriptor.swizzle = swizzle;
}

MTLTextureType_objc MTL::Get_textureType(MTLTextureDescriptor *descriptor)
{
  return (MTLTextureType_objc)descriptor.textureType;
}

MTLPixelFormat_objc MTL::Get_pixelFormat(MTLTextureDescriptor *descriptor)
{
  return (MTLPixelFormat_objc)descriptor.pixelFormat;
}

NSUInteger_objc MTL::Get_width(MTLTextureDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.width;
}

NSUInteger_objc MTL::Get_height(MTLTextureDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.height;
}

NSUInteger_objc MTL::Get_depth(MTLTextureDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.depth;
}

NSUInteger_objc MTL::Get_mipmapLevelCount(MTLTextureDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.mipmapLevelCount;
}

NSUInteger_objc MTL::Get_sampleCount(MTLTextureDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.sampleCount;
}

NSUInteger_objc MTL::Get_arrayLength(MTLTextureDescriptor *descriptor)
{
  return (NSUInteger_objc)descriptor.arrayLength;
}

MTLResourceOptions_objc MTL::Get_resourceOptions(MTLTextureDescriptor *descriptor)
{
  return (MTLResourceOptions_objc)descriptor.resourceOptions;
}

MTLCPUCacheMode_objc MTL::Get_cpuCacheMode(MTLTextureDescriptor *descriptor)
{
  return (MTLCPUCacheMode_objc)descriptor.cpuCacheMode;
}

MTLStorageMode_objc MTL::Get_storageMode(MTLTextureDescriptor *descriptor)
{
  return (MTLStorageMode_objc)descriptor.storageMode;
}

MTLHazardTrackingMode_objc MTL::Get_hazardTrackingMode(MTLTextureDescriptor *descriptor)
{
  return (MTLHazardTrackingMode_objc)descriptor.hazardTrackingMode;
}

MTLTextureUsage_objc MTL::Get_usage(MTLTextureDescriptor *descriptor)
{
  return (MTLTextureUsage_objc)descriptor.usage;
}

bool MTL::Get_allowGPUOptimizedContents(MTLTextureDescriptor *descriptor)
{
  return descriptor.allowGPUOptimizedContents;
}

MTLTextureSwizzleChannels MTL::Get_swizzle(MTLTextureDescriptor *descriptor)
{
  return descriptor.swizzle;
}
