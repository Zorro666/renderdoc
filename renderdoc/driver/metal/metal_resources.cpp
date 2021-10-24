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

#include "metal_resources.h"
#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_command_queue.h"
#include "metal_device.h"
#include "metal_function.h"
#include "metal_library.h"
#include "metal_manager.h"
#include "metal_render_command_encoder.h"
#include "metal_render_pipeline_state.h"
#include "metal_texture.h"
#include "metal_types.h"

ResourceId GetResID(WrappedMTLObject *obj)
{
  if(obj == NULL)
    return ResourceId();

  return obj->id;
}

template <typename realtype>
realtype GetWrappedObjCResource(MetalResourceManager *rm, ResourceId id)
{
  if(rm && !IsStructuredExporting(rm->GetState()))
  {
    if(id != ResourceId())
    {
      if(rm->HasLiveResource(id))
      {
        using WrappedType = typename MetalResourceManager::UnwrapHelper<realtype>::Outer;
        WrappedType *wrapped = (WrappedType *)rm->GetLiveResource(id);
        return UnwrapObjC<realtype>(wrapped);
      }
    }
  }
  return realtype();
}

void WrappedMTLObject::Dealloc()
{
  GetResourceManager()->ReleaseWrappedResource(this);
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

MTL::Device *WrappedMTLObject::GetObjCWrappedMTLDevice()
{
  return UnwrapObjC<MTL::Device *>(m_WrappedMTLDevice);
}

#define INSTANTIATE_GET_OBJC_FROM_WRAPPED(CPPTYPE) \
  template MTL::CPPTYPE *GetWrappedObjCResource(MetalResourceManager *rm, ResourceId id);

METALCPP_WRAPPED_PROTOCOLS(INSTANTIATE_GET_OBJC_FROM_WRAPPED)
#undef INSTANTIATE_GET_OBJC_FROM_WRAPPED

void MetalResources::ImageState::BeginCapture()
{
  maxRefType = eFrameRef_None;
}

MetalResourceRecord::~MetalResourceRecord()
{
  if(resType == eResCommandBuffer)
    SAFE_DELETE(cmdInfo);
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
    MTL::TextureType type = el.type;
    MTL::TextureUsage usage = el.usage;
    SERIALISE_ELEMENT(usage);
    SERIALISE_ELEMENT(type);
    if(ser.IsReading())
    {
      el.usage = usage;
      el.type = type;
    }
  }
}

INSTANTIATE_SERIALISE_TYPE(MetalResources::ImageState);
INSTANTIATE_SERIALISE_TYPE(MetalResources::ImageInfo);
