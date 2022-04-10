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

#include "metal_common.h"

// TODO: YUV format support
static uint32_t GetYUVPlaneCount(MTL::PixelFormat format)
{
  switch(format)
  {
    default: break;
  }

  return 1;
}

// TODO: YUV support
static MTL::Size GetPlaneShape(uint32_t width, uint32_t height, MTL::PixelFormat format,
                               uint32_t plane)
{
  switch(format)
  {
    default: return MTL::Size(width, height, 0);
  }
}

// The shape of blocks in (a plane of) a texture format.
// Non-block formats are considered to have 1x1 blocks.
struct BlockShape
{
  // the width of a block, in texels (or 1 for non-block formats)
  uint32_t width;

  // the height of a block, in texels (or 1 for non-block formats)
  uint32_t height;

  // the number of bytes used to encode the block
  uint32_t bytes;
};

// TODO: Support all format types
static BlockShape GetBlockShape(MTL::PixelFormat format, uint32_t plane)
{
  switch(format)
  {
    case MTL::PixelFormatBGRA8Unorm:
    case MTL::PixelFormatRGBA8Unorm:
    case MTL::PixelFormatBGRA8Unorm_sRGB:
    case MTL::PixelFormatRGBA8Unorm_sRGB:
    case MTL::PixelFormatRGBA8Snorm:
    case MTL::PixelFormatRGBA8Uint: return {1, 1, 4};

    default: RDCERR("Unrecognised MTL::PixelFormat: %d", format);
  }
  return {1, 1, 1};
}

static uint32_t GetPlaneByteSize(uint32_t width, uint32_t height, uint32_t depth,
                                 MTL::PixelFormat format, uint32_t mip, uint32_t plane)
{
  uint32_t mipWidth = RDCMAX(width >> mip, 1U);
  uint32_t mipHeight = RDCMAX(height >> mip, 1U);
  uint32_t mipDepth = RDCMAX(depth >> mip, 1U);

  MTL::Size planeShape = GetPlaneShape(mipWidth, mipHeight, format, plane);

  BlockShape blockShape = GetBlockShape(format, plane);

  uint32_t widthInBlocks = (planeShape.width + blockShape.width - 1) / blockShape.width;
  uint32_t heightInBlocks = (planeShape.height + blockShape.height - 1) / blockShape.height;

  return blockShape.bytes * widthInBlocks * heightInBlocks * mipDepth;
}

