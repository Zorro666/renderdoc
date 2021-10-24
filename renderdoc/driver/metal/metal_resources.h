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

struct MetalResourceRecord;
class WrappedMTLDevice;
class WrappedMTLCommandQueue;
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
  eResRenderCommandEncoder,
  eResRenderPipelineState,
  eResTexture
};

DECLARE_REFLECTION_ENUM(MetalResourceType);

struct WrappedMTLObject
{
  WrappedMTLObject() = delete;
  WrappedMTLObject(WrappedMTLDevice *wrappedMTLDevice, CaptureState &captureState)
      : wrappedObjC(NULL),
        real(NULL),
        record(NULL),
        m_WrappedMTLDevice(wrappedMTLDevice),
        m_State(captureState)
  {
  }
  WrappedMTLObject(void *mtlObject, ResourceId objId, WrappedMTLDevice *wrappedMTLDevice,
                   CaptureState &captureState)
      : wrappedObjC(NULL),
        real(mtlObject),
        id(objId),
        record(NULL),
        m_WrappedMTLDevice(wrappedMTLDevice),
        m_State(captureState)
  {
  }
  ~WrappedMTLObject() = default;

  void Dealloc();

  MTL::Device *GetObjCWrappedMTLDevice();

  MetalResourceManager *GetResourceManager();
  void AddEvent();
  void AddAction(const ActionDescription &a);

  void *wrappedObjC;
  void *real;
  ResourceId id;
  MetalResourceRecord *record;
  WrappedMTLDevice *m_WrappedMTLDevice;
  CaptureState &m_State;
};

ResourceId GetResID(WrappedMTLObject *obj);

template <typename WrappedType>
MetalResourceRecord *GetRecord(WrappedType *obj)
{
  if(obj == NULL)
    return NULL;

  return obj->record;
}

template <typename RealType>
RealType Unwrap(WrappedMTLObject *obj)
{
  if(obj == NULL)
    return RealType();

  return (RealType)obj->real;
}

template <typename RealType>
RealType UnwrapObjC(WrappedMTLObject *obj)
{
  if(obj == NULL)
    return RealType();

  return (RealType)obj->wrappedObjC;
}

template <typename RealType>
RealType GetWrappedObjCResource(MetalResourceManager *rm, ResourceId id);

namespace MetalResources
{
struct CmdBufferRecordingInfo
{
  CmdBufferRecordingInfo() {}
  CmdBufferRecordingInfo(const CmdBufferRecordingInfo &) = delete;
  CmdBufferRecordingInfo(CmdBufferRecordingInfo &&) = delete;
  CmdBufferRecordingInfo &operator=(const CmdBufferRecordingInfo &) = delete;
  ~CmdBufferRecordingInfo()
  {
    SAFE_DELETE(alloc);
    SAFE_DELETE(allocPool);
  }

  WrappedMTLCommandQueue *queue;
  WrappedMTLDevice *device;

  ChunkPagePool *allocPool = NULL;
  ChunkAllocator *alloc = NULL;

  // The drawable that present was called on
  MTL::Drawable *drawable;
  // AdvanceFrame/Present should be called after this buffer is committed.
  bool present;
  // an encoder is active : waiting for endEncoding to be called
  bool isEncoding;
};

struct ImageInfo
{
  MTL::Size extent;
  MTL::TextureUsage usage;
  MTL::TextureType type;
  uint32_t layerCount = 0;
  uint16_t levelCount = 0;
  uint16_t sampleCount = 0;
  bool storage = false;

  ImageInfo() {}
  ImageInfo(MTL::TextureType type, MTL::Size extent, uint16_t levelCount, uint32_t layerCount,
            uint16_t sampleCount)
      : type(type),
        extent(extent),
        levelCount(levelCount),
        layerCount(layerCount),
        sampleCount(sampleCount)
  {
  }
  //  ImageInfo(const VkImageCreateInfo &ci)
  //      : layerCount(ci.arrayLayers),
  //        levelCount((uint16_t)ci.mipLevels),
  //        sampleCount((uint16_t)ci.samples),
  //        extent(ci.extent),
  //        imageType(ci.imageType),
  //        format(ci.format),
  //        initialLayout(ci.initialLayout),
  //        sharingMode(ci.sharingMode)
  //  {
  //    if(ci.imageType == VK_IMAGE_TYPE_1D)
  //    {
  //      extent.height = extent.depth = 1;
  //    }
  //    else if(ci.imageType == VK_IMAGE_TYPE_2D)
  //    {
  //      extent.depth = 1;
  //    }
  //    aspects = FormatImageAspects(format);
  //
  //    if(ci.usage & VK_IMAGE_USAGE_STORAGE_BIT)
  //    {
  //      storage = true;
  //    }
  //  }
  //  ImageInfo(const VkSwapchainCreateInfoKHR &ci)
  //      : layerCount(ci.imageArrayLayers),
  //        levelCount(1),
  //        sampleCount(1),
  //        format(ci.imageFormat),
  //        sharingMode(ci.imageSharingMode)
  //  {
  //    extent.width = ci.imageExtent.width;
  //    extent.height = ci.imageExtent.height;
  //    extent.depth = 1;
  //    aspects = FormatImageAspects(format);
  //  }
  inline bool operator==(const ImageInfo &other) const
  {
    return layerCount == other.layerCount && levelCount == other.levelCount &&
           sampleCount == other.sampleCount && extent.width == other.extent.width &&
           extent.height == other.extent.height && extent.depth == other.extent.depth &&
           usage == other.usage && type == other.type;
  }
};

struct ImageState
{
  ImageInfo imageInfo;
  bool isMemoryBound = false;
  bool m_Overlay = false;
  bool m_Storage = false;
  ResourceId boundMemory = ResourceId();
  FrameRefType maxRefType = eFrameRef_None;
  MTL::Texture *wrappedHandle = NULL;

