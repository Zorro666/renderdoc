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

#include "metal_manager.h"
#include "metal_device.h"

template <typename SerialiserType>
void MetalResourceManager::SerialiseImageStates(
    SerialiserType &ser, std::map<ResourceId, MetalResources::LockingImageState> &states)
{
  SERIALISE_ELEMENT_LOCAL(NumImages, (uint32_t)states.size()).Important();

  auto srcit = states.begin();

  for(uint32_t i = 0; i < NumImages; i++)
  {
    SERIALISE_ELEMENT_LOCAL(Image, (ResourceId)(srcit->first)).TypedAs("MTLTexture"_lit);
    if(ser.IsWriting())
    {
      MetalResources::LockedImageStateRef lockedState = srcit->second.LockWrite();
      MetalResources::ImageState &ImageState = *lockedState;
      SERIALISE_ELEMENT(ImageState);
      ++srcit;
    }
    else
    {
      bool hasLiveRes = HasLiveResource(Image);

      MetalResources::ImageState imageState;

      {
        MetalResources::ImageState &ImageState = imageState;
        SERIALISE_ELEMENT(ImageState);
      }
      if(IsReplayingAndReading() && hasLiveRes)
      {
        // TODO: handle image states during the capture
      }
      if(hasLiveRes)
      {
        ResourceId liveid = GetLiveID(Image);

        if(IsLoading(m_State))
        {
          // TODO: handle image states during the capture
        }
        else if(IsActiveReplaying(m_State))
        {
          // TODO: handle image states during the capture
        }
      }
    }
  }
}

bool MetalResourceManager::Prepare_InitialState(WrappedMTLObject *res)
{
  return m_WrappedMTLDevice->Prepare_InitialState(res);
}

uint64_t MetalResourceManager::GetSize_InitialState(ResourceId id, const MetalInitialContents &initial)
{
  return m_WrappedMTLDevice->GetSize_InitialState(id, initial);
}

bool MetalResourceManager::Serialise_InitialState(WriteSerialiser &ser, ResourceId id,
                                                  MetalResourceRecord *record,
                                                  const MetalInitialContents *initial)
{
  return m_WrappedMTLDevice->Serialise_InitialState(ser, id, record, initial);
}

void MetalResourceManager::Create_InitialState(ResourceId id, WrappedMTLObject *live, bool hasData)
{
  return m_WrappedMTLDevice->Create_InitialState(id, live, hasData);
}

void MetalResourceManager::Apply_InitialState(WrappedMTLObject *live,
                                              const MetalInitialContents &initial)
{
  return m_WrappedMTLDevice->Apply_InitialState(live, initial);
}
rdcarray<ResourceId> MetalResourceManager::InitialContentResources()
{
  rdcarray<ResourceId> resources =
      ResourceManager<MetalResourceManagerConfiguration>::InitialContentResources();
  std::sort(resources.begin(), resources.end(), [this](ResourceId a, ResourceId b) {
    return m_InitialContents[a].data.type < m_InitialContents[b].data.type;
  });
  return resources;
}

template void MetalResourceManager::SerialiseImageStates(
    WriteSerialiser &ser, std::map<ResourceId, MetalResources::LockingImageState> &states);
template void MetalResourceManager::SerialiseImageStates(
    ReadSerialiser &ser, std::map<ResourceId, MetalResources::LockingImageState> &states);