ResourceFormat MakeResourceFormat(MTL::PixelFormat format)
{
  ResourceFormat ret;

  ret.type = ResourceFormatType::Regular;
  ret.compByteWidth = 0;
  ret.compCount = 0;
  ret.compType = CompType::Typeless;

  if(format == MTL::PixelFormatInvalid)
  {
    ret.type = ResourceFormatType::Undefined;
    return ret;
  }

  switch(format)
  {
    case MTL::PixelFormatA8Unorm: ret.type = ResourceFormatType::A8; break;
    case MTL::PixelFormatB5G6R5Unorm: ret.type = ResourceFormatType::R5G6B5; break;
    case MTL::PixelFormatA1BGR5Unorm:
    case MTL::PixelFormatBGR5A1Unorm: ret.type = ResourceFormatType::R5G5B5A1; break;
    case MTL::PixelFormatABGR4Unorm: ret.type = ResourceFormatType::R4G4B4A4; break;
    case MTL::PixelFormatRGB10A2Unorm:
    case MTL::PixelFormatRG11B10Float: ret.type = ResourceFormatType::R11G11B10; break;
    case MTL::PixelFormatRGB9E5Float: ret.type = ResourceFormatType::R9G9B9E5; break;
    case MTL::PixelFormatRGB10A2Uint:
    case MTL::PixelFormatBGR10A2Unorm:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatBGR10_XR_sRGB:
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBGRA10_XR_sRGB: ret.type = ResourceFormatType::R10G10B10A2; break;
    case MTL::PixelFormatBC1_RGBA:
    case MTL::PixelFormatBC1_RGBA_sRGB: ret.type = ResourceFormatType::BC1; break;
    case MTL::PixelFormatBC2_RGBA:
    case MTL::PixelFormatBC2_RGBA_sRGB: ret.type = ResourceFormatType::BC2; break;
    case MTL::PixelFormatBC3_RGBA:
    case MTL::PixelFormatBC3_RGBA_sRGB: ret.type = ResourceFormatType::BC3; break;
    case MTL::PixelFormatBC4_RUnorm:
    case MTL::PixelFormatBC4_RSnorm: ret.type = ResourceFormatType::BC4; break;
    case MTL::PixelFormatBC5_RGUnorm:
    case MTL::PixelFormatBC5_RGSnorm: ret.type = ResourceFormatType::BC5; break;
    case MTL::PixelFormatBC6H_RGBFloat:
    case MTL::PixelFormatBC6H_RGBUfloat: ret.type = ResourceFormatType::BC6; break;
    case MTL::PixelFormatBC7_RGBAUnorm:
    case MTL::PixelFormatBC7_RGBAUnorm_sRGB: ret.type = ResourceFormatType::BC7; break;
    case MTL::PixelFormatPVRTC_RGB_2BPP:
    case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGB_4BPP:
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_2BPP:
    case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_4BPP:
    case MTL::PixelFormatPVRTC_RGBA_4BPP_sRGB: ret.type = ResourceFormatType::PVRTC; break;
    case MTL::PixelFormatEAC_R11Unorm:
    case MTL::PixelFormatEAC_R11Snorm:
    case MTL::PixelFormatEAC_RG11Unorm:
    case MTL::PixelFormatEAC_RG11Snorm:
    case MTL::PixelFormatEAC_RGBA8:
    case MTL::PixelFormatEAC_RGBA8_sRGB: ret.type = ResourceFormatType::EAC; break;
    case MTL::PixelFormatETC2_RGB8:
    case MTL::PixelFormatETC2_RGB8_sRGB:
    case MTL::PixelFormatETC2_RGB8A1:
    case MTL::PixelFormatETC2_RGB8A1_sRGB: ret.type = ResourceFormatType::ETC2; break;
    case MTL::PixelFormatASTC_4x4_sRGB:
    case MTL::PixelFormatASTC_5x4_sRGB:
    case MTL::PixelFormatASTC_5x5_sRGB:
    case MTL::PixelFormatASTC_6x5_sRGB:
    case MTL::PixelFormatASTC_6x6_sRGB:
    case MTL::PixelFormatASTC_8x5_sRGB:
    case MTL::PixelFormatASTC_8x6_sRGB:
    case MTL::PixelFormatASTC_8x8_sRGB:
    case MTL::PixelFormatASTC_10x5_sRGB:
    case MTL::PixelFormatASTC_10x6_sRGB:
    case MTL::PixelFormatASTC_10x8_sRGB:
    case MTL::PixelFormatASTC_10x10_sRGB:
    case MTL::PixelFormatASTC_12x10_sRGB:
    case MTL::PixelFormatASTC_12x12_sRGB:
    case MTL::PixelFormatASTC_4x4_LDR:
    case MTL::PixelFormatASTC_5x4_LDR:
    case MTL::PixelFormatASTC_5x5_LDR:
    case MTL::PixelFormatASTC_6x5_LDR:
    case MTL::PixelFormatASTC_6x6_LDR:
    case MTL::PixelFormatASTC_8x5_LDR:
    case MTL::PixelFormatASTC_8x6_LDR:
    case MTL::PixelFormatASTC_8x8_LDR:
    case MTL::PixelFormatASTC_10x5_LDR:
    case MTL::PixelFormatASTC_10x6_LDR:
    case MTL::PixelFormatASTC_10x8_LDR:
    case MTL::PixelFormatASTC_10x10_LDR:
    case MTL::PixelFormatASTC_12x10_LDR:
    case MTL::PixelFormatASTC_12x12_LDR:
    case MTL::PixelFormatASTC_4x4_HDR:
    case MTL::PixelFormatASTC_5x4_HDR:
    case MTL::PixelFormatASTC_5x5_HDR:
    case MTL::PixelFormatASTC_6x5_HDR:
    case MTL::PixelFormatASTC_6x6_HDR:
    case MTL::PixelFormatASTC_8x5_HDR:
    case MTL::PixelFormatASTC_8x6_HDR:
    case MTL::PixelFormatASTC_8x8_HDR:
    case MTL::PixelFormatASTC_10x5_HDR:
    case MTL::PixelFormatASTC_10x6_HDR:
    case MTL::PixelFormatASTC_10x8_HDR:
    case MTL::PixelFormatASTC_10x10_HDR:
    case MTL::PixelFormatASTC_12x10_HDR:
    case MTL::PixelFormatASTC_12x12_HDR: ret.type = ResourceFormatType::ASTC; break;
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422: ret.type = ResourceFormatType::YUV16; break;
    case MTL::PixelFormatDepth24Unorm_Stencil8: ret.type = ResourceFormatType::D24S8; break;
    case MTL::PixelFormatDepth32Float_Stencil8: ret.type = ResourceFormatType::D32S8; break;
    case MTL::PixelFormatStencil8: ret.type = ResourceFormatType::S8; break;
    case MTL::PixelFormatX32_Stencil8:
    case MTL::PixelFormatX24_Stencil8: ret.type = ResourceFormatType::S8; break;
    default: break;
  }

  // TODO: set the BGRA Order
  switch(format)
  {
    case MTL::PixelFormatB5G6R5Unorm:
    case MTL::PixelFormatA1BGR5Unorm:
    case MTL::PixelFormatABGR4Unorm:
    case MTL::PixelFormatBGR5A1Unorm:
    case MTL::PixelFormatBGRA8Unorm:
    case MTL::PixelFormatBGRA8Unorm_sRGB:
    case MTL::PixelFormatBGR10A2Unorm:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatBGR10_XR_sRGB:
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBGRA10_XR_sRGB:
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422: ret.SetBGRAOrder(true); break;
    default: break;
  }

  switch(format)
  {
    case MTL::PixelFormatA8Unorm:
    case MTL::PixelFormatR8Unorm:
    case MTL::PixelFormatR8Unorm_sRGB:
    case MTL::PixelFormatR8Snorm:
    case MTL::PixelFormatR8Uint:
    case MTL::PixelFormatR8Sint:
    case MTL::PixelFormatR16Unorm:
    case MTL::PixelFormatR16Snorm:
    case MTL::PixelFormatR16Uint:
    case MTL::PixelFormatR16Sint:
    case MTL::PixelFormatR16Float:
    case MTL::PixelFormatR32Uint:
    case MTL::PixelFormatR32Sint:
    case MTL::PixelFormatR32Float:
    case MTL::PixelFormatEAC_R11Unorm:
    case MTL::PixelFormatEAC_R11Snorm:
    case MTL::PixelFormatBC4_RUnorm:
    case MTL::PixelFormatBC4_RSnorm:
    case MTL::PixelFormatDepth16Unorm:
    case MTL::PixelFormatDepth32Float:
    case MTL::PixelFormatStencil8:
    case MTL::PixelFormatX32_Stencil8:
    case MTL::PixelFormatX24_Stencil8: ret.compCount = 1; break;
    case MTL::PixelFormatRG8Unorm:
    case MTL::PixelFormatRG8Unorm_sRGB:
    case MTL::PixelFormatRG8Snorm:
    case MTL::PixelFormatRG8Uint:
    case MTL::PixelFormatRG8Sint:
    case MTL::PixelFormatRG16Unorm:
    case MTL::PixelFormatRG16Snorm:
    case MTL::PixelFormatRG16Uint:
    case MTL::PixelFormatRG16Sint:
    case MTL::PixelFormatRG16Float:
    case MTL::PixelFormatRG32Uint:
    case MTL::PixelFormatRG32Sint:
    case MTL::PixelFormatRG32Float:
    case MTL::PixelFormatBC5_RGUnorm:
    case MTL::PixelFormatBC5_RGSnorm:
    case MTL::PixelFormatEAC_RG11Unorm:
    case MTL::PixelFormatEAC_RG11Snorm:
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422:
    case MTL::PixelFormatDepth24Unorm_Stencil8:
    case MTL::PixelFormatDepth32Float_Stencil8: ret.compCount = 2; break;
    case MTL::PixelFormatB5G6R5Unorm:
    case MTL::PixelFormatRG11B10Float:
    case MTL::PixelFormatRGB9E5Float:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatBGR10_XR_sRGB:
    case MTL::PixelFormatBC6H_RGBFloat:
    case MTL::PixelFormatBC6H_RGBUfloat:
    case MTL::PixelFormatPVRTC_RGB_2BPP:
    case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGB_4BPP:
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTL::PixelFormatETC2_RGB8:
    case MTL::PixelFormatETC2_RGB8_sRGB: ret.compCount = 3; break;
    case MTL::PixelFormatA1BGR5Unorm:
    case MTL::PixelFormatABGR4Unorm:
    case MTL::PixelFormatBGR5A1Unorm:
    case MTL::PixelFormatRGBA8Unorm:
    case MTL::PixelFormatRGBA8Unorm_sRGB:
    case MTL::PixelFormatRGBA8Snorm:
    case MTL::PixelFormatRGBA8Uint:
    case MTL::PixelFormatRGBA8Sint:
    case MTL::PixelFormatBGRA8Unorm:
    case MTL::PixelFormatBGRA8Unorm_sRGB:
    case MTL::PixelFormatRGB10A2Unorm:
    case MTL::PixelFormatRGB10A2Uint:
    case MTL::PixelFormatBGR10A2Unorm:
    case MTL::PixelFormatRGBA16Unorm:
    case MTL::PixelFormatRGBA16Snorm:
    case MTL::PixelFormatRGBA16Uint:
    case MTL::PixelFormatRGBA16Sint:
    case MTL::PixelFormatRGBA16Float:
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBGRA10_XR_sRGB:
    case MTL::PixelFormatRGBA32Uint:
    case MTL::PixelFormatRGBA32Sint:
    case MTL::PixelFormatRGBA32Float:
    case MTL::PixelFormatBC1_RGBA:
    case MTL::PixelFormatBC1_RGBA_sRGB:
    case MTL::PixelFormatBC2_RGBA:
    case MTL::PixelFormatBC2_RGBA_sRGB:
    case MTL::PixelFormatBC3_RGBA:
    case MTL::PixelFormatBC3_RGBA_sRGB:
    case MTL::PixelFormatBC7_RGBAUnorm:
    case MTL::PixelFormatBC7_RGBAUnorm_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_2BPP:
    case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_4BPP:
    case MTL::PixelFormatPVRTC_RGBA_4BPP_sRGB:
    case MTL::PixelFormatEAC_RGBA8:
    case MTL::PixelFormatEAC_RGBA8_sRGB:
    case MTL::PixelFormatETC2_RGB8A1:
    case MTL::PixelFormatETC2_RGB8A1_sRGB:
    case MTL::PixelFormatASTC_4x4_sRGB:
    case MTL::PixelFormatASTC_5x4_sRGB:
    case MTL::PixelFormatASTC_5x5_sRGB:
    case MTL::PixelFormatASTC_6x5_sRGB:
    case MTL::PixelFormatASTC_6x6_sRGB:
    case MTL::PixelFormatASTC_8x5_sRGB:
    case MTL::PixelFormatASTC_8x6_sRGB:
    case MTL::PixelFormatASTC_8x8_sRGB:
    case MTL::PixelFormatASTC_10x5_sRGB:
    case MTL::PixelFormatASTC_10x6_sRGB:
    case MTL::PixelFormatASTC_10x8_sRGB:
    case MTL::PixelFormatASTC_10x10_sRGB:
    case MTL::PixelFormatASTC_12x10_sRGB:
    case MTL::PixelFormatASTC_12x12_sRGB:
    case MTL::PixelFormatASTC_4x4_LDR:
    case MTL::PixelFormatASTC_5x4_LDR:
    case MTL::PixelFormatASTC_5x5_LDR:
    case MTL::PixelFormatASTC_6x5_LDR:
    case MTL::PixelFormatASTC_6x6_LDR:
    case MTL::PixelFormatASTC_8x5_LDR:
    case MTL::PixelFormatASTC_8x6_LDR:
    case MTL::PixelFormatASTC_8x8_LDR:
    case MTL::PixelFormatASTC_10x5_LDR:
    case MTL::PixelFormatASTC_10x6_LDR:
    case MTL::PixelFormatASTC_10x8_LDR:
    case MTL::PixelFormatASTC_10x10_LDR:
    case MTL::PixelFormatASTC_12x10_LDR:
    case MTL::PixelFormatASTC_12x12_LDR:
    case MTL::PixelFormatASTC_4x4_HDR:
    case MTL::PixelFormatASTC_5x4_HDR:
    case MTL::PixelFormatASTC_5x5_HDR:
    case MTL::PixelFormatASTC_6x5_HDR:
    case MTL::PixelFormatASTC_6x6_HDR:
    case MTL::PixelFormatASTC_8x5_HDR:
    case MTL::PixelFormatASTC_8x6_HDR:
    case MTL::PixelFormatASTC_8x8_HDR:
    case MTL::PixelFormatASTC_10x5_HDR:
    case MTL::PixelFormatASTC_10x6_HDR:
    case MTL::PixelFormatASTC_10x8_HDR:
    case MTL::PixelFormatASTC_10x10_HDR:
    case MTL::PixelFormatASTC_12x10_HDR:
    case MTL::PixelFormatASTC_12x12_HDR: ret.compCount = 4; break;
    case MTL::PixelFormatInvalid: ret.compCount = 1; break;
  }

  // TODO: complete setting compType
  switch(format)
  {
    case MTL::PixelFormatA8Unorm:
    case MTL::PixelFormatR8Unorm:
    case MTL::PixelFormatR16Unorm:
    case MTL::PixelFormatRG8Unorm:
    case MTL::PixelFormatB5G6R5Unorm:
    case MTL::PixelFormatA1BGR5Unorm:
    case MTL::PixelFormatABGR4Unorm:
    case MTL::PixelFormatBGR5A1Unorm:
    case MTL::PixelFormatRG16Unorm:
    case MTL::PixelFormatRGBA8Unorm:
    case MTL::PixelFormatBGRA8Unorm:
    case MTL::PixelFormatRGB10A2Unorm:
    case MTL::PixelFormatBGR10A2Unorm:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatRGBA16Unorm:
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBC1_RGBA:
    case MTL::PixelFormatBC2_RGBA:
    case MTL::PixelFormatBC3_RGBA:
    case MTL::PixelFormatBC4_RUnorm:
    case MTL::PixelFormatBC5_RGUnorm:
    case MTL::PixelFormatBC7_RGBAUnorm:
    case MTL::PixelFormatPVRTC_RGB_2BPP:
    case MTL::PixelFormatPVRTC_RGB_4BPP:
    case MTL::PixelFormatPVRTC_RGBA_2BPP:
    case MTL::PixelFormatPVRTC_RGBA_4BPP:
    case MTL::PixelFormatEAC_R11Unorm:
    case MTL::PixelFormatEAC_RG11Unorm:
    case MTL::PixelFormatEAC_RGBA8:
    case MTL::PixelFormatETC2_RGB8:
    case MTL::PixelFormatETC2_RGB8A1:
    case MTL::PixelFormatASTC_4x4_LDR:
    case MTL::PixelFormatASTC_5x4_LDR:
    case MTL::PixelFormatASTC_5x5_LDR:
    case MTL::PixelFormatASTC_6x5_LDR:
    case MTL::PixelFormatASTC_6x6_LDR:
    case MTL::PixelFormatASTC_8x5_LDR:
    case MTL::PixelFormatASTC_8x6_LDR:
    case MTL::PixelFormatASTC_8x8_LDR:
    case MTL::PixelFormatASTC_10x5_LDR:
    case MTL::PixelFormatASTC_10x6_LDR:
    case MTL::PixelFormatASTC_10x8_LDR:
    case MTL::PixelFormatASTC_10x10_LDR:
    case MTL::PixelFormatASTC_12x10_LDR:
    case MTL::PixelFormatASTC_12x12_LDR:
    case MTL::PixelFormatASTC_4x4_HDR:
    case MTL::PixelFormatASTC_5x4_HDR:
    case MTL::PixelFormatASTC_5x5_HDR:
    case MTL::PixelFormatASTC_6x5_HDR:
    case MTL::PixelFormatASTC_6x6_HDR:
    case MTL::PixelFormatASTC_8x5_HDR:
    case MTL::PixelFormatASTC_8x6_HDR:
    case MTL::PixelFormatASTC_8x8_HDR:
    case MTL::PixelFormatASTC_10x5_HDR:
    case MTL::PixelFormatASTC_10x6_HDR:
    case MTL::PixelFormatASTC_10x8_HDR:
    case MTL::PixelFormatASTC_10x10_HDR:
    case MTL::PixelFormatASTC_12x10_HDR:
    case MTL::PixelFormatASTC_12x12_HDR:
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422: ret.compType = CompType::UNorm; break;
    case MTL::PixelFormatR8Unorm_sRGB:
    case MTL::PixelFormatRG8Unorm_sRGB:
    case MTL::PixelFormatRGBA8Unorm_sRGB:
    case MTL::PixelFormatBGRA8Unorm_sRGB:
    case MTL::PixelFormatBGR10_XR_sRGB:
    case MTL::PixelFormatBGRA10_XR_sRGB:
    case MTL::PixelFormatBC1_RGBA_sRGB:
    case MTL::PixelFormatBC2_RGBA_sRGB:
    case MTL::PixelFormatBC3_RGBA_sRGB:
    case MTL::PixelFormatBC7_RGBAUnorm_sRGB:
    case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_4BPP_sRGB:
    case MTL::PixelFormatEAC_RGBA8_sRGB:
    case MTL::PixelFormatETC2_RGB8_sRGB:
    case MTL::PixelFormatETC2_RGB8A1_sRGB:
    case MTL::PixelFormatASTC_4x4_sRGB:
    case MTL::PixelFormatASTC_5x4_sRGB:
    case MTL::PixelFormatASTC_5x5_sRGB:
    case MTL::PixelFormatASTC_6x5_sRGB:
    case MTL::PixelFormatASTC_6x6_sRGB:
    case MTL::PixelFormatASTC_8x5_sRGB:
    case MTL::PixelFormatASTC_8x6_sRGB:
    case MTL::PixelFormatASTC_8x8_sRGB:
    case MTL::PixelFormatASTC_10x5_sRGB:
    case MTL::PixelFormatASTC_10x6_sRGB:
    case MTL::PixelFormatASTC_10x8_sRGB:
    case MTL::PixelFormatASTC_10x10_sRGB:
    case MTL::PixelFormatASTC_12x10_sRGB:
    case MTL::PixelFormatASTC_12x12_sRGB: ret.compType = CompType::UNormSRGB; break;
    case MTL::PixelFormatR8Snorm:
    case MTL::PixelFormatR16Snorm:
    case MTL::PixelFormatRG8Snorm:
    case MTL::PixelFormatRG16Snorm:
    case MTL::PixelFormatRGBA8Snorm:
    case MTL::PixelFormatRGBA16Snorm:
    case MTL::PixelFormatBC4_RSnorm:
    case MTL::PixelFormatBC5_RGSnorm:
    case MTL::PixelFormatEAC_R11Snorm:
    case MTL::PixelFormatEAC_RG11Snorm: ret.compType = CompType::SNorm; break;
    case MTL::PixelFormatR8Uint:
    case MTL::PixelFormatR16Uint:
    case MTL::PixelFormatRG8Uint:
    case MTL::PixelFormatR32Uint:
    case MTL::PixelFormatRG16Uint:
    case MTL::PixelFormatRGBA8Uint:
    case MTL::PixelFormatRGB10A2Uint:
    case MTL::PixelFormatRG32Uint:
    case MTL::PixelFormatRGBA16Uint:
    case MTL::PixelFormatRGBA32Uint: ret.compType = CompType::UInt; break;
    case MTL::PixelFormatR8Sint:
    case MTL::PixelFormatR16Sint:
    case MTL::PixelFormatRG8Sint:
    case MTL::PixelFormatR32Sint:
    case MTL::PixelFormatRG16Sint:
    case MTL::PixelFormatRGBA8Sint:
    case MTL::PixelFormatRG32Sint:
    case MTL::PixelFormatRGBA16Sint:
    case MTL::PixelFormatRGBA32Sint: ret.compType = CompType::SInt; break;
    case MTL::PixelFormatR16Float:
    case MTL::PixelFormatR32Float:
    case MTL::PixelFormatRG16Float:
    case MTL::PixelFormatRG11B10Float:
    case MTL::PixelFormatRGB9E5Float:
    case MTL::PixelFormatRG32Float:
    case MTL::PixelFormatRGBA16Float:
    case MTL::PixelFormatRGBA32Float:
    case MTL::PixelFormatBC6H_RGBFloat:
    case MTL::PixelFormatBC6H_RGBUfloat: ret.compType = CompType::Float; break;
    case MTL::PixelFormatDepth16Unorm:
    case MTL::PixelFormatDepth32Float:
    case MTL::PixelFormatStencil8:
    case MTL::PixelFormatDepth24Unorm_Stencil8:
    case MTL::PixelFormatDepth32Float_Stencil8:
    case MTL::PixelFormatX32_Stencil8:
    case MTL::PixelFormatX24_Stencil8: ret.compType = CompType::Depth; break;
    case MTL::PixelFormatInvalid: ret.compType = CompType::Typeless; break;
  }

  switch(format)
  {
    case MTL::PixelFormatA8Unorm:
    case MTL::PixelFormatR8Unorm:
    case MTL::PixelFormatR8Unorm_sRGB:
    case MTL::PixelFormatR8Snorm:
    case MTL::PixelFormatR8Uint:
    case MTL::PixelFormatR8Sint:
    case MTL::PixelFormatRG8Unorm:
    case MTL::PixelFormatRG8Unorm_sRGB:
    case MTL::PixelFormatRG8Snorm:
    case MTL::PixelFormatRG8Uint:
    case MTL::PixelFormatRG8Sint:
    case MTL::PixelFormatRGBA8Unorm:
    case MTL::PixelFormatRGBA8Unorm_sRGB:
    case MTL::PixelFormatRGBA8Snorm:
    case MTL::PixelFormatRGBA8Uint:
    case MTL::PixelFormatRGBA8Sint:
    case MTL::PixelFormatBGRA8Unorm:
    case MTL::PixelFormatBGRA8Unorm_sRGB:
    case MTL::PixelFormatRGB10A2Unorm:
    case MTL::PixelFormatRGB10A2Uint:
    case MTL::PixelFormatRG11B10Float:
    case MTL::PixelFormatRGB9E5Float:
    case MTL::PixelFormatBGR10A2Unorm:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatBGR10_XR_sRGB:
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBGRA10_XR_sRGB:
    case MTL::PixelFormatStencil8:
    case MTL::PixelFormatDepth32Float_Stencil8:
    case MTL::PixelFormatX32_Stencil8:
    case MTL::PixelFormatX24_Stencil8: ret.compByteWidth = 1; break;
    case MTL::PixelFormatR16Unorm:
    case MTL::PixelFormatR16Snorm:
    case MTL::PixelFormatR16Uint:
    case MTL::PixelFormatR16Sint:
    case MTL::PixelFormatR16Float:
    case MTL::PixelFormatB5G6R5Unorm:
    case MTL::PixelFormatA1BGR5Unorm:
    case MTL::PixelFormatABGR4Unorm:
    case MTL::PixelFormatBGR5A1Unorm:
    case MTL::PixelFormatRG16Unorm:
    case MTL::PixelFormatRG16Snorm:
    case MTL::PixelFormatRG16Uint:
    case MTL::PixelFormatRG16Sint:
    case MTL::PixelFormatRG16Float:
    case MTL::PixelFormatRGBA16Unorm:
    case MTL::PixelFormatRGBA16Snorm:
    case MTL::PixelFormatRGBA16Uint:
    case MTL::PixelFormatRGBA16Sint:
    case MTL::PixelFormatRGBA16Float:
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422:
    case MTL::PixelFormatDepth16Unorm: ret.compByteWidth = 2; break;

    case MTL::PixelFormatDepth24Unorm_Stencil8: ret.compByteWidth = 3; break;

    case MTL::PixelFormatR32Uint:
    case MTL::PixelFormatR32Sint:
    case MTL::PixelFormatR32Float:
    case MTL::PixelFormatRG32Uint:
    case MTL::PixelFormatRG32Sint:
    case MTL::PixelFormatRG32Float:
    case MTL::PixelFormatRGBA32Uint:
    case MTL::PixelFormatRGBA32Sint:
    case MTL::PixelFormatRGBA32Float:
    case MTL::PixelFormatDepth32Float: ret.compByteWidth = 4; break;
    case MTL::PixelFormatBC1_RGBA:
    case MTL::PixelFormatBC1_RGBA_sRGB:
    case MTL::PixelFormatBC2_RGBA:
    case MTL::PixelFormatBC2_RGBA_sRGB:
    case MTL::PixelFormatBC3_RGBA:
    case MTL::PixelFormatBC3_RGBA_sRGB:
    case MTL::PixelFormatBC4_RUnorm:
    case MTL::PixelFormatBC4_RSnorm:
    case MTL::PixelFormatBC5_RGUnorm:
    case MTL::PixelFormatBC5_RGSnorm:
    case MTL::PixelFormatBC6H_RGBFloat:
    case MTL::PixelFormatBC6H_RGBUfloat:
    case MTL::PixelFormatBC7_RGBAUnorm:
    case MTL::PixelFormatBC7_RGBAUnorm_sRGB:
    case MTL::PixelFormatPVRTC_RGB_2BPP:
    case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGB_4BPP:
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_2BPP:
    case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_4BPP:
    case MTL::PixelFormatPVRTC_RGBA_4BPP_sRGB:
    case MTL::PixelFormatEAC_R11Unorm:
    case MTL::PixelFormatEAC_R11Snorm:
    case MTL::PixelFormatEAC_RG11Unorm:
    case MTL::PixelFormatEAC_RG11Snorm:
    case MTL::PixelFormatEAC_RGBA8:
    case MTL::PixelFormatEAC_RGBA8_sRGB:
    case MTL::PixelFormatETC2_RGB8:
    case MTL::PixelFormatETC2_RGB8_sRGB:
    case MTL::PixelFormatETC2_RGB8A1:
    case MTL::PixelFormatETC2_RGB8A1_sRGB:
    case MTL::PixelFormatASTC_4x4_sRGB:
    case MTL::PixelFormatASTC_5x4_sRGB:
    case MTL::PixelFormatASTC_5x5_sRGB:
    case MTL::PixelFormatASTC_6x5_sRGB:
    case MTL::PixelFormatASTC_6x6_sRGB:
    case MTL::PixelFormatASTC_8x5_sRGB:
    case MTL::PixelFormatASTC_8x6_sRGB:
    case MTL::PixelFormatASTC_8x8_sRGB:
    case MTL::PixelFormatASTC_10x5_sRGB:
    case MTL::PixelFormatASTC_10x6_sRGB:
    case MTL::PixelFormatASTC_10x8_sRGB:
    case MTL::PixelFormatASTC_10x10_sRGB:
    case MTL::PixelFormatASTC_12x10_sRGB:
    case MTL::PixelFormatASTC_12x12_sRGB:
    case MTL::PixelFormatASTC_4x4_LDR:
    case MTL::PixelFormatASTC_5x4_LDR:
    case MTL::PixelFormatASTC_5x5_LDR:
    case MTL::PixelFormatASTC_6x5_LDR:
    case MTL::PixelFormatASTC_6x6_LDR:
    case MTL::PixelFormatASTC_8x5_LDR:
    case MTL::PixelFormatASTC_8x6_LDR:
    case MTL::PixelFormatASTC_8x8_LDR:
    case MTL::PixelFormatASTC_10x5_LDR:
    case MTL::PixelFormatASTC_10x6_LDR:
    case MTL::PixelFormatASTC_10x8_LDR:
    case MTL::PixelFormatASTC_10x10_LDR:
    case MTL::PixelFormatASTC_12x10_LDR:
    case MTL::PixelFormatASTC_12x12_LDR:
    case MTL::PixelFormatASTC_4x4_HDR:
    case MTL::PixelFormatASTC_5x4_HDR:
    case MTL::PixelFormatASTC_5x5_HDR:
    case MTL::PixelFormatASTC_6x5_HDR:
    case MTL::PixelFormatASTC_6x6_HDR:
    case MTL::PixelFormatASTC_8x5_HDR:
    case MTL::PixelFormatASTC_8x6_HDR:
    case MTL::PixelFormatASTC_8x8_HDR:
    case MTL::PixelFormatASTC_10x5_HDR:
    case MTL::PixelFormatASTC_10x6_HDR:
    case MTL::PixelFormatASTC_10x8_HDR:
    case MTL::PixelFormatASTC_10x10_HDR:
    case MTL::PixelFormatASTC_12x10_HDR:
    case MTL::PixelFormatASTC_12x12_HDR: ret.compByteWidth = 1; break;
    case MTL::PixelFormatInvalid: ret.compByteWidth = 1; break;
  }

  // TODO: YUV format support

  return ret;
}

uint32_t GetByteSize(uint32_t width, uint32_t height, uint32_t depth, MTL::PixelFormat format,
                     uint32_t mip)
{
  uint32_t planeCount = GetYUVPlaneCount(format);
  uint32_t size = 0;
  for(uint32_t p = 0; p < planeCount; p++)
    size += GetPlaneByteSize(width, height, depth, format, mip, p);
  return size;
}
