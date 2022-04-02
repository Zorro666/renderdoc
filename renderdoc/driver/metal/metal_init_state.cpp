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

#include "metal_buffer.h"
#include "metal_common.h"
#include "metal_device.h"
#include "metal_texture.h"

static rdcliteral NameOfType(MetalResourceType type)
{
  switch(type)
  {
    case eResBuffer: return "MTLBuffer"_lit;
    case eResTexture: return "MTLTexture"_lit;
    default: break;
  }
  return "MTLResource"_lit;
}

bool WrappedMTLDevice::Prepare_InitialState(WrappedMTLObject *res)
{
  ResourceId id = GetResourceManager()->GetID(res);

  MetalResourceType type = res->m_Record->m_Type;

  if(type == eResBuffer)
  {
    WrappedMTLBuffer *buffer = (WrappedMTLBuffer *)res;
    MTL::Buffer *mtlBuffer = Unwrap(buffer);
    MTL::Buffer *mtlSharedBuffer = NULL;
    MTL::StorageMode storageMode = mtlBuffer->storageMode();
    size_t len = mtlBuffer->length();
    byte *data = NULL;
    if(storageMode == MTL::StorageModeShared)
    {
      // MTLStorageModeShared buffers are automatically synchronized
      data = (byte *)mtlBuffer->contents();
    }
    else if(storageMode == MTL::StorageModeManaged)
    {
      // MTLStorageModeManaged buffers need to call MTLBlitCommandEncoder::synchronizeResource
      MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
      MTL::BlitCommandEncoder *mtlBlitEncoder = mtlCommandBuffer->blitCommandEncoder();
      mtlBlitEncoder->synchronizeResource(mtlBuffer);
      mtlBlitEncoder->endEncoding();
      mtlCommandBuffer->commit();
      mtlCommandBuffer->waitUntilCompleted();
      data = (byte *)mtlBuffer->contents();
    }
    else if(storageMode == MTL::StorageModePrivate)
    {
      // TODO: postpone readback until data is required
      // TODO: batch readback for multiple resources to avoid sync per resource
      // MTLStorageModePrivate buffer need to copy into a temporary MTLStorageModeShared buffer
      mtlSharedBuffer = Unwrap(this)->newBuffer(len, MTL::ResourceStorageModeShared);
      MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
      MTL::BlitCommandEncoder *mtlBlitEncoder = mtlCommandBuffer->blitCommandEncoder();
      mtlBlitEncoder->copyFromBuffer(mtlBuffer, 0, mtlSharedBuffer, 0, len);
      mtlBlitEncoder->endEncoding();
      mtlCommandBuffer->commit();
      mtlCommandBuffer->waitUntilCompleted();
      data = (byte *)mtlSharedBuffer->contents();
    }
    else
    {
      RDCERR("Unhandled buffer storage mode 0x%X", storageMode);
    }

    bytebuf bufferContents(data, len);
    MetalInitialContents initialContents(type, bufferContents);
    GetResourceManager()->SetInitialContents(id, initialContents);
    if(mtlSharedBuffer)
    {
      mtlSharedBuffer->release();
    }
    if(storageMode == MTL::StorageModeShared)
    {
      // Set the base snapshot to match the initial contents
      MetalBufferInfo *bufInfo = res->m_Record->bufInfo;
      if(bufInfo->baseSnapshot.isEmpty())
        bufInfo->baseSnapshot.resize(len);
      RDCASSERTEQUAL(bufInfo->baseSnapshot.size(), len);
      memcpy(bufInfo->baseSnapshot.data(), bufferContents.data(), len);
    }
    return true;
  }
  else if(type == eResTexture)
  {
    WrappedMTLTexture *texture = (WrappedMTLTexture *)res;
    const MetalTextureInfo &textureInfo = *texture->m_Record->texInfo;

    MetalTextureState *state = FindTextureState(texture->m_ID);

    if(!state)
      return true;

    // TODO: Implement fetching texture contents
    /*
        VkDevice d = GetDev();
        VkCommandBuffer cmd = GetNextCmd();

        // must ensure offset remains valid. Must be multiple of block size, or 4, depending on
    format
        VkDeviceSize bufAlignment = 4;
        if(IsBlockFormat(textureInfo.format))
          bufAlignment = (VkDeviceSize)GetByteSize(1, 1, 1, textureInfo.format, 0);

        VkBufferCreateInfo bufInfo = {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            NULL,
            0,
            0,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        VkImage arrayIm = VK_NULL_HANDLE;

        VkImage realim = im->real.As<VkImage>();
        int numLayers = textureInfo.layerCount;

        uint32_t planeCount = GetYUVPlaneCount(textureInfo.format);
        uint32_t horizontalPlaneShift = 0;
        uint32_t verticalPlaneShift = 0;

        if(planeCount > 1)
        {
          switch(textureInfo.format)
          {
            case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
            case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
            case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
              horizontalPlaneShift = verticalPlaneShift = 1;
              break;
            case VK_FORMAT_G8B8G8R8_422_UNORM:
            case VK_FORMAT_B8G8R8G8_422_UNORM:
            case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
            case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
            case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G16B16G16R16_422_UNORM:
            case VK_FORMAT_B16G16R16G16_422_UNORM:
            case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
            case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: horizontalPlaneShift = 1; break;
            default: break;
          }
        }

        VkFormat sizeFormat = GetDepthOnlyFormat(textureInfo.format);

        for(int a = 0; a < numLayers; a++)
        {
          for(int m = 0; m < textureInfo.levelCount; m++)
          {
            bufInfo.size = AlignUp(bufInfo.size, bufAlignment);

            if(planeCount > 1)
            {
              // need to consider each plane aspect separately. We simplify the calculation by just
              // aligning up the width to a multiple of 4, that ensures each plane will start at a
              // multiple of 4 because the rowpitch must be a multiple of 4
              bufInfo.size += GetByteSize(AlignUp4(textureInfo.extent.width),
    textureInfo.extent.height,
                                          textureInfo.extent.depth, sizeFormat, m);
            }
            else
            {
              bufInfo.size += GetByteSize(textureInfo.extent.width, textureInfo.extent.height,
                                          textureInfo.extent.depth, sizeFormat, m);

              if(sizeFormat != textureInfo.format)
              {
                // if there's stencil and depth, allocate space for stencil
                bufInfo.size = AlignUp(bufInfo.size, bufAlignment);

                bufInfo.size += GetByteSize(textureInfo.extent.width, textureInfo.extent.height,
                                            textureInfo.extent.depth, VK_FORMAT_S8_UINT, m);
              }
            }
          }
        }

        // since this happens during capture, we don't want to start serialising extra buffer
    creates,
        // so we manually create & then just wrap.
        VkBuffer dstBuf;

        vkr = ObjDisp(d)->CreateBuffer(Unwrap(d), &bufInfo, NULL, &dstBuf);
        CheckVkResult(vkr);

        GetResourceManager()->WrapResource(Unwrap(d), dstBuf);

        MemoryAllocation readbackmem =
            AllocateMemoryForResource(dstBuf, MemoryScope::InitialContents, MemoryType::Readback);

        if(readbackmem.mem == VK_NULL_HANDLE)
          return false;

        vkr = ObjDisp(d)->BindBufferMemory(Unwrap(d), Unwrap(dstBuf), Unwrap(readbackmem.mem),
                                           readbackmem.offs);
        CheckVkResult(vkr);

        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
                                              VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        vkr = ObjDisp(d)->BeginCommandBuffer(Unwrap(cmd), &beginInfo);
        CheckVkResult(vkr);

        VkImageAspectFlags aspectFlags = FormatImageAspects(textureInfo.format);

        ImageBarrierSequence setupBarriers, cleanupBarriers;

        VkImageLayout readingLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        state->TempTransition(m_QueueFamilyIdx, readingLayout,
                              VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
    setupBarriers,
                              cleanupBarriers, GetImageTransitionInfo());
        InlineSetupImageBarriers(cmd, setupBarriers);
        m_setupImageBarriers.Merge(setupBarriers);

        VkDeviceSize bufOffset = 0;

        // loop over every slice/mip, copying it to the appropriate point in the buffer
        for(int a = 0; a < numLayers; a++)
        {
          VkExtent3D extent = textureInfo.extent;

          for(int m = 0; m < textureInfo.levelCount; m++)
          {
            VkBufferImageCopy region = {
                0,
                0,
                0,
                {aspectFlags, (uint32_t)m, (uint32_t)a, 1},
                {
                    0, 0, 0,
                },
                extent,
            };

            if(planeCount > 1)
            {
              // need to consider each plane aspect separately
              for(uint32_t i = 0; i < planeCount; i++)
              {
                bufOffset = AlignUp(bufOffset, bufAlignment);

                region.imageExtent = extent;
                region.bufferOffset = bufOffset;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT << i;

                if(i > 0)
                {
                  region.imageExtent.width >>= horizontalPlaneShift;
                  region.imageExtent.height >>= verticalPlaneShift;
                }

                bufOffset += GetPlaneByteSize(textureInfo.extent.width, textureInfo.extent.height,
                                              textureInfo.extent.depth, sizeFormat, m, i);

                ObjDisp(d)->CmdCopyImageToBuffer(Unwrap(cmd), realim,
                                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    Unwrap(dstBuf),
                                                 1, &region);
              }
            }
            else
            {
              bufOffset = AlignUp(bufOffset, bufAlignment);

              region.bufferOffset = bufOffset;

              // for depth/stencil copies, copy depth first
              if(aspectFlags == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

              bufOffset += GetByteSize(textureInfo.extent.width, textureInfo.extent.height,
                                       textureInfo.extent.depth, sizeFormat, m);

              ObjDisp(d)->CmdCopyImageToBuffer(
                  Unwrap(cmd), realim, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Unwrap(dstBuf), 1,
    &region);

              if(aspectFlags == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
              {
                // if we have combined stencil to process, copy that separately now.
                bufOffset = AlignUp(bufOffset, bufAlignment);

                region.bufferOffset = bufOffset;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;

                bufOffset += GetByteSize(textureInfo.extent.width, textureInfo.extent.height,
                                         textureInfo.extent.depth, VK_FORMAT_S8_UINT, m);

                ObjDisp(d)->CmdCopyImageToBuffer(Unwrap(cmd), realim,
                                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    Unwrap(dstBuf),
                                                 1, &region);
              }
            }

            // update the extent for the next mip
            extent.width = RDCMAX(extent.width >> 1, 1U);
            extent.height = RDCMAX(extent.height >> 1, 1U);
            extent.depth = RDCMAX(extent.depth >> 1, 1U);
          }
        }

        RDCASSERTMSG("buffer wasn't sized sufficiently!", bufOffset <= bufInfo.size, bufOffset,
                     readbackmem.size, textureInfo.extent, textureInfo.format, numLayers,
                     textureInfo.levelCount);
        InlineCleanupImageBarriers(cmd, cleanupBarriers);
        m_cleanupImageBarriers.Merge(cleanupBarriers);

        vkr = ObjDisp(d)->EndCommandBuffer(Unwrap(cmd));
        CheckVkResult(vkr);

    //    SubmitAndFlushImageStateBarriers(m_setupImageBarriers);
        SubmitCmds();
        FlushQ();
    //    SubmitAndFlushImageStateBarriers(m_cleanupImageBarriers);

        ObjDisp(d)->DestroyBuffer(Unwrap(d), Unwrap(dstBuf), NULL);
        GetResourceManager()->ReleaseWrappedResource(dstBuf);

        MetalInitialContents initialContents(type, readbackmem);

        GetResourceManager()->SetInitialContents(id, initialContents);
     */

    return true;
  }
  else
  {
    RDCERR("Unhandled resource type %d", type);
  }

  return false;
}

