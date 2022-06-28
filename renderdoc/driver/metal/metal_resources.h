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

  void Dealloc();

  MTL::Device *GetDevice() { return (MTL::Device *)m_Device; }
  MetalResourceManager *GetResourceManager();

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

enum MetalCmdBufferStatus
{
  MetalCmdBuffer_Enqueued = 1 << 0,
  MetalCmdBuffer_Committed = 1 << 1,
  MetalCmdBuffer_Submitted = 1 << 2,
  MetalCmdBuffer_Present = 1 << 3,
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
  uint32_t flags = 0;
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
  };
};
