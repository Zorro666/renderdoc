/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2022-2024 Baldur Karlsson
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

#include "core/resource_manager.h"
#include "metal_common.h"
#include "metal_types.h"

struct MetalResourceRecord;
class WrappedMTLDevice;
class MetalResourceManager;

enum MetalResourceType
{
  eResUnknown = 0,
  eResBuffer,
  eResCommandBuffer,
  eResCommandQueue,
  eResDevice,
  eResLibrary,
  eResFunction,
  eResRenderPipelineState,
  eResTexture,
  eResRenderCommandEncoder,
  eResBlitCommandEncoder,
  eResMax
};

DECLARE_REFLECTION_ENUM(MetalResourceType);

struct WrappedMTLObject
{
  WrappedMTLObject() = delete;
  WrappedMTLObject(WrappedMTLDevice *wrappedMTLDevice, CaptureState &captureState)
      : m_Real(NULL), m_Device(wrappedMTLDevice), m_State(captureState)
  {
  }
  WrappedMTLObject(void *mtlObject, ResourceId objId, WrappedMTLDevice *wrappedMTLDevice,
                   CaptureState &captureState)
      : m_Real(mtlObject), m_ID(objId), m_Device(wrappedMTLDevice), m_State(captureState)
  {
  }
  ~WrappedMTLObject() = default;

  MTL::Device *GetDevice() { return (MTL::Device *)m_Device; }
  MetalResourceManager *GetResourceManager();
  void AddEvent();
  void AddAction(const ActionDescription &a);

  void *m_ObjcBridge = NULL;
  void *m_Real;
  ResourceId m_ID;
  MetalResourceRecord *m_Record = NULL;
  WrappedMTLDevice *m_Device;
  CaptureState &m_State;
};

ResourceId GetResID(WrappedMTLObject *obj);

inline ResourceId GetResID(WrappedMTLResource *obj)
{
  return GetResID((WrappedMTLObject *)obj);
}

template <typename WrappedType>
MetalResourceRecord *GetRecord(WrappedType *obj)
{
  if(obj == NULL)
    return NULL;

  return obj->m_Record;
}

template <typename RealType>
RealType Unwrap(WrappedMTLObject *obj)
{
  if(obj == NULL)
    return RealType();

  return (RealType)obj->m_Real;
}

// template magic voodoo to unwrap types
template <typename inner>
struct UnwrapHelper
{
};

#define WRAPPED_TYPE_HELPERS(CPPTYPE)          \
  template <>                                  \
  struct UnwrapHelper<MTL::CPPTYPE *>          \
  {                                            \
    typedef CONCAT(WrappedMTL, CPPTYPE) Outer; \
  };                                           \
  extern MTL::CPPTYPE *Unwrap(WrappedMTL##CPPTYPE *obj);

METALCPP_WRAPPED_PROTOCOLS(WRAPPED_TYPE_HELPERS)
#undef WRAPPED_TYPE_HELPERS

inline MTL::Resource *Unwrap(WrappedMTLResource *obj)
{
  return Unwrap<MTL::Resource *>((WrappedMTLObject *)obj);
}

enum class MetalCmdBufferStatus : uint8_t
{
  Unknown,
  Enqueued,
  Committed,
  Submitted,
};

struct MetalCmdBufferRecordingInfo
{
  MetalCmdBufferRecordingInfo(WrappedMTLCommandQueue *parentQueue) : queue(parentQueue) {}
  MetalCmdBufferRecordingInfo() = delete;
  MetalCmdBufferRecordingInfo(const MetalCmdBufferRecordingInfo &) = delete;
  MetalCmdBufferRecordingInfo(MetalCmdBufferRecordingInfo &&) = delete;
  MetalCmdBufferRecordingInfo &operator=(const MetalCmdBufferRecordingInfo &) = delete;
  ~MetalCmdBufferRecordingInfo() {}
  WrappedMTLCommandQueue *queue;

  // The MetalLayer to present
  CA::MetalLayer *outputLayer = NULL;
  // The texture to present
  WrappedMTLTexture *backBuffer = NULL;
  MetalCmdBufferStatus status = MetalCmdBufferStatus::Unknown;
  bool presented = false;
};

struct MetalBufferInfo
{
  MetalBufferInfo() = delete;
  MetalBufferInfo(MTL::StorageMode mode) : storageMode(mode), data(NULL), length(0) {}
  MTL::StorageMode storageMode;
  bytebuf baseSnapshot;
  byte *data;
  size_t length;
};

struct MetalTextureInfo
{
  MTL::Size extent;
  MTL::TextureUsage usage;
  MTL::TextureType type;
  uint32_t layerCount = 0;
  uint16_t levelCount = 0;
  uint16_t sampleCount = 0;
  bool frameBufferOnly = false;

  MetalTextureInfo() {}
  MetalTextureInfo(RDMTL::TextureDescriptor &descriptor, bool frameBufferOnly)
      : usage(descriptor.usage),
        type(descriptor.textureType),
        layerCount((uint32_t)descriptor.arrayLength),
        levelCount(descriptor.mipmapLevelCount),
        sampleCount(descriptor.sampleCount),
        frameBufferOnly(frameBufferOnly)
  {
    extent.width = descriptor.width;
    extent.height = descriptor.height;
    extent.depth = descriptor.depth;
    MTL::TextureType textureType = descriptor.textureType;
    if(textureType == MTL::TextureType1D)
    {
      extent.height = extent.depth = 1;
    }
    else if(textureType == MTL::TextureType2D)
    {
      extent.depth = 1;
    }
  }

  inline bool operator==(const MetalTextureInfo &other) const
  {
    return layerCount == other.layerCount && levelCount == other.levelCount &&
           sampleCount == other.sampleCount && extent.width == other.extent.width &&
           extent.height == other.extent.height && extent.depth == other.extent.depth &&
           usage == other.usage && type == other.type && frameBufferOnly == other.frameBufferOnly;
  }
};

struct MetalResourceRecord : public ResourceRecord
{
public:
  enum
  {
    NullResource = NULL
  };

  MetalResourceRecord(ResourceId id)
      : ResourceRecord(id, true), m_Resource(NULL), m_Type(eResUnknown), ptrUnion(NULL)
  {
  }
  ~MetalResourceRecord();
  WrappedMTLObject *m_Resource;
  MetalResourceType m_Type;

  // Each entry is only used by specific record types
  union
  {
    void *ptrUnion;                          // for initialisation to NULL
    MetalCmdBufferRecordingInfo *cmdInfo;    // only for command buffers
    MetalBufferInfo *bufInfo;                // only for buffers
    MetalTextureInfo *texInfo;               // only for textures
  };
};

struct MetalTextureState
{
  WrappedMTLTexture *wrappedTexture = NULL;
  MetalTextureInfo textureInfo;
  FrameRefType maxRefType = eFrameRef_None;

  inline const MetalTextureInfo &GetTextureInfo() const { return textureInfo; }
  inline MetalTextureState() {}
  inline MetalTextureState(WrappedMTLTexture *inWrappedTexture,
                           const MetalTextureInfo &inTextureInfo, FrameRefType refType)
      : wrappedTexture(inWrappedTexture), textureInfo(inTextureInfo), maxRefType(refType)
  {
  }
  MetalTextureState InitialState() const;
  void InitialState(MetalTextureState &result) const;
  MetalTextureState CommandBufferInitialState() const;

  void BeginCapture();
  void FixupStorageReferences();
};

DECLARE_REFLECTION_STRUCT(MetalTextureState);
DECLARE_REFLECTION_STRUCT(MetalTextureInfo);
