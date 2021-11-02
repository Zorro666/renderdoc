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

#include <stdint.h>
#include <string.h>
#include "api/replay/rdcstr.h"
#include "common/common.h"
#include "serialise/serialiser.h"

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
class WrappedMTLTexture;

enum NSInteger_objc : int64_t
{
  NSInteger_Zero = 0
};

enum NSUInteger_objc : uint64_t
{
  NSUInteger_Zero = 0
};

#if defined(__OBJC__)

#import <objc/NSObject.h>

struct id_NSObject
{
  id_NSObject(void *i) : m_Ptr(i) {}
  operator id<NSObject>() { return (__bridge id<NSObject>)m_Ptr; }
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

#import <IOSurface/IOSurfaceAPI.h>
#import <Metal/MTLBuffer.h>
#import <Metal/MTLCommandBuffer.h>
#import <Metal/MTLCommandQueue.h>
#import <Metal/MTLDevice.h>
#import <Metal/MTLLibrary.h>
#import <Metal/MTLPipeline.h>
#import <Metal/MTLRenderCommandEncoder.h>
#import <Metal/MTLRenderPass.h>
#import <Metal/MTLRenderPipeline.h>
#import <Metal/MTLTexture.h>
#import <QuartzCore/CAMetalLayer.h>

#else

struct MTLDevice;
struct MTLCommandQueue;
struct MTLCommandBuffer;
struct MTLLibrary;
struct MTLRenderCommandEncoder;
struct MTLRenderPipelineState;
struct MTLBuffer;
struct MTLFunction;
struct MTLTexture;
struct __IOSurface;
typedef __IOSurface *IOSurfaceRef;

#endif

// TODO: where does Metal specify it is max 8 color attachments
const uint32_t MAX_RENDER_PASS_COLOR_ATTACHMENTS = 8;

#define METAL_WRAPPED_PROTOCOLS(FUNC) \
  FUNC(MTLDevice);                    \
  FUNC(MTLCommandQueue);              \
  FUNC(MTLCommandBuffer);             \
  FUNC(MTLLibrary);                   \
  FUNC(MTLRenderCommandEncoder);      \
  FUNC(MTLRenderPipelineState);       \
  FUNC(MTLBuffer);                    \
  FUNC(MTLFunction);                  \
  FUNC(MTLTexture);

#define METAL_IDWRAPPED_PROTOCOLS(FUNC) \
  FUNC(MTLDrawable);                    \
  FUNC(CAMetalDrawable);

// NS_ENUM
#define METAL_ENUMS(FUNC)                     \
  FUNC(MTLPrimitiveType);                     \
  FUNC(MTLPixelFormat);                       \
  FUNC(MTLPrimitiveTopologyClass);            \
  FUNC(MTLWinding);                           \
  FUNC(MTLDepthClipMode);                     \
  FUNC(MTLTriangleFillMode);                  \
  FUNC(MTLCullMode);                          \
  FUNC(MTLTessellationFactorFormat);          \
  FUNC(MTLTessellationControlPointIndexType); \
  FUNC(MTLTessellationFactorStepFunction);    \
  FUNC(MTLTessellationPartitionMode);         \
  FUNC(MTLCPUCacheMode);                      \
  FUNC(MTLStorageMode);                       \
  FUNC(MTLHazardTrackingMode);                \
  FUNC(MTLTextureType);                       \
  FUNC(MTLLoadAction);                        \
  FUNC(MTLStoreAction);                       \
  FUNC(MTLBlendFactor);                       \
  FUNC(MTLBlendOperation)

// NS_OPTIONS
#define METAL_OPTIONS(FUNC)    \
  FUNC(MTLResourceOptions);    \
  FUNC(MTLTextureUsage);       \
  FUNC(MTLStoreActionOptions); \
  FUNC(MTLColorWriteMask);

#if defined(__OBJC__)

#define DECLARE_IDWRAPPED_PROTOCOL(TYPE)                           \
  struct id_##TYPE                                                 \
  {                                                                \
    id_##TYPE() : m_Ptr(NULL) {}                                   \
    id_##TYPE(id_NSObject i) : m_Ptr((__bridge void *)i) {}        \
    id_##TYPE(id<TYPE> i) : m_Ptr((__bridge void *)i) {}           \
    operator id<TYPE>() { return (__bridge id<TYPE>)m_Ptr; }       \
    operator id_NSObject() { return (__bridge id_NSObject)m_Ptr; } \
  private:                                                         \
    void *m_Ptr;                                                   \
  };

#define DECLARE_METAL_ENUM(TYPE) enum TYPE##_objc : uint64_t;
#define DECLARE_METAL_OPTION(TYPE) enum TYPE##_objc : uint64_t;

#else

#define DECLARE_IDWRAPPED_PROTOCOL(TYPE)                  \
  struct id_##TYPE                                        \
  {                                                       \
    id_##TYPE() : m_Ptr(NULL) {}                          \
    id_##TYPE(id_MTLObject i) : m_Ptr((void *)i) {}       \
    operator id_NSObject() { return (id_NSObject)m_Ptr; } \
    operator void *() { return m_Ptr; }                   \
  private:                                                \
    void *m_Ptr;                                          \
  };

#define DECLARE_METAL_ENUM_OPTION_INNER(TYPE)       \
  enum TYPE##_objc : uint64_t;                      \
  const TYPE##_objc TYPE##_Zero = (TYPE##_objc)0UL; \
  template <>                                       \
  inline rdcliteral TypeName<TYPE##_objc>()         \
  {                                                 \
    return STRING_LITERAL(#TYPE);                   \
  }                                                 \
  RDCCOMPILE_ASSERT(sizeof(TYPE##_objc) == sizeof(TYPE), "enum size does not match");

#define DECLARE_METAL_ENUM(TYPE) \
  enum TYPE : NSUInteger;        \
  DECLARE_METAL_ENUM_OPTION_INNER(TYPE)

#define DECLARE_METAL_OPTION(TYPE) \
  typedef NSUInteger TYPE;         \
  DECLARE_METAL_ENUM_OPTION_INNER(TYPE)

#endif

METAL_WRAPPED_PROTOCOLS(DECLARE_IDWRAPPED_PROTOCOL)
METAL_IDWRAPPED_PROTOCOLS(DECLARE_IDWRAPPED_PROTOCOL)
#undef DECLARE_IDWRAPPED_PROTOCOL

#define DECLARE_OBJC_HELPERS(TYPE)                  \
  extern Wrapped##TYPE *GetWrapped(id_##TYPE objC); \
  extern id_##TYPE GetReal(id_##TYPE objC);         \
  extern ResourceId GetId(id_##TYPE objC);

METAL_WRAPPED_PROTOCOLS(DECLARE_OBJC_HELPERS)
#undef DECLARE_OBJC_HELPERS

#if defined(__OBJC__)

enum MTLTextureSwizzle_objc : uint8_t;

#else

enum MTLTextureSwizzle : uint8_t;
enum MTLTextureSwizzle_objc : uint8_t;
const MTLTextureSwizzle_objc MTLTextureSwizzle_Zero = (MTLTextureSwizzle_objc)0UL;

template <>
inline rdcliteral TypeName<MTLTextureSwizzle_objc>()
{
  return "MTLTextureSwizzle"_lit;
}
RDCCOMPILE_ASSERT(sizeof(MTLTextureSwizzle_objc) == sizeof(MTLTextureSwizzle),
                  "enum size does not match");

#if NSINTEGER_DEFINED
#error NSINTEGER_DEFINED is already defined
#endif

typedef long NSInteger;
typedef unsigned long NSUInteger;

struct MTLRenderPassColorAttachmentDescriptorArray;
struct MTLRenderPassSampleBufferAttachmentDescriptorArray;
struct MTLRenderPipelineDescriptor;
struct MTLRenderPipelineColorAttachmentDescriptor;
struct MTLRenderPassDescriptor;
struct MTLRenderPassAttachmentDescriptor;
struct MTLRenderPassColorAttachmentDescriptor;
struct MTLRenderPassDepthAttachmentDescriptor;
struct MTLRenderPassStencilAttachmentDescriptor;
struct MTLTextureDescriptor;
struct MTLCompileOptions;

template <>
inline rdcliteral TypeName<NSInteger_objc>()
{
  return "NSInteger"_lit;
}

template <>
inline rdcliteral TypeName<NSUInteger_objc>()
{
  return "NSUInteger"_lit;
}

struct NSError;
struct NSString;

template <>
inline rdcliteral TypeName<NSString *>()
{
  return "NSString"_lit;
}
template <class SerialiserType>
void DoSerialise(SerialiserType &ser, NSString *&el);

// NSRange.h
typedef struct _NSRange
{
  NSUInteger location;
  NSUInteger length;
} NSRange;

// NSRange.h
inline NSRange NSMakeRange(NSUInteger loc, NSUInteger len)
{
  NSRange r;
  r.location = loc;
  r.length = len;
  return r;
}

// MTLTypes.h
typedef struct
{
  NSUInteger width, height, depth;
} MTLSize;

// MTLTypes.h
typedef struct
{
  float x, y;
} MTLSamplePosition;

// MTLRenderPass.h
typedef struct
{
  double red;
  double green;
  double blue;
  double alpha;
} MTLClearColor;

// MTLTexture.h
typedef struct
{
  MTLTextureSwizzle red;
  MTLTextureSwizzle green;
  MTLTextureSwizzle blue;
  MTLTextureSwizzle alpha;
} MTLTextureSwizzleChannels;

// MTLRenderCommandEncoder.h
enum MTLPrimitiveType : NSUInteger
{
  MTLPrimitiveTypePoint = 0,
  MTLPrimitiveTypeLine = 1,
  MTLPrimitiveTypeLineStrip = 2,
  MTLPrimitiveTypeTriangle = 3,
  MTLPrimitiveTypeTriangleStrip = 4,
};

// MTLRenderCommandEncoder.h
typedef struct
{
  NSUInteger x, y, width, height;
} MTLScissorRect;

// MTLRenderCommandEncoder.h
typedef struct
{
  double originX, originY, width, height, znear, zfar;
} MTLViewport;

// MTLRenderCommandEncoder.h
enum MTLDepthClipMode : NSUInteger
{
  MTLDepthClipModeClip = 0,
  MTLDepthClipModeClamp = 1,
};

// MTLRenderCommandEncoder.h
enum MTLTriangleFillMode : NSUInteger
{
  MTLTriangleFillModeFill = 0,
  MTLTriangleFillModeLines = 1,
};

// MTLRenderCommandEncoder.h
enum MTLCullMode : NSUInteger
{
  MTLCullModeNone = 0,
  MTLCullModeFront = 1,
  MTLCullModeBack = 2,
};

// MTLRenderCommandEncoder.h
enum MTLWinding : NSUInteger
{
  MTLWindingClockwise = 0,
  MTLWindingCounterClockwise = 1,
};

// MTLRenderPipeline.h
enum MTLTessellationFactorFormat : NSUInteger
{
  MTLTessellationFactorFormatHalf = 0,
};

// MTLRenderPipeline.h
enum MTLPrimitiveTopologyClass : NSUInteger
{
  MTLPrimitiveTopologyClassUnspecified = 0,
  MTLPrimitiveTopologyClassPoint = 1,
  MTLPrimitiveTopologyClassLine = 2,
  MTLPrimitiveTopologyClassTriangle = 3,
};

// MTLRenderPipeline.h
enum MTLTessellationControlPointIndexType : NSUInteger
{
  MTLTessellationControlPointIndexTypeNone = 0,
  MTLTessellationControlPointIndexTypeUInt16 = 1,
  MTLTessellationControlPointIndexTypeUInt32 = 2,
};

// MTLRenderPipeline.h
enum MTLTessellationFactorStepFunction : NSUInteger
{
  MTLTessellationFactorStepFunctionConstant = 0,
  MTLTessellationFactorStepFunctionPerPatch = 1,
  MTLTessellationFactorStepFunctionPerInstance = 2,
  MTLTessellationFactorStepFunctionPerPatchAndPerInstance = 3,
};

// MTLRenderPipeline.h
enum MTLTessellationPartitionMode : NSUInteger
{
  MTLTessellationPartitionModePow2 = 0,
  MTLTessellationPartitionModeInteger = 1,
  MTLTessellationPartitionModeFractionalOdd = 2,
  MTLTessellationPartitionModeFractionalEven = 3,
};

// MTLResource.h
enum MTLCPUCacheMode : NSUInteger
{
  MTLCPUCacheModeDefaultCache = 0,
  MTLCPUCacheModeWriteCombined = 1,
};

// MTLPixelFormat.h
enum MTLPixelFormat : NSUInteger
{
  MTLPixelFormatInvalid = 0,

  /* Normal 8 bit formats */

  MTLPixelFormatA8Unorm = 1,

  MTLPixelFormatR8Unorm = 10,
  MTLPixelFormatR8Unorm_sRGB = 11,
  MTLPixelFormatR8Snorm = 12,
  MTLPixelFormatR8Uint = 13,
  MTLPixelFormatR8Sint = 14,

  /* Normal 16 bit formats */

  MTLPixelFormatR16Unorm = 20,
  MTLPixelFormatR16Snorm = 22,
  MTLPixelFormatR16Uint = 23,
  MTLPixelFormatR16Sint = 24,
  MTLPixelFormatR16Float = 25,

  MTLPixelFormatRG8Unorm = 30,
  MTLPixelFormatRG8Unorm_sRGB = 31,
  MTLPixelFormatRG8Snorm = 32,
  MTLPixelFormatRG8Uint = 33,
  MTLPixelFormatRG8Sint = 34,

  /* Packed 16 bit formats */

  MTLPixelFormatB5G6R5Unorm = 40,
  MTLPixelFormatA1BGR5Unorm = 41,
  MTLPixelFormatABGR4Unorm = 42,
  MTLPixelFormatBGR5A1Unorm = 43,

  /* Normal 32 bit formats */

  MTLPixelFormatR32Uint = 53,
  MTLPixelFormatR32Sint = 54,
  MTLPixelFormatR32Float = 55,

  MTLPixelFormatRG16Unorm = 60,
  MTLPixelFormatRG16Snorm = 62,
  MTLPixelFormatRG16Uint = 63,
  MTLPixelFormatRG16Sint = 64,
  MTLPixelFormatRG16Float = 65,

  MTLPixelFormatRGBA8Unorm = 70,
  MTLPixelFormatRGBA8Unorm_sRGB = 71,
  MTLPixelFormatRGBA8Snorm = 72,
  MTLPixelFormatRGBA8Uint = 73,
  MTLPixelFormatRGBA8Sint = 74,

  MTLPixelFormatBGRA8Unorm = 80,
  MTLPixelFormatBGRA8Unorm_sRGB = 81,

  /* Packed 32 bit formats */

  MTLPixelFormatRGB10A2Unorm = 90,
  MTLPixelFormatRGB10A2Uint = 91,

  MTLPixelFormatRG11B10Float = 92,
  MTLPixelFormatRGB9E5Float = 93,

  MTLPixelFormatBGR10A2Unorm = 94,

  MTLPixelFormatBGR10_XR = 554,
  MTLPixelFormatBGR10_XR_sRGB = 555,

  /* Normal 64 bit formats */

  MTLPixelFormatRG32Uint = 103,
  MTLPixelFormatRG32Sint = 104,
  MTLPixelFormatRG32Float = 105,

  MTLPixelFormatRGBA16Unorm = 110,
  MTLPixelFormatRGBA16Snorm = 112,
  MTLPixelFormatRGBA16Uint = 113,
  MTLPixelFormatRGBA16Sint = 114,
  MTLPixelFormatRGBA16Float = 115,

  MTLPixelFormatBGRA10_XR = 552,
  MTLPixelFormatBGRA10_XR_sRGB = 553,

  /* Normal 128 bit formats */

  MTLPixelFormatRGBA32Uint = 123,
  MTLPixelFormatRGBA32Sint = 124,
  MTLPixelFormatRGBA32Float = 125,

  /* Compressed formats. */

  /* S3TC/DXT */
  MTLPixelFormatBC1_RGBA = 130,
  MTLPixelFormatBC1_RGBA_sRGB = 131,
  MTLPixelFormatBC2_RGBA = 132,
  MTLPixelFormatBC2_RGBA_sRGB = 133,
  MTLPixelFormatBC3_RGBA = 134,
  MTLPixelFormatBC3_RGBA_sRGB = 135,

  /* RGTC */
  MTLPixelFormatBC4_RUnorm = 140,
  MTLPixelFormatBC4_RSnorm = 141,
  MTLPixelFormatBC5_RGUnorm = 142,
  MTLPixelFormatBC5_RGSnorm = 143,

  /* BPTC */
  MTLPixelFormatBC6H_RGBFloat = 150,
  MTLPixelFormatBC6H_RGBUfloat = 151,
  MTLPixelFormatBC7_RGBAUnorm = 152,
  MTLPixelFormatBC7_RGBAUnorm_sRGB = 153,

  /* PVRTC */
  MTLPixelFormatPVRTC_RGB_2BPP = 160,
  MTLPixelFormatPVRTC_RGB_2BPP_sRGB = 161,
  MTLPixelFormatPVRTC_RGB_4BPP = 162,
  MTLPixelFormatPVRTC_RGB_4BPP_sRGB = 163,
  MTLPixelFormatPVRTC_RGBA_2BPP = 164,
  MTLPixelFormatPVRTC_RGBA_2BPP_sRGB = 165,
  MTLPixelFormatPVRTC_RGBA_4BPP = 166,
  MTLPixelFormatPVRTC_RGBA_4BPP_sRGB = 167,

  /* ETC2 */
  MTLPixelFormatEAC_R11Unorm = 170,
  MTLPixelFormatEAC_R11Snorm = 172,
  MTLPixelFormatEAC_RG11Unorm = 174,
  MTLPixelFormatEAC_RG11Snorm = 176,
  MTLPixelFormatEAC_RGBA8 = 178,
  MTLPixelFormatEAC_RGBA8_sRGB = 179,

  MTLPixelFormatETC2_RGB8 = 180,
  MTLPixelFormatETC2_RGB8_sRGB = 181,
  MTLPixelFormatETC2_RGB8A1 = 182,
  MTLPixelFormatETC2_RGB8A1_sRGB = 183,

  /* ASTC */
  MTLPixelFormatASTC_4x4_sRGB = 186,
  MTLPixelFormatASTC_5x4_sRGB = 187,
  MTLPixelFormatASTC_5x5_sRGB = 188,
  MTLPixelFormatASTC_6x5_sRGB = 189,
  MTLPixelFormatASTC_6x6_sRGB = 190,
  MTLPixelFormatASTC_8x5_sRGB = 192,
  MTLPixelFormatASTC_8x6_sRGB = 193,
  MTLPixelFormatASTC_8x8_sRGB = 194,
  MTLPixelFormatASTC_10x5_sRGB = 195,
  MTLPixelFormatASTC_10x6_sRGB = 196,
  MTLPixelFormatASTC_10x8_sRGB = 197,
  MTLPixelFormatASTC_10x10_sRGB = 198,
  MTLPixelFormatASTC_12x10_sRGB = 199,
  MTLPixelFormatASTC_12x12_sRGB = 200,

  MTLPixelFormatASTC_4x4_LDR = 204,
  MTLPixelFormatASTC_5x4_LDR = 205,
  MTLPixelFormatASTC_5x5_LDR = 206,
  MTLPixelFormatASTC_6x5_LDR = 207,
  MTLPixelFormatASTC_6x6_LDR = 208,
  MTLPixelFormatASTC_8x5_LDR = 210,
  MTLPixelFormatASTC_8x6_LDR = 211,
  MTLPixelFormatASTC_8x8_LDR = 212,
  MTLPixelFormatASTC_10x5_LDR = 213,
  MTLPixelFormatASTC_10x6_LDR = 214,
  MTLPixelFormatASTC_10x8_LDR = 215,
  MTLPixelFormatASTC_10x10_LDR = 216,
  MTLPixelFormatASTC_12x10_LDR = 217,
  MTLPixelFormatASTC_12x12_LDR = 218,

  // ASTC HDR (High Dynamic Range) Formats
  MTLPixelFormatASTC_4x4_HDR = 222,
  MTLPixelFormatASTC_5x4_HDR = 223,
  MTLPixelFormatASTC_5x5_HDR = 224,
  MTLPixelFormatASTC_6x5_HDR = 225,
  MTLPixelFormatASTC_6x6_HDR = 226,
  MTLPixelFormatASTC_8x5_HDR = 228,
  MTLPixelFormatASTC_8x6_HDR = 229,
  MTLPixelFormatASTC_8x8_HDR = 230,
  MTLPixelFormatASTC_10x5_HDR = 231,
  MTLPixelFormatASTC_10x6_HDR = 232,
  MTLPixelFormatASTC_10x8_HDR = 233,
  MTLPixelFormatASTC_10x10_HDR = 234,
  MTLPixelFormatASTC_12x10_HDR = 235,
  MTLPixelFormatASTC_12x12_HDR = 236,

  MTLPixelFormatGBGR422 = 240,
  MTLPixelFormatBGRG422 = 241,

  /* Depth */

  MTLPixelFormatDepth16Unorm = 250,
  MTLPixelFormatDepth32Float = 252,

  /* Stencil */

  MTLPixelFormatStencil8 = 253,

  /* Depth Stencil */

  MTLPixelFormatDepth24Unorm_Stencil8 = 255,
  MTLPixelFormatDepth32Float_Stencil8 = 260,

  MTLPixelFormatX32_Stencil8 = 261,
  MTLPixelFormatX24_Stencil8 = 262,
};

// MTLResource.h
enum MTLStorageMode : NSUInteger
{
  MTLStorageModeShared = 0,
  MTLStorageModeManaged = 1,
  MTLStorageModePrivate = 2,
  MTLStorageModeMemoryless = 3,
};

// MTLResource.h
enum MTLHazardTrackingMode : NSUInteger
{
  MTLHazardTrackingModeDefault = 0,
  MTLHazardTrackingModeUntracked = 1,
  MTLHazardTrackingModeTracked = 2,
};

// MTLResource.h
#define MTLResourceCPUCacheModeShift 0
#define MTLResourceCPUCacheModeMask (0xfUL << MTLResourceCPUCacheModeShift)

#define MTLResourceStorageModeShift 4
#define MTLResourceStorageModeMask (0xfUL << MTLResourceStorageModeShift)

#define MTLResourceHazardTrackingModeShift 8UL
#define MTLResourceHazardTrackingModeMask (0x3UL << MTLResourceHazardTrackingModeShift)

// MTLResource.h
enum
{
  MTLResourceCPUCacheModeDefaultCache = MTLCPUCacheModeDefaultCache << MTLResourceCPUCacheModeShift,
  MTLResourceCPUCacheModeWriteCombined = MTLCPUCacheModeWriteCombined << MTLResourceCPUCacheModeShift,

  MTLResourceStorageModeShared = MTLStorageModeShared << MTLResourceStorageModeShift,
  MTLResourceStorageModeManaged = MTLStorageModeManaged << MTLResourceStorageModeShift,
  MTLResourceStorageModePrivate = MTLStorageModePrivate << MTLResourceStorageModeShift,
  MTLResourceStorageModeMemoryless = MTLStorageModeMemoryless << MTLResourceStorageModeShift,

  MTLResourceHazardTrackingModeDefault = MTLHazardTrackingModeDefault
                                         << MTLResourceHazardTrackingModeShift,
  MTLResourceHazardTrackingModeUntracked = MTLHazardTrackingModeUntracked
                                           << MTLResourceHazardTrackingModeShift,
  MTLResourceHazardTrackingModeTracked = MTLHazardTrackingModeTracked
                                         << MTLResourceHazardTrackingModeShift,

  // Deprecated spellings
  MTLResourceOptionCPUCacheModeDefault = MTLResourceCPUCacheModeDefaultCache,
  MTLResourceOptionCPUCacheModeWriteCombined = MTLResourceCPUCacheModeWriteCombined,
};

// MTLRenderPipeline.h
enum
{
  MTLColorWriteMaskNone = 0,
  MTLColorWriteMaskRed = 0x1 << 3,
  MTLColorWriteMaskGreen = 0x1 << 2,
  MTLColorWriteMaskBlue = 0x1 << 1,
  MTLColorWriteMaskAlpha = 0x1 << 0,
  MTLColorWriteMaskAll = 0xf
};

// MTLRenderPass.h
enum
{
  MTLStoreActionOptionNone = 0,
  MTLStoreActionOptionCustomSamplePositions = 1 << 0,
};

// MTLRenderPipeline.h
enum MTLBlendFactor : NSUInteger
{
  MTLBlendFactorZero = 0,
  MTLBlendFactorOne = 1,
  MTLBlendFactorSourceColor = 2,
  MTLBlendFactorOneMinusSourceColor = 3,
  MTLBlendFactorSourceAlpha = 4,
  MTLBlendFactorOneMinusSourceAlpha = 5,
  MTLBlendFactorDestinationColor = 6,
  MTLBlendFactorOneMinusDestinationColor = 7,
  MTLBlendFactorDestinationAlpha = 8,
  MTLBlendFactorOneMinusDestinationAlpha = 9,
  MTLBlendFactorSourceAlphaSaturated = 10,
  MTLBlendFactorBlendColor = 11,
  MTLBlendFactorOneMinusBlendColor = 12,
  MTLBlendFactorBlendAlpha = 13,
  MTLBlendFactorOneMinusBlendAlpha = 14,
  MTLBlendFactorSource1Color = 15,
  MTLBlendFactorOneMinusSource1Color = 16,
  MTLBlendFactorSource1Alpha = 17,
  MTLBlendFactorOneMinusSource1Alpha = 18,
};

// MTLRenderPipeline.h
enum MTLBlendOperation : NSUInteger
{
  MTLBlendOperationAdd = 0,
  MTLBlendOperationSubtract = 1,
  MTLBlendOperationReverseSubtract = 2,
  MTLBlendOperationMin = 3,
  MTLBlendOperationMax = 4,
};

// MTLTexture.h
enum MTLTextureType : NSUInteger
{
  MTLTextureType1D = 0,
  MTLTextureType1DArray = 1,
  MTLTextureType2D = 2,
  MTLTextureType2DArray = 3,
  MTLTextureType2DMultisample = 4,
  MTLTextureTypeCube = 5,
  MTLTextureTypeCubeArray = 6,
  MTLTextureType3D = 7,
  MTLTextureType2DMultisampleArray = 8,
  MTLTextureTypeTextureBuffer = 9
};

// MTLTexture.h
enum
{
  MTLTextureUsageUnknown = 0x0000,
  MTLTextureUsageShaderRead = 0x0001,
  MTLTextureUsageShaderWrite = 0x0002,
  MTLTextureUsageRenderTarget = 0x0004,
  MTLTextureUsagePixelFormatView = 0x0010,
};

// MTLTexture.h
enum MTLTextureSwizzle : uint8_t
{
  MTLTextureSwizzleZero = 0,
  MTLTextureSwizzleOne = 1,
  MTLTextureSwizzleRed = 2,
  MTLTextureSwizzleGreen = 3,
  MTLTextureSwizzleBlue = 4,
  MTLTextureSwizzleAlpha = 5,
};

// MTLRenderPass.h
enum MTLLoadAction : NSUInteger
{
  MTLLoadActionDontCare = 0,
  MTLLoadActionLoad = 1,
  MTLLoadActionClear = 2,
};

// MTLRenderPass.h
enum MTLStoreAction : NSUInteger
{
  MTLStoreActionDontCare = 0,
  MTLStoreActionStore = 1,
  MTLStoreActionMultisampleResolve = 2,
  MTLStoreActionStoreAndMultisampleResolve = 3,
  MTLStoreActionUnknown = 4,
  MTLStoreActionCustomSampleDepthStore = 5,
};

// MTLRenderPass.h
inline MTLClearColor MTLClearColorMake(double red, double green, double blue, double alpha)
{
  MTLClearColor result;
  result.red = red;
  result.green = green;
  result.blue = blue;
  result.alpha = alpha;
  return result;
}

DECLARE_REFLECTION_STRUCT(MTLRenderPipelineDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLRenderPipelineColorAttachmentDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLRenderPassDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLRenderPassAttachmentDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLRenderPassColorAttachmentDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLRenderPassDepthAttachmentDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLRenderPassStencilAttachmentDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLTextureDescriptor *);
DECLARE_REFLECTION_STRUCT(MTLClearColor);
DECLARE_REFLECTION_STRUCT(MTLTextureSwizzleChannels);
DECLARE_REFLECTION_STRUCT(MTLSize);

#endif    // #if defined(__OBJC__)

METAL_OPTIONS(DECLARE_METAL_OPTION)
#undef DECLARE_METAL_OPTION

METAL_ENUMS(DECLARE_METAL_ENUM)
#undef DECLARE_METAL_ENUM
