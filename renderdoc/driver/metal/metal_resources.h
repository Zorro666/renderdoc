/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Baldur Karlsson
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
#include "metal_types.h"

struct MetalResourceRecord;

enum MetalResourceType
{
  eResUnknown = 0,
  eResBuffer,
  eResCommandBuffer,
  eResCommandQueue,
  eResDevice,
  eResLibrary,
  eResFunction,
  eResRenderCommandEncoder,
  eResRenderPipelineState
};

DECLARE_REFLECTION_ENUM(MetalResourceType);

struct WrappedMTLObject
{
  WrappedMTLObject() : real(NULL), record(NULL), m_WrappedMTLDevice(NULL) {}
  WrappedMTLObject(WrappedMTLDevice *wrappedMTLDevice)
      : real(NULL), record(NULL), m_WrappedMTLDevice(wrappedMTLDevice)
  {
  }
  WrappedMTLObject(id_MTLObject mtlObject, ResourceId objId, WrappedMTLDevice *wrappedMTLDevice)
      : real(mtlObject), id(objId), record(NULL), m_WrappedMTLDevice(wrappedMTLDevice)
  {
  }

  id_MTLDevice GetObjCWrappedMTLDevice();

  MetalResourceManager *GetResourceManager();
  id_MTLObject real;
  ResourceId id;
  MetalResourceRecord *record;
  WrappedMTLDevice *m_WrappedMTLDevice;
};

ResourceId GetResID(WrappedMTLObject *obj);

template <typename WrappedType>
MetalResourceRecord *GetRecord(WrappedType *obj)
{
  if(obj == NULL)
    return NULL;

  return ((WrappedMTLObject *)obj)->record;
}

template <typename RealType>
RealType Unwrap(WrappedMTLObject *obj)
{
  if(obj == NULL)
    return RealType();

  return (RealType)(((WrappedMTLObject *)obj)->real);
}

struct CmdBufferRecordingInfo
{
  CmdBufferRecordingInfo() {}
  CmdBufferRecordingInfo(const CmdBufferRecordingInfo &) = delete;
  CmdBufferRecordingInfo(CmdBufferRecordingInfo &&) = delete;
  CmdBufferRecordingInfo &operator=(const CmdBufferRecordingInfo &) = delete;
  ~CmdBufferRecordingInfo() {}
  WrappedMTLCommandQueue *queue;
  WrappedMTLDevice *device;

  ChunkPagePool *allocPool = NULL;
  ChunkAllocator *alloc = NULL;

  // AdvanceFrame/Present should be called after this buffer is committed.
  bool present;
  // an encoder is active : waiting for endEncoding to be called
  bool isEncoding;
};

struct MetalResourceRecord : public ResourceRecord
{
public:
  enum
  {
    NullResource = NULL
  };

  MetalResourceRecord(ResourceId id)
      : ResourceRecord(id, true), Resource(NULL), resType(eResUnknown), ptrunion(NULL)
  {
  }
  ~MetalResourceRecord() {}
  WrappedMTLObject *Resource;
  MetalResourceType resType;

  // Each entry is only used by specific record types
  union
  {
    void *ptrunion;                     // for initialisation to NULL
    CmdBufferRecordingInfo *cmdInfo;    // only for command buffers
  };
};
