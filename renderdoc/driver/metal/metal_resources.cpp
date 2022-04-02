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

#include "metal_resources.h"
#include "metal_blit_command_encoder.h"
#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_command_queue.h"
#include "metal_device.h"
#include "metal_function.h"
#include "metal_library.h"
#include "metal_render_command_encoder.h"
#include "metal_render_pipeline_state.h"
#include "metal_texture.h"
#include "metal_types.h"

ResourceId GetResID(WrappedMTLObject *obj)
{
  if(obj == NULL)
    return ResourceId();

  return obj->m_ID;
}

#define IMPLEMENT_WRAPPED_TYPE_HELPERS(CPPTYPE)  \
  MTL::CPPTYPE *Unwrap(WrappedMTL##CPPTYPE *obj) \
  {                                              \
    return Unwrap<MTL::CPPTYPE *>(obj);          \
  }
METALCPP_WRAPPED_PROTOCOLS(IMPLEMENT_WRAPPED_TYPE_HELPERS)
#undef IMPLEMENT_WRAPPED_TYPE_HELPERS

MetalResourceManager *WrappedMTLObject::GetResourceManager()
{
  return m_Device->GetResourceManager();
}

MetalResourceRecord::~MetalResourceRecord()
{
  if(m_Type == eResCommandBuffer)
    SAFE_DELETE(cmdInfo);
  else if(m_Type == eResBuffer)
    SAFE_DELETE(bufInfo);
  else if(m_Type == eResTexture)
    SAFE_DELETE(texInfo);
}

void WrappedMTLObject::AddEvent()
{
  m_Device->AddEvent();
}

void WrappedMTLObject::AddAction(const ActionDescription &a)
{
  m_Device->AddAction(a);
}

void MetalTextureState::BeginCapture()
{
  maxRefType = eFrameRef_None;
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalTextureState &el)
{
  SERIALISE_ELEMENT_LOCAL(textureInfo, el.GetTextureInfo());
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalTextureInfo &el)
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

INSTANTIATE_SERIALISE_TYPE(MetalTextureState);
INSTANTIATE_SERIALISE_TYPE(MetalTextureInfo);
