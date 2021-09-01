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

#include <os/availability.h>
#include "metal_common.h"
#include "metal_device.h"
#include "metal_resources.h"

// MTLTexture.h
namespace MTL
{
// MTLTextureDescriptor
MTLTextureDescriptor *NewMTLTextureDescriptor();
GETSET_PROPERTY(MTLTextureDescriptor, MTLTextureType_objc, textureType);
GETSET_PROPERTY(MTLTextureDescriptor, MTLPixelFormat_objc, pixelFormat);
GETSET_PROPERTY(MTLTextureDescriptor, NSUInteger_objc, width);
GETSET_PROPERTY(MTLTextureDescriptor, NSUInteger_objc, height);
GETSET_PROPERTY(MTLTextureDescriptor, NSUInteger_objc, depth);
GETSET_PROPERTY(MTLTextureDescriptor, NSUInteger_objc, mipmapLevelCount);
GETSET_PROPERTY(MTLTextureDescriptor, NSUInteger_objc, sampleCount);
GETSET_PROPERTY(MTLTextureDescriptor, NSUInteger_objc, arrayLength);
GETSET_PROPERTY(MTLTextureDescriptor, MTLResourceOptions_objc, resourceOptions);
GETSET_PROPERTY(MTLTextureDescriptor, MTLCPUCacheMode_objc, cpuCacheMode);
GETSET_PROPERTY(MTLTextureDescriptor, MTLStorageMode_objc, storageMode);
GETSET_PROPERTY(MTLTextureDescriptor, MTLHazardTrackingMode_objc, hazardTrackingMode);
GETSET_PROPERTY(MTLTextureDescriptor, MTLTextureUsage_objc, usage);
GETSET_PROPERTY(MTLTextureDescriptor, bool, allowGPUOptimizedContents);
GETSET_PROPERTY(MTLTextureDescriptor, MTLTextureSwizzleChannels, swizzle);
};
