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
#include "metal_resources.h"
#include "metal_types.h"

struct MetalInitialContents
{
  MetalInitialContents()
  {
    RDCCOMPILE_ASSERT(std::is_standard_layout<MetalInitialContents>::value,
                      "MetalInitialContents must be POD");
    memset(this, 0, sizeof(*this));
  }

  MetalInitialContents(MetalResourceType t)
  {
    memset(this, 0, sizeof(*this));
    type = t;
  }

  template <typename Configuration>
  void Free(ResourceManager<Configuration> *rm)
  {
    RDCASSERT(false);
  }
  bytebuf resourceContents;

  // for plain resources, we store the resource type
  MetalResourceType type;
  // MTL::Buffer* buf;
  // MTL::Texture* img;
};

struct MetalResourceManagerConfiguration
{
  typedef WrappedMTLObject *WrappedResourceType;
  typedef void *RealResourceType;
  typedef MetalResourceRecord RecordType;
  typedef MetalInitialContents InitialContentData;
};

class MetalResourceManager : public ResourceManager<MetalResourceManagerConfiguration>
{
public:
  MetalResourceManager(CaptureState &state, WrappedMTLDevice *device)
      : ResourceManager(state), m_WrappedMTLDevice(device)
  {
  }
  void SetState(CaptureState state) { m_State = state; }
  CaptureState GetState() { return m_State; }
  ~MetalResourceManager() {}
  void ClearWithoutReleasing()
  {
    // if any objects leaked past, it's no longer safe to delete them as we would
    // be calling Shutdown() after the device that owns them is destroyed. Instead
    // we just have to leak ourselves.
    RDCASSERT(m_LiveResourceMap.empty());
    RDCASSERT(m_InitialContents.empty());
    RDCASSERT(m_ResourceRecords.empty());
    RDCASSERT(m_CurrentResourceMap.empty());
    RDCASSERT(m_WrapperMap.empty());

    m_LiveResourceMap.clear();
    m_InitialContents.clear();
    m_ResourceRecords.clear();
    m_CurrentResourceMap.clear();
    m_WrapperMap.clear();
  }

  ResourceId GetID(WrappedMTLObject *res)
  {
    if(res == NULL)
      return ResourceId();

    return res->id;
  }

  // template magic voodoo to unwrap types
  template <typename inner>
  struct UnwrapHelper
  {
  };

#define UNWRAP_HELPER(CPPTYPE)                 \
  template <>                                  \
  struct UnwrapHelper<MTL::CPPTYPE *>          \
  {                                            \
    typedef CONCAT(WrappedMTL, CPPTYPE) Outer; \
  };

  METALCPP_WRAPPED_PROTOCOLS(UNWRAP_HELPER)
#undef UNWRAP_HELPER

  template <typename realtype>
  ResourceId WrapResource(realtype obj, typename UnwrapHelper<realtype>::Outer *&wrapped)
  {
    RDCASSERT(obj != NULL);
    RDCASSERT(m_WrappedMTLDevice != NULL);

    ResourceId id = ResourceIDGen::GetNewUniqueID();
    using WrappedType = typename UnwrapHelper<realtype>::Outer;
    wrapped = new WrappedType(obj, id, m_WrappedMTLDevice);
    wrapped->real = obj;
    AddCurrentResource(id, wrapped);

    // TODO: implement RD MTL replay
    //    if(IsReplayMode(m_State))
    //     AddWrapper(wrapMetalResourceManager(obj));
    return id;
  }

  template <typename realtype>
  void ReleaseWrappedResource(realtype obj, bool clearID = false)
  {
    ResourceId id = GetResID(obj);

    // TODO: implement RD MTL replay
    //    if(IsReplayMode(m_State))
    //      ResourceManager::RemoveWrapper(ToTypedHandle(Unwrap(obj)));

    ResourceManager::ReleaseCurrentResource(id);
    MetalResourceRecord *record = GetRecord(obj);
    if(record)
    {
      record->Delete(this);
    }
    /*
        if(clearID)
        {
          // note the nulling of the wrapped object's ID here is rather unpleasant,
          // but the lesser of two evils to ensure that stale descriptor set slots
          // referencing the object behave safely. To do this correctly we would need
          // to maintain a list of back-references to every descriptor set that has
          // this object bound, and invalidate them. Instead we just make sure the ID
          // is always something sensible, since we know the deallocation doesn't
          // free the memory - the object is pool-allocated.
          // If a new object is allocated in that pool slot, it will still be a valid
          // ID and if the resource isn't ever referenced elsewhere, it will just be
          // a non-live ID to be ignored.
          WrappedMTLObject *res = (WrappedMTLObject *)GetWrapped(obj);
          res->id = ResourceId();
          res->record = NULL;
        }
    */
    delete obj;
  }

  using ResourceManager::AddResourceRecord;

  template <typename wrappedtype>
  MetalResourceRecord *AddResourceRecord(wrappedtype *wrapped)
  {
    MetalResourceRecord *ret = wrapped->record = ResourceManager::AddResourceRecord(wrapped->id);

    ret->Resource = (WrappedMTLObject *)wrapped;
    ret->resType = (MetalResourceType)wrappedtype::TypeEnum;
    return ret;
  }

  void SetInternalResource(ResourceId id) {}
  InitPolicy GetInitPolicy() { return m_InitPolicy; }
  void SetOptimisationLevel(ReplayOptimisationLevel level)
  {
    switch(level)
    {
      case ReplayOptimisationLevel::Count:
        RDCERR("Invalid optimisation level specified");
        m_InitPolicy = eInitPolicy_NoOpt;
        break;
      case ReplayOptimisationLevel::NoOptimisation: m_InitPolicy = eInitPolicy_NoOpt; break;
      case ReplayOptimisationLevel::Conservative: m_InitPolicy = eInitPolicy_CopyAll; break;
      case ReplayOptimisationLevel::Balanced: m_InitPolicy = eInitPolicy_ClearUnread; break;
      case ReplayOptimisationLevel::Fastest: m_InitPolicy = eInitPolicy_Fastest; break;
    }
  }

  template <typename SerialiserType>
  void SerialiseImageStates(SerialiserType &ser,
                            std::map<ResourceId, MetalResources::LockingImageState> &states);

private:
  bool ResourceTypeRelease(WrappedMTLObject *res) { return false; }
  bool Prepare_InitialState(WrappedMTLObject *res);
  uint64_t GetSize_InitialState(ResourceId id, const MetalInitialContents &initial);
  bool Serialise_InitialState(WriteSerialiser &ser, ResourceId id, MetalResourceRecord *record,
                              const MetalInitialContents *initial);
  void Create_InitialState(ResourceId id, WrappedMTLObject *live, bool hasData);
  void Apply_InitialState(WrappedMTLObject *live, const MetalInitialContents &initial);
  rdcarray<ResourceId> InitialContentResources();

  WrappedMTLDevice *m_WrappedMTLDevice;
  InitPolicy m_InitPolicy = eInitPolicy_CopyAll;
};
