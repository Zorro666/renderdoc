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

class MetalResourceManager;
class MetalReplay;
class WrappedMTLDevice;
class WrappedMTLCommandQueue;
class WrappedMTLCommandBuffer;
class WrappedMTLLibrary;
class WrappedMTLRenderCommandEncoder;
class WrappedMTLRenderPipelineState;
class WrappedMTLBuffer;
class WrappedMTLFunction;

#if defined(__OBJC__)

#import <objc/NSObject.h>

struct id_NSObject
{
  id_NSObject(void *i) : m_Ptr(i) {}
  operator id<NSObject>() { return (id<NSObject>)m_Ptr; }
private:
  void *m_Ptr;
};

#else

struct id_NSObject
{
  id_NSObject(void *i) : m_Ptr(i) {}
  operator void *() { return m_Ptr; }
private:
  void *m_Ptr;
};

#endif

typedef id_NSObject id_MTLObject;

#if defined(__OBJC__)

#import <Metal/MTLBuffer.h>
#import <Metal/MTLCommandBuffer.h>
#import <Metal/MTLCommandQueue.h>
#import <Metal/MTLDevice.h>
#import <Metal/MTLLibrary.h>
#import <Metal/MTLPipeline.h>
#import <Metal/MTLRenderCommandEncoder.h>
#import <Metal/MTLRenderPipeline.h>

#endif

#define METAL_WRAPPED_TYPES(FUNC) \
  FUNC(MTLDevice);                \
  FUNC(MTLLibrary);               \
  FUNC(MTLFunction);              \
  FUNC(MTLRenderPipelineState);   \
  FUNC(MTLCommandQueue);          \
  FUNC(MTLBuffer);                \
  FUNC(MTLCommandBuffer);         \
  FUNC(MTLRenderCommandEncoder);

#define METAL_IDWRAPPED_TYPES(FUNC) FUNC(MTLDrawable);

#if defined(__OBJC__)

#define DECLARE_IDWRAPPED_TYPE(TYPE)                      \
  struct id_##TYPE                                        \
  {                                                       \
    id_##TYPE() : m_Ptr(NULL) {}                          \
    id_##TYPE(id_NSObject i) : m_Ptr((void *)i) {}        \
    id_##TYPE(id<TYPE> i) : m_Ptr((void *)i) {}           \
    operator id<TYPE>() { return (id<TYPE>)m_Ptr; }       \
    operator id_NSObject() { return (id_NSObject)m_Ptr; } \
  private:                                                \
    void *m_Ptr;                                          \
  };

#else

#define DECLARE_IDWRAPPED_TYPE(TYPE)                      \
  struct id_##TYPE                                        \
  {                                                       \
    id_##TYPE() : m_Ptr(NULL) {}                          \
    id_##TYPE(id_MTLObject i) : m_Ptr((void *)i) {}       \
    operator id_NSObject() { return (id_NSObject)m_Ptr; } \
    operator void *() { return m_Ptr; }                   \
  private:                                                \
    void *m_Ptr;                                          \
  };

#endif

METAL_WRAPPED_TYPES(DECLARE_IDWRAPPED_TYPE)
METAL_IDWRAPPED_TYPES(DECLARE_IDWRAPPED_TYPE)
#undef DECLARE_IDWRAPPED_TYPE

#define DECLARE_GET_WRAPPED_FROM_OBJC(TYPE) \
  extern Wrapped##TYPE *GetWrappedFromObjC(id_##TYPE objC);

METAL_WRAPPED_TYPES(DECLARE_GET_WRAPPED_FROM_OBJC)
#undef DECLARE_GET_WRAPPED_FROM_OBJC

#if !defined(__OBJC__)

#if NSINTEGER_DEFINED
#error NSINTEGER_DEFINED is already defined
#endif

typedef long NSInteger;
typedef unsigned long NSUInteger;

struct MTLRenderPipelineDescriptor;
struct MTLRenderPassDescriptor;
struct NSError;
struct NSString;

typedef NSUInteger MTLResourceOptions;
enum class MTLPrimitiveType : NSUInteger;

#endif    // #if !defined(__OBJC__)

struct RD_MTLSize
{
#if defined(__OBJC__)
  RD_MTLSize(MTLSize v) : m_Width(v.width), m_Height(v.height), m_Depth(v.depth) {}
  operator MTLSize() { return MTLSizeMake(m_Width, m_Height, m_Depth); }
#else
  NSUInteger width() { return m_Width; }
  NSUInteger height() { return m_Height; }
  NSUInteger depth() { return m_Depth; }
#endif
private:
  NSUInteger m_Width, m_Height, m_Depth;
};
