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

#include "metal_resources.h"
#include "metal_manager.h"
#include "metal_metal.h"

ResourceId GetResID(WrappedMTLObject *obj)
{
  if(obj == NULL)
    return ResourceId();

  return ((WrappedMTLObject *)obj)->id;
}

template <typename realtype>
realtype GetObjCWrappedResource(MetalResourceManager *rm, ResourceId id)
{
  if(rm && !IsStructuredExporting(rm->GetState()))
  {
    if(id != ResourceId())
    {
      if(rm->HasLiveResource(id))
      {
        using WrappedType = typename MetalResourceManager::UnwrapHelper<realtype>::Outer;
        WrappedType *wrapped = (WrappedType *)rm->GetLiveResource(id);
        return wrapped->objc;
      }
    }
  }
  return realtype();
}

MetalResourceManager *WrappedMTLObject::GetResourceManager()
{
  return m_WrappedMTLDevice->GetResourceManager();
}

void WrappedMTLObject::AddEvent()
{
  m_WrappedMTLDevice->AddEvent();
}

void WrappedMTLObject::AddAction(const ActionDescription &a)
{
  m_WrappedMTLDevice->AddAction(a);
}

id_MTLDevice WrappedMTLObject::GetObjCWrappedMTLDevice()
{
  return m_WrappedMTLDevice->objc;
}

#define INSTANTIATE_GET_OBJC_FROM_WRAPPED(TYPE) \
  template id_##TYPE GetObjCWrappedResource(MetalResourceManager *rm, ResourceId id);

METAL_WRAPPED_PROTOCOLS(INSTANTIATE_GET_OBJC_FROM_WRAPPED)
#undef INSTANTIATE_GET_OBJC_FROM_WRAPPED

void MetalResources::ImageState::BeginCapture()
{
  maxRefType = eFrameRef_None;
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalResources::ImageState &el)
{
  SERIALISE_ELEMENT_LOCAL(imageInfo, el.GetImageInfo());
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalResources::ImageInfo &el)
{
  SERIALISE_MEMBER(layerCount);
  // serialise these as full 32-bit integers for backwards compatibility
  {
    uint32_t levelCount = el.levelCount;
    uint32_t sampleCount = el.sampleCount;
    SERIALISE_ELEMENT(levelCount);
    SERIALISE_ELEMENT(sampleCount);
    if(ser.IsReading())
    {
      el.levelCount = (uint16_t)levelCount;
      el.sampleCount = (uint16_t)sampleCount;
    }
  }

  SERIALISE_MEMBER(extent);
  {
    MTLTextureType_objc type = (MTLTextureType_objc)el.type;
    MTLTextureUsage_objc usage = (MTLTextureUsage_objc)el.usage;
    SERIALISE_ELEMENT(usage);
    SERIALISE_ELEMENT(type);
    if(ser.IsReading())
    {
      el.usage = (MTLTextureUsage)usage;
      el.type = (MTLTextureType)type;
    }
  }
}

INSTANTIATE_SERIALISE_TYPE(MetalResources::ImageState);
INSTANTIATE_SERIALISE_TYPE(MetalResources::ImageInfo);