uint64_t WrappedMTLDevice::GetSize_InitialState(ResourceId id, const MetalInitialContents &initial)
{
  uint64_t ret = 128;

  if(initial.type == eResBuffer)
  {
    ret += uint64_t(initial.resourceContents.size() + WriteSerialiser::GetChunkAlignment());
    return ret;
  }

  RDCERR("Unhandled resource type %s", ToStr(initial.type).c_str());
  return 0;
}

template <typename SerialiserType>
bool WrappedMTLDevice::Serialise_InitialState(SerialiserType &ser, ResourceId id,
                                              MetalResourceRecord *record,
                                              const MetalInitialContents *initial)
{
  SERIALISE_ELEMENT_LOCAL(type, initial->type);
  SERIALISE_ELEMENT(id).TypedAs(NameOfType(type)).Important();

  if(IsReplayingAndReading())
  {
    AddResourceCurChunk(id);
  }

  if(type == eResBuffer)
  {
    SERIALISE_CHECK_READ_ERRORS();

    bytebuf contents;
    if(ser.IsWriting())
    {
      ser.Serialise("Contents"_lit, initial->resourceContents);
    }
    else
    {
      ser.Serialise("Contents"_lit, contents);
    }

    if(IsReplayingAndReading())
    {
      MetalInitialContents initialContents(eResBuffer, contents);
      GetResourceManager()->SetInitialContents(id, initialContents);
    }
    return true;
  }
  RDCERR("Unhandled resource type %d", type);
  return false;
}