  inline const ImageInfo &GetImageInfo() const { return imageInfo; }
  inline ImageState() {}
  inline ImageState(MTL::Texture *wrappedHandle, const ImageInfo &imageInfo, FrameRefType refType)
      : wrappedHandle(wrappedHandle),
        imageInfo(imageInfo),
        maxRefType(refType),
        m_Storage(imageInfo.storage)
  {
  }
  void SetOverlay() { m_Overlay = true; }
  ImageState InitialState() const;
  void InitialState(ImageState &result) const;
  ImageState CommandBufferInitialState() const;

  void BeginCapture();
  void FixupStorageReferences();
};

template <typename ImageStateT>
class LockedImageStateRefTemplate
{
public:
  LockedImageStateRefTemplate() = default;
  LockedImageStateRefTemplate(ImageStateT *state, Threading::SpinLock &spin)
      : m_state(state), m_lock(spin)
  {
  }
  inline ImageStateT &operator*() const { return *m_state; }
  inline ImageStateT *operator->() const { return m_state; }
  inline operator bool() const { return m_state != NULL; }
private:
  ImageStateT *m_state = NULL;
  Threading::ScopedSpinLock m_lock;
};

class LockedConstImageStateRef : public LockedImageStateRefTemplate<const ImageState>
{
public:
  LockedConstImageStateRef() = default;
  LockedConstImageStateRef(const ImageState *state, Threading::SpinLock &spin)
      : LockedImageStateRefTemplate<const ImageState>(state, spin)
  {
  }
};

class LockedImageStateRef : public LockedImageStateRefTemplate<ImageState>
{
public:
  LockedImageStateRef() = default;
  LockedImageStateRef(ImageState *state, Threading::SpinLock &spin)
      : LockedImageStateRefTemplate<ImageState>(state, spin)
  {
  }
};

class LockingImageState
{
public:
  LockingImageState() = default;
  LockingImageState(MTL::Texture *wrappedHandle, const ImageInfo &imageInfo, FrameRefType refType)
      : m_state(wrappedHandle, imageInfo, refType)
  {
  }
  LockingImageState(const ImageState &state) : m_state(state) {}
  LockedImageStateRef LockWrite() { return LockedImageStateRef(&m_state, m_lock); }
  LockedConstImageStateRef LockRead() { return LockedConstImageStateRef(&m_state, m_lock); }
  inline ImageState *state() { return &m_state; }
private:
  ImageState m_state;
  Threading::SpinLock m_lock;
};

struct TaggedImageState
{
  ResourceId id;
  ImageState state;
};

struct ImgRefs
{
  rdcarray<FrameRefType> rangeRefs;
  WrappedMTLObject *initializedLiveRes = NULL;
  ImageInfo imageInfo;

  bool areAspectsSplit = false;
  bool areLevelsSplit = false;
  bool areLayersSplit = false;

  ImgRefs() : initializedLiveRes(NULL) {}
  inline ImgRefs(const ImageInfo &imageInfo) : imageInfo(imageInfo)
  {
    rangeRefs.fill(1, eFrameRef_None);
    if(imageInfo.extent.depth > 1)
      // Depth slices of 3D views are treated as array layers
      this->imageInfo.layerCount = (uint32_t)imageInfo.extent.depth;
  }
};

struct ImgRefsPair
{
  ResourceId image;
  ImgRefs imgRefs;
};

// these structs are allocated for images and buffers, then pointed to (non-owning) by views
struct ResourceInfo
{
  // commonly we expect only one aspect (COLOR is vastly likely and METADATA is rare) so have one
  // directly accessible. If we have others (like separate DEPTH and STENCIL, or anything and
  // METADATA) we put them in the array.
  ImageInfo imageInfo;
};
};

struct MetalResourceRecord : public ResourceRecord
{
public:
  enum
  {
    NullResource = NULL
  };

  MetalResourceRecord(ResourceId id)
      : ResourceRecord(id, true), Resource(NULL), resType(eResUnknown), ptrUnion(NULL)
  {
  }
  ~MetalResourceRecord();
  WrappedMTLObject *Resource;
  MetalResourceType resType;

  // Each entry is only used by specific record types
  union
  {
    void *ptrUnion;                                     // for initialisation to NULL
    MetalResources::CmdBufferRecordingInfo *cmdInfo;    // only for command buffers
    MetalResources::ResourceInfo *resInfo;              // only for images or buffers
  };
};

template <class SerialiserType>
void DoSerialise(SerialiserType &ser, MetalResources::ImgRefsPair &el)
{
  SERIALISE_MEMBER(image);
  SERIALISE_MEMBER(imgRefs);
}

DECLARE_REFLECTION_STRUCT(MetalResources::TaggedImageState);
DECLARE_REFLECTION_STRUCT(MetalResources::ImageState);
DECLARE_REFLECTION_STRUCT(MetalResources::ImageInfo);
DECLARE_REFLECTION_STRUCT(MetalResources::ImgRefs);
DECLARE_REFLECTION_STRUCT(MetalResources::ImgRefsPair);
