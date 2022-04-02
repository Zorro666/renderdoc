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

#include "metal_helpers_bridge.h"
#import <AppKit/AppKit.h>
#import <Foundation/NSStream.h>
#import <QuartzCore/CAMetalLayer.h>

// helper defined in vk_apple_helpers.mm
void getMetalLayerSize(void *layerHandle, int &width, int &height);

void ObjC::Get_defaultLibraryData(bytebuf &buffer)
{
  NSBundle *mainAppBundle = [NSBundle mainBundle];
  NSString *defaultLibaryPath = [mainAppBundle pathForResource:@"default" ofType:@"metallib"];
  NSData *myData = [NSData dataWithContentsOfFile:defaultLibaryPath];
  dispatch_data_t data = dispatch_data_create(
      myData.bytes, myData.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  NSData *nsData = (NSData *)data;
  buffer.assign((byte *)nsData.bytes, nsData.length);
  dispatch_release(data);
}

MTL::Texture *ObjC::Get_Texture(MTL::Drawable *drawableHandle)
{
  id<CAMetalDrawable> drawable = id<CAMetalDrawable>(drawableHandle);
  return (MTL::Texture *)drawable.texture;
}

CA::MetalLayer *ObjC::Get_Layer(MTL::Drawable *drawableHandle)
{
  id<CAMetalDrawable> drawable = id<CAMetalDrawable>(drawableHandle);
  return (CA::MetalLayer *)drawable.layer;
}

void ObjC::Get_LayerSize(void *layerHandle, int &width, int &height)
{
  ::getMetalLayerSize(layerHandle, width, height);
}

void ObjC::Set_LayerSize(void *layerHandle, int w, int h)
{
  CAMetalLayer *layer = (CAMetalLayer *)layerHandle;
  assert([layer isKindOfClass:[CAMetalLayer class]]);

  CGSize cgSize;
  cgSize.width = w;
  cgSize.height = h;
  layer.drawableSize = cgSize;
}

void ObjC::Set_Device(void *layerHandle, MTL::Device *device)
{
  CAMetalLayer *layer = (CAMetalLayer *)layerHandle;
  assert([layer isKindOfClass:[CAMetalLayer class]]);
  layer.device = id<MTLDevice>(device);
}

void ObjC::Set_FramebufferOnly(void *layerHandle, bool enable)
{
  CAMetalLayer *layer = (CAMetalLayer *)layerHandle;
  assert([layer isKindOfClass:[CAMetalLayer class]]);
  layer.framebufferOnly = enable ? YES : NO;
}

CA::MetalDrawable *ObjC::MTLGetNextDrawable(void *layerHandle)
{
  CAMetalLayer *metalLayer = (CAMetalLayer *)layerHandle;
  RDCASSERT([metalLayer isKindOfClass:[CAMetalLayer class]]);
  metalLayer.pixelFormat = (MTLPixelFormat)MTL::PixelFormatBGRA8Unorm;
  metalLayer.framebufferOnly = NO;
  metalLayer.allowsNextDrawableTimeout = YES;
  CA::MetalDrawable *drawable = (__bridge CA::MetalDrawable *)[metalLayer nextDrawable];
  return drawable;
}