void WrappedMTLDevice::Create_InitialState(ResourceId id, WrappedMTLObject *live, bool hasData)
{
  if(IsStructuredExporting(m_State))
  {
    return;
  }

  MetalResourceType type = live->m_Record->m_Type;
  if(type == eResBuffer)
  {
    return;
  }
  RDCERR("Unhandled resource type %d", type);
}

void WrappedMTLDevice::Apply_InitialState(WrappedMTLObject *live, const MetalInitialContents &initial)
{
  MetalResourceType type = initial.type;
  if(type == eResBuffer)
  {
    WrappedMTLBuffer *buffer = (WrappedMTLBuffer *)live;
    MTL::Buffer *mtlBuffer = Unwrap(buffer);
    MTL::StorageMode storageMode = mtlBuffer->storageMode();
    size_t len = mtlBuffer->length();
    byte *data = NULL;
    RDCASSERT(len == initial.resourceContents.size());
    if(storageMode == MTL::StorageModeShared)
    {
      // MTLStorageModeShared buffers automatically synchronized
      memcpy(mtlBuffer->contents(), initial.resourceContents.data(), initial.resourceContents.size());
    }
    else if(storageMode == MTL::StorageModeManaged)
    {
      // MTLStorageModeManaged buffers need to call MTLBuffer::didModifyRange
      memcpy(mtlBuffer->contents(), initial.resourceContents.data(), initial.resourceContents.size());
      NS::Range range = NS::Range::Make(0, len);
      mtlBuffer->didModifyRange(range);
    }
    else if(storageMode == MTL::StorageModePrivate)
    {
      // MTLStorageModePrivate need to copy from a MTLStorageModeShared buffer
      MTL::Buffer *mtlSharedBuffer = Unwrap(this)->newBuffer(initial.resourceContents.data(), len,
                                                             MTL::ResourceStorageModeShared);
      MTL::CommandBuffer *mtlCommandBuffer = m_mtlCommandQueue->commandBuffer();
      MTL::BlitCommandEncoder *mtlBlitEncoder = mtlCommandBuffer->blitCommandEncoder();
      mtlBlitEncoder->copyFromBuffer(mtlSharedBuffer, 0, mtlBuffer, 0, len);
      mtlBlitEncoder->endEncoding();
      mtlCommandBuffer->commit();
      mtlCommandBuffer->waitUntilCompleted();
      mtlSharedBuffer->release();
    }
    return;
  }
  RDCERR("Unhandled resource type %d", type);
}

INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLDevice, void, InitialState, ResourceId id,
                                MetalResourceRecord *record, const MetalInitialContents *initial);
