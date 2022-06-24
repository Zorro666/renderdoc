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

static bool IsOneComponent(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
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
    case MTL::PixelFormatBC4_RUnorm:
    case MTL::PixelFormatBC4_RSnorm:
    case MTL::PixelFormatEAC_R11Unorm:
    case MTL::PixelFormatEAC_R11Snorm:
    case MTL::PixelFormatDepth16Unorm:
    case MTL::PixelFormatDepth32Float:
    case MTL::PixelFormatStencil8:
    case MTL::PixelFormatX32_Stencil8:
    case MTL::PixelFormatX24_Stencil8: return true;
    default: return false;
  }
};

static bool IsTwoComponent(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
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
    case MTL::PixelFormatDepth24Unorm_Stencil8:
    case MTL::PixelFormatDepth32Float_Stencil8: return true;
    default: return false;
  }
};

static bool IsThreeComponent(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatB5G6R5Unorm:
    case MTL::PixelFormatRGB9E5Float:
    case MTL::PixelFormatRG11B10Float:
    case MTL::PixelFormatBC6H_RGBFloat:
    case MTL::PixelFormatBC6H_RGBUfloat:
    case MTL::PixelFormatPVRTC_RGB_2BPP:
    case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGB_4BPP:
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTL::PixelFormatETC2_RGB8:
    case MTL::PixelFormatETC2_RGB8_sRGB:
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatBGR10_XR_sRGB: return true;
    default: return false;
  }
};

static bool IsFourComponent(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
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
    case MTL::PixelFormatASTC_12x12_HDR:
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBGRA10_XR_sRGB: return true;
    default: return false;
  }
};

static bool IsBlockFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
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
    case MTL::PixelFormatPVRTC_RGBA_2BPP:
    case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGB_4BPP:
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
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
    case MTL::PixelFormatASTC_12x12_HDR: return true;
    default: return false;
  }
}

static bool IsDepthOrStencilFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatDepth16Unorm:
    case MTL::PixelFormatDepth32Float:
    case MTL::PixelFormatStencil8:
    case MTL::PixelFormatDepth24Unorm_Stencil8:
    case MTL::PixelFormatDepth32Float_Stencil8:
    case MTL::PixelFormatX32_Stencil8:
    case MTL::PixelFormatX24_Stencil8: return true;
    default: return false;
  }
}

static bool IsUNormFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatA8Unorm:
    case MTL::PixelFormatR8Unorm:
    case MTL::PixelFormatR8Unorm_sRGB:
    case MTL::PixelFormatR16Unorm:
    case MTL::PixelFormatRG8Unorm:
    case MTL::PixelFormatB5G6R5Unorm:
    case MTL::PixelFormatA1BGR5Unorm:
    case MTL::PixelFormatABGR4Unorm:
    case MTL::PixelFormatBGR5A1Unorm:
    case MTL::PixelFormatRG16Unorm:
    case MTL::PixelFormatRGBA8Unorm:
    case MTL::PixelFormatRGBA8Unorm_sRGB:
    case MTL::PixelFormatBGRA8Unorm:
    case MTL::PixelFormatBGRA8Unorm_sRGB:
    case MTL::PixelFormatRGB10A2Unorm:
    case MTL::PixelFormatBGR10A2Unorm:
    case MTL::PixelFormatRGBA16Unorm:
    case MTL::PixelFormatBC1_RGBA:
    case MTL::PixelFormatBC1_RGBA_sRGB:
    case MTL::PixelFormatBC2_RGBA:
    case MTL::PixelFormatBC2_RGBA_sRGB:
    case MTL::PixelFormatBC3_RGBA:
    case MTL::PixelFormatBC3_RGBA_sRGB:
    case MTL::PixelFormatBC4_RUnorm:
    case MTL::PixelFormatBC5_RGUnorm:
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
    case MTL::PixelFormatEAC_RG11Unorm:
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
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422:
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBGRA10_XR_sRGB:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatBGR10_XR_sRGB: return true;
    default: return false;
  }
};

static bool IsSNormFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatR8Snorm:
    case MTL::PixelFormatR16Snorm:
    case MTL::PixelFormatRG8Snorm:
    case MTL::PixelFormatRG16Snorm:
    case MTL::PixelFormatRGBA8Snorm:
    case MTL::PixelFormatRGBA16Snorm:
    case MTL::PixelFormatBC4_RSnorm:
    case MTL::PixelFormatBC5_RGSnorm:
    case MTL::PixelFormatEAC_R11Snorm:
    case MTL::PixelFormatEAC_RG11Snorm: return true;
    default: return false;
  }
};

static bool IsFloatFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatR16Float:
    case MTL::PixelFormatR32Float:
    case MTL::PixelFormatRG16Float:
    case MTL::PixelFormatRG11B10Float:
    case MTL::PixelFormatRGB9E5Float:
    case MTL::PixelFormatRG32Float:
    case MTL::PixelFormatRGBA16Float:
    case MTL::PixelFormatRGBA32Float:
    case MTL::PixelFormatBC6H_RGBFloat:
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
    case MTL::PixelFormatASTC_12x12_HDR: return true;
    default: return false;
  }
};

static bool IsUIntFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatR8Uint:
    case MTL::PixelFormatR16Uint:
    case MTL::PixelFormatRG8Uint:
    case MTL::PixelFormatR32Uint:
    case MTL::PixelFormatRG16Uint:
    case MTL::PixelFormatRGBA8Uint:
    case MTL::PixelFormatRGB10A2Uint:
    case MTL::PixelFormatRG32Uint:
    case MTL::PixelFormatRGBA16Uint:
    case MTL::PixelFormatRGBA32Uint: return true;
    default: return false;
  }
};

static bool IsSIntFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatR8Sint:
    case MTL::PixelFormatR16Sint:
    case MTL::PixelFormatRG8Sint:
    case MTL::PixelFormatR32Sint:
    case MTL::PixelFormatRG16Sint:
    case MTL::PixelFormatRGBA8Sint:
    case MTL::PixelFormatRG32Sint:
    case MTL::PixelFormatRGBA16Sint:
    case MTL::PixelFormatRGBA32Sint: return true;
    default: return false;
  }
};

static bool IsSRGBFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatR8Unorm_sRGB:
    case MTL::PixelFormatRGBA8Unorm_sRGB:
    case MTL::PixelFormatBGRA8Unorm_sRGB:
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
    case MTL::PixelFormatASTC_12x12_sRGB:
    case MTL::PixelFormatBGRA10_XR_sRGB:
    case MTL::PixelFormatBGR10_XR_sRGB: return true;
    default: return false;
  }
};

static bool IsYUVFormat(MTL::PixelFormat mtlFormat)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422: return true;
    default: return false;
  }
}

// The shape of blocks in (a plane of) a texture format.
// Non-block, non-YUV formats are considered to have 1x1 blocks
struct BlockShape
{
  // the width of a block, in texels (1 for non-block, non-YUV formats)
  uint32_t width;

  // the height of a block, in texels (1 for non-block, non-YUV formats)
  uint32_t height;

  // the number of bytes used to encode the block
  uint32_t bytes;
};

static BlockShape GetBlockShape(MTL::PixelFormat mtlFormat, uint32_t plane)
{
  switch(mtlFormat)
  {
    case MTL::PixelFormatA8Unorm:
    case MTL::PixelFormatR8Unorm:
    case MTL::PixelFormatR8Unorm_sRGB:
    case MTL::PixelFormatR8Snorm:
    case MTL::PixelFormatR8Uint:
    case MTL::PixelFormatR8Sint:
    case MTL::PixelFormatStencil8: return {1, 1, 1};

    case MTL::PixelFormatR16Unorm:
    case MTL::PixelFormatR16Snorm:
    case MTL::PixelFormatR16Uint:
    case MTL::PixelFormatR16Sint:
    case MTL::PixelFormatR16Float:
    case MTL::PixelFormatRG8Unorm:
    case MTL::PixelFormatRG8Unorm_sRGB:
    case MTL::PixelFormatRG8Snorm:
    case MTL::PixelFormatRG8Uint:
    case MTL::PixelFormatRG8Sint:
    case MTL::PixelFormatB5G6R5Unorm:
    case MTL::PixelFormatA1BGR5Unorm:
    case MTL::PixelFormatABGR4Unorm:
    case MTL::PixelFormatBGR5A1Unorm:
    case MTL::PixelFormatDepth16Unorm: return {1, 1, 2};

    case MTL::PixelFormatR32Uint:
    case MTL::PixelFormatR32Sint:
    case MTL::PixelFormatR32Float:
    case MTL::PixelFormatRG16Unorm:
    case MTL::PixelFormatRG16Snorm:
    case MTL::PixelFormatRG16Uint:
    case MTL::PixelFormatRG16Sint:
    case MTL::PixelFormatRG16Float:
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
    case MTL::PixelFormatBGRA10_XR:
    case MTL::PixelFormatBGRA10_XR_sRGB:
    case MTL::PixelFormatBGR10_XR:
    case MTL::PixelFormatBGR10_XR_sRGB:
    case MTL::PixelFormatDepth32Float:
    case MTL::PixelFormatDepth24Unorm_Stencil8:
    case MTL::PixelFormatX24_Stencil8: return {1, 1, 4};

    case MTL::PixelFormatRG32Uint:
    case MTL::PixelFormatRG32Sint:
    case MTL::PixelFormatRG32Float:
    case MTL::PixelFormatRGBA16Unorm:
    case MTL::PixelFormatRGBA16Snorm:
    case MTL::PixelFormatRGBA16Uint:
    case MTL::PixelFormatRGBA16Sint:
    case MTL::PixelFormatRGBA16Float:
    case MTL::PixelFormatDepth32Float_Stencil8:
    case MTL::PixelFormatX32_Stencil8: return {1, 1, 8};

    case MTL::PixelFormatRGBA32Uint:
    case MTL::PixelFormatRGBA32Sint:
    case MTL::PixelFormatRGBA32Float: return {1, 1, 16};

    case MTL::PixelFormatBC1_RGBA:
    case MTL::PixelFormatBC1_RGBA_sRGB:
    case MTL::PixelFormatBC4_RUnorm:
    case MTL::PixelFormatBC4_RSnorm: return {4, 4, 8};

    case MTL::PixelFormatBC2_RGBA:
    case MTL::PixelFormatBC2_RGBA_sRGB:
    case MTL::PixelFormatBC3_RGBA:
    case MTL::PixelFormatBC3_RGBA_sRGB:
    case MTL::PixelFormatBC5_RGUnorm:
    case MTL::PixelFormatBC5_RGSnorm: return {4, 4, 16};

    case MTL::PixelFormatBC6H_RGBFloat:
    case MTL::PixelFormatBC6H_RGBUfloat:
    case MTL::PixelFormatBC7_RGBAUnorm:
    case MTL::PixelFormatBC7_RGBAUnorm_sRGB: return {4, 4, 16};

    case MTL::PixelFormatPVRTC_RGB_2BPP:
    case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_2BPP:
    case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB: return {8, 4, 8};

    case MTL::PixelFormatPVRTC_RGB_4BPP:
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTL::PixelFormatPVRTC_RGBA_4BPP:
    case MTL::PixelFormatPVRTC_RGBA_4BPP_sRGB: return {4, 4, 8};

    case MTL::PixelFormatEAC_R11Unorm:
    case MTL::PixelFormatEAC_R11Snorm: return {4, 4, 8};

    case MTL::PixelFormatEAC_RG11Unorm:
    case MTL::PixelFormatEAC_RG11Snorm: return {4, 4, 16};

    case MTL::PixelFormatEAC_RGBA8:
    case MTL::PixelFormatEAC_RGBA8_sRGB: return {4, 4, 8};

    case MTL::PixelFormatETC2_RGB8:
    case MTL::PixelFormatETC2_RGB8_sRGB:
    case MTL::PixelFormatETC2_RGB8A1:
    case MTL::PixelFormatETC2_RGB8A1_sRGB: return {4, 4, 16};

    case MTL::PixelFormatASTC_4x4_sRGB:
    case MTL::PixelFormatASTC_4x4_LDR:
    case MTL::PixelFormatASTC_4x4_HDR: return {4, 4, 16};

    case MTL::PixelFormatASTC_5x4_sRGB:
    case MTL::PixelFormatASTC_5x4_LDR:
    case MTL::PixelFormatASTC_5x4_HDR: return {5, 4, 16};

    case MTL::PixelFormatASTC_5x5_sRGB:
    case MTL::PixelFormatASTC_5x5_LDR:
    case MTL::PixelFormatASTC_5x5_HDR: return {5, 5, 16};

    case MTL::PixelFormatASTC_6x5_sRGB:
    case MTL::PixelFormatASTC_6x5_LDR:
    case MTL::PixelFormatASTC_6x5_HDR: return {6, 5, 16};

    case MTL::PixelFormatASTC_6x6_sRGB:
    case MTL::PixelFormatASTC_6x6_LDR:
    case MTL::PixelFormatASTC_6x6_HDR: return {6, 6, 16};

    case MTL::PixelFormatASTC_8x5_sRGB:
    case MTL::PixelFormatASTC_8x5_LDR:
    case MTL::PixelFormatASTC_8x5_HDR: return {8, 5, 16};

    case MTL::PixelFormatASTC_8x6_sRGB:
    case MTL::PixelFormatASTC_8x6_LDR:
    case MTL::PixelFormatASTC_8x6_HDR: return {8, 6, 16};

    case MTL::PixelFormatASTC_8x8_sRGB:
    case MTL::PixelFormatASTC_8x8_LDR:
    case MTL::PixelFormatASTC_8x8_HDR: return {8, 8, 16};

    case MTL::PixelFormatASTC_10x5_sRGB:
    case MTL::PixelFormatASTC_10x5_LDR:
    case MTL::PixelFormatASTC_10x5_HDR: return {10, 5, 16};

    case MTL::PixelFormatASTC_10x6_sRGB:
    case MTL::PixelFormatASTC_10x6_LDR:
    case MTL::PixelFormatASTC_10x6_HDR: return {10, 6, 16};

    case MTL::PixelFormatASTC_10x8_sRGB:
    case MTL::PixelFormatASTC_10x8_LDR:
    case MTL::PixelFormatASTC_10x8_HDR: return {10, 8, 16};

    case MTL::PixelFormatASTC_10x10_sRGB:
    case MTL::PixelFormatASTC_10x10_LDR:
    case MTL::PixelFormatASTC_10x10_HDR: return {10, 10, 16};

    case MTL::PixelFormatASTC_12x10_sRGB:
    case MTL::PixelFormatASTC_12x10_LDR:
    case MTL::PixelFormatASTC_12x10_HDR: return {12, 10, 16};

    case MTL::PixelFormatASTC_12x12_sRGB:
    case MTL::PixelFormatASTC_12x12_LDR:
    case MTL::PixelFormatASTC_12x12_HDR: return {12, 12, 16};

    case MTL::PixelFormatGBGR422:
    case MTL::PixelFormatBGRG422:
      // 4:2:2 packed 8-bit, so 1 byte per pixel for luma and 1 byte per pixel for chroma (2 chroma
      // samples, with 50% subsampling = 1 byte per pixel)
      return {2, 1, 4};
    case MTL::PixelFormatInvalid: return {1, 1, 1};
  }
}

static uint32_t GetPlaneByteSize(uint32_t width, uint32_t height, uint32_t depth,
                                 MTL::PixelFormat mtlFormat, uint32_t mip, uint32_t plane)
{
  uint32_t mipWidth = RDCMAX(width >> mip, 1U);
  uint32_t mipHeight = RDCMAX(height >> mip, 1U);
  uint32_t mipDepth = RDCMAX(depth >> mip, 1U);

  MTL::Size planeShape(mipWidth, mipHeight, 0);
  BlockShape blockShape(GetBlockShape(mtlFormat, plane));

  uint32_t widthInBlocks = (planeShape.width + blockShape.width - 1) / blockShape.width;
  uint32_t heightInBlocks = (planeShape.height + blockShape.height - 1) / blockShape.height;

  return blockShape.bytes * widthInBlocks * heightInBlocks * mipDepth;
}

BlendMultiplier MakeBlendMultiplier(MTL::BlendFactor blend)
{
  switch(blend)
  {
    case MTL::BlendFactorZero: return BlendMultiplier::Zero;
    case MTL::BlendFactorOne: return BlendMultiplier::One;
    case MTL::BlendFactorSourceColor: return BlendMultiplier::SrcCol;
    case MTL::BlendFactorOneMinusSourceColor: return BlendMultiplier::InvSrcCol;
    case MTL::BlendFactorDestinationColor: return BlendMultiplier::DstCol;
    case MTL::BlendFactorOneMinusDestinationColor: return BlendMultiplier::InvDstCol;
    case MTL::BlendFactorSourceAlpha: return BlendMultiplier::SrcAlpha;
    case MTL::BlendFactorOneMinusSourceAlpha: return BlendMultiplier::InvSrcAlpha;
    case MTL::BlendFactorDestinationAlpha: return BlendMultiplier::DstAlpha;
    case MTL::BlendFactorOneMinusDestinationAlpha: return BlendMultiplier::InvDstAlpha;
    case MTL::BlendFactorBlendColor: return BlendMultiplier::FactorRGB;
    case MTL::BlendFactorOneMinusBlendColor: return BlendMultiplier::InvFactorRGB;
    case MTL::BlendFactorBlendAlpha: return BlendMultiplier::FactorAlpha;
    case MTL::BlendFactorOneMinusBlendAlpha: return BlendMultiplier::InvFactorAlpha;
    case MTL::BlendFactorSourceAlphaSaturated: return BlendMultiplier::SrcAlphaSat;
    case MTL::BlendFactorSource1Color: return BlendMultiplier::Src1Col;
    case MTL::BlendFactorOneMinusSource1Color: return BlendMultiplier::InvSrc1Col;
    case MTL::BlendFactorSource1Alpha: return BlendMultiplier::Src1Alpha;
    case MTL::BlendFactorOneMinusSource1Alpha: return BlendMultiplier::InvSrc1Alpha;
  }

  return BlendMultiplier::One;
}

BlendOperation MakeBlendOp(MTL::BlendOperation op)
{
  switch(op)
  {
    case MTL::BlendOperationAdd: return BlendOperation::Add;
    case MTL::BlendOperationSubtract: return BlendOperation::Subtract;
    case MTL::BlendOperationReverseSubtract: return BlendOperation::ReversedSubtract;
    case MTL::BlendOperationMin: return BlendOperation::Minimum;
    case MTL::BlendOperationMax: return BlendOperation::Maximum;
  }

  return BlendOperation::Add;
}

byte MakeWriteMask(MTL::ColorWriteMask mask)
{
  byte ret = 0;

  if(mask & MTL::ColorWriteMaskRed)
    ret |= 0x1;
  if(mask & MTL::ColorWriteMaskGreen)
    ret |= 0x2;
  if(mask & MTL::ColorWriteMaskBlue)
    ret |= 0x4;
  if(mask & MTL::ColorWriteMaskAlpha)
    ret |= 0x8;

  return ret;
}

ResourceFormat MakeResourceFormat(MTL::PixelFormat mtlFormat)
{
  ResourceFormat ret;

  if(mtlFormat == MTL::PixelFormatInvalid)
  {
    ret.type = ResourceFormatType::Undefined;
    ret.compByteWidth = 0;
    ret.compCount = 0;
    ret.compType = CompType::Typeless;
    return ret;
  }

  struct ResourceFormatInfo
  {
    ResourceFormatType type;
    CompType compType;
    uint8_t compCount;
    uint8_t compByteWidth;
    bool bgra;
  };

  ResourceFormatInfo info;
  switch(mtlFormat)
  {
    case MTL::PixelFormatA8Unorm:
      info = {ResourceFormatType::A8, CompType::UNorm, 1, 1, false};
      break;
    case MTL::PixelFormatR8Unorm:
      info = {ResourceFormatType::Regular, CompType::UNorm, 1, 1, false};
      break;
    case MTL::PixelFormatR8Unorm_sRGB:
      info = {ResourceFormatType::Regular, CompType::UNormSRGB, 1, 1, false};
      break;
    case MTL::PixelFormatR8Snorm:
      info = {ResourceFormatType::Regular, CompType::SNorm, 1, 1, false};
      break;
    case MTL::PixelFormatR8Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 1, 1, false};
      break;
    case MTL::PixelFormatR8Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 1, 1, false};
      break;
    case MTL::PixelFormatR16Unorm:
      info = {ResourceFormatType::Regular, CompType::UNorm, 1, 2, false};
      break;
    case MTL::PixelFormatR16Snorm:
      info = {ResourceFormatType::Regular, CompType::SNorm, 1, 2, false};
      break;
    case MTL::PixelFormatR16Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 1, 2, false};
      break;
    case MTL::PixelFormatR16Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 1, 2, false};
      break;
    case MTL::PixelFormatR16Float:
      info = {ResourceFormatType::Regular, CompType::Float, 1, 2, false};
      break;
    case MTL::PixelFormatRG8Unorm:
      info = {ResourceFormatType::Regular, CompType::UNorm, 2, 1, false};
      break;
    case MTL::PixelFormatRG8Unorm_sRGB:
      info = {ResourceFormatType::Regular, CompType::UNormSRGB, 2, 1, false};
      break;
    case MTL::PixelFormatRG8Snorm:
      info = {ResourceFormatType::Regular, CompType::SNorm, 2, 1, false};
      break;
    case MTL::PixelFormatRG8Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 2, 1, false};
      break;
    case MTL::PixelFormatRG8Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 2, 1, false};
      break;
    case MTL::PixelFormatB5G6R5Unorm:
      info = {ResourceFormatType::R5G6B5, CompType::UNorm, 3, 1, true};
      break;
    case MTL::PixelFormatA1BGR5Unorm:
      info = {ResourceFormatType::R5G5B5A1, CompType::UNorm, 4, 1, true};
      break;
    case MTL::PixelFormatABGR4Unorm:
      info = {ResourceFormatType::R4G4B4A4, CompType::UNorm, 4, 1, true};
      break;
    case MTL::PixelFormatBGR5A1Unorm:
      info = {ResourceFormatType::R5G5B5A1, CompType::UNorm, 4, 1, true};
      break;
    case MTL::PixelFormatR32Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 1, 4, false};
      break;
    case MTL::PixelFormatR32Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 1, 4, false};
      break;
    case MTL::PixelFormatR32Float:
      info = {ResourceFormatType::Regular, CompType::Float, 1, 4, false};
      break;
    case MTL::PixelFormatRG16Unorm:
      info = {ResourceFormatType::Regular, CompType::UNorm, 2, 2, false};
      break;
    case MTL::PixelFormatRG16Snorm:
      info = {ResourceFormatType::Regular, CompType::SNorm, 2, 2, false};
      break;
    case MTL::PixelFormatRG16Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 2, 2, false};
      break;
    case MTL::PixelFormatRG16Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 2, 2, false};
      break;
    case MTL::PixelFormatRG16Float:
      info = {ResourceFormatType::Regular, CompType::Float, 2, 2, false};
      break;
    case MTL::PixelFormatRGBA8Unorm:
      info = {ResourceFormatType::Regular, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatRGBA8Unorm_sRGB:
      info = {ResourceFormatType::Regular, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatRGBA8Snorm:
      info = {ResourceFormatType::Regular, CompType::SNorm, 4, 1, false};
      break;
    case MTL::PixelFormatRGBA8Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 4, 1, false};
      break;
    case MTL::PixelFormatRGBA8Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 4, 1, false};
      break;
    case MTL::PixelFormatBGRA8Unorm:
      info = {ResourceFormatType::Regular, CompType::UNorm, 4, 1, true};
      break;
    case MTL::PixelFormatBGRA8Unorm_sRGB:
      info = {ResourceFormatType::Regular, CompType::UNormSRGB, 4, 1, true};
      break;
    case MTL::PixelFormatRGB10A2Unorm:
      info = {ResourceFormatType::R10G10B10A2, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatRGB10A2Uint:
      info = {ResourceFormatType::R10G10B10A2, CompType::UInt, 4, 1, false};
      break;
    case MTL::PixelFormatRG11B10Float:
      info = {ResourceFormatType::R11G11B10, CompType::Float, 3, 1, false};
      break;
    case MTL::PixelFormatRGB9E5Float:
      info = {ResourceFormatType::R9G9B9E5, CompType::Float, 3, 1, false};
      break;
    case MTL::PixelFormatBGR10A2Unorm:
      info = {ResourceFormatType::R10G10B10A2, CompType::UNorm, 4, 1, true};
      break;
    case MTL::PixelFormatRG32Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 2, 4, false};
      break;
    case MTL::PixelFormatRG32Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 2, 4, false};
      break;
    case MTL::PixelFormatRG32Float:
      info = {ResourceFormatType::Regular, CompType::Float, 2, 4, false};
      break;
    case MTL::PixelFormatRGBA16Unorm:
      info = {ResourceFormatType::Regular, CompType::UNorm, 4, 2, false};
      break;
    case MTL::PixelFormatRGBA16Snorm:
      info = {ResourceFormatType::Regular, CompType::SNorm, 4, 2, false};
      break;
    case MTL::PixelFormatRGBA16Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 4, 2, false};
      break;
    case MTL::PixelFormatRGBA16Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 4, 2, false};
      break;
    case MTL::PixelFormatRGBA16Float:
      info = {ResourceFormatType::Regular, CompType::Float, 4, 2, false};
      break;
    case MTL::PixelFormatRGBA32Uint:
      info = {ResourceFormatType::Regular, CompType::UInt, 4, 4, false};
      break;
    case MTL::PixelFormatRGBA32Sint:
      info = {ResourceFormatType::Regular, CompType::SInt, 4, 4, false};
      break;
    case MTL::PixelFormatRGBA32Float:
      info = {ResourceFormatType::Regular, CompType::Float, 4, 4, false};
      break;
    case MTL::PixelFormatBC1_RGBA:
      info = {ResourceFormatType::BC1, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatBC1_RGBA_sRGB:
      info = {ResourceFormatType::BC1, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatBC2_RGBA:
      info = {ResourceFormatType::BC2, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatBC2_RGBA_sRGB:
      info = {ResourceFormatType::BC2, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatBC3_RGBA:
      info = {ResourceFormatType::BC3, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatBC3_RGBA_sRGB:
      info = {ResourceFormatType::BC3, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatBC4_RUnorm:
      info = {ResourceFormatType::BC4, CompType::UNorm, 1, 1, false};
      break;
    case MTL::PixelFormatBC4_RSnorm:
      info = {ResourceFormatType::BC4, CompType::SNorm, 1, 1, false};
      break;
    case MTL::PixelFormatBC5_RGUnorm:
      info = {ResourceFormatType::BC5, CompType::UNorm, 2, 1, false};
      break;
    case MTL::PixelFormatBC5_RGSnorm:
      info = {ResourceFormatType::BC5, CompType::SNorm, 2, 1, false};
      break;
    case MTL::PixelFormatBC6H_RGBFloat:
      info = {ResourceFormatType::BC6, CompType::Float, 3, 1, false};
      break;
    case MTL::PixelFormatBC6H_RGBUfloat:
      info = {ResourceFormatType::BC6, CompType::UNorm, 3, 1, false};
      break;
    case MTL::PixelFormatBC7_RGBAUnorm:
      info = {ResourceFormatType::BC7, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatBC7_RGBAUnorm_sRGB:
      info = {ResourceFormatType::BC7, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGB_2BPP:
      info = {ResourceFormatType::PVRTC, CompType::UNorm, 3, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
      info = {ResourceFormatType::PVRTC, CompType::UNormSRGB, 3, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGB_4BPP:
      info = {ResourceFormatType::PVRTC, CompType::UNorm, 3, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
      info = {ResourceFormatType::PVRTC, CompType::UNormSRGB, 3, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGBA_2BPP:
      info = {ResourceFormatType::PVRTC, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB:
      info = {ResourceFormatType::PVRTC, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGBA_4BPP:
      info = {ResourceFormatType::PVRTC, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatPVRTC_RGBA_4BPP_sRGB:
      info = {ResourceFormatType::PVRTC, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatEAC_R11Unorm:
      info = {ResourceFormatType::EAC, CompType::UNorm, 1, 1, false};
      break;
    case MTL::PixelFormatEAC_R11Snorm:
      info = {ResourceFormatType::EAC, CompType::SNorm, 1, 1, false};
      break;
    case MTL::PixelFormatEAC_RG11Unorm:
      info = {ResourceFormatType::EAC, CompType::UNorm, 2, 1, false};
      break;
    case MTL::PixelFormatEAC_RG11Snorm:
      info = {ResourceFormatType::EAC, CompType::SNorm, 2, 1, false};
      break;
    case MTL::PixelFormatEAC_RGBA8:
      info = {ResourceFormatType::EAC, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatEAC_RGBA8_sRGB:
      info = {ResourceFormatType::EAC, CompType::UNormSRGB, 4, 1, false};
      break;
    case MTL::PixelFormatETC2_RGB8:
      info = {ResourceFormatType::ETC2, CompType::UNorm, 3, 1, false};
      break;
    case MTL::PixelFormatETC2_RGB8_sRGB:
      info = {ResourceFormatType::ETC2, CompType::UNormSRGB, 3, 1, false};
      break;
    case MTL::PixelFormatETC2_RGB8A1:
      info = {ResourceFormatType::ETC2, CompType::UNorm, 4, 1, false};
      break;
    case MTL::PixelFormatETC2_RGB8A1_sRGB:
      info = {ResourceFormatType::ETC2, CompType::UNormSRGB, 4, 1, false};
      break;
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
      info = {ResourceFormatType::ASTC, CompType::UNormSRGB, 4, 1, false};
      break;
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
      info = {ResourceFormatType::ASTC, CompType::UNorm, 4, 1, false};
      break;
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
      info = {ResourceFormatType::ASTC, CompType::Float, 4, 1, false};
      break;
    case MTL::PixelFormatGBGR422:
      info = {ResourceFormatType::YUV8, CompType::UNorm, 3, 1, false};
      break;
    case MTL::PixelFormatBGRG422:
      info = {ResourceFormatType::YUV8, CompType::UNorm, 3, 1, true};
      break;
    case MTL::PixelFormatDepth16Unorm:
      info = {ResourceFormatType::Regular, CompType::Depth, 1, 2, false};
      break;
    case MTL::PixelFormatDepth32Float:
      info = {ResourceFormatType::Regular, CompType::Depth, 1, 4, false};
      break;
    case MTL::PixelFormatStencil8:
      info = {ResourceFormatType::S8, CompType::Depth, 1, 1, false};
      break;
    case MTL::PixelFormatDepth24Unorm_Stencil8:
      info = {ResourceFormatType::D24S8, CompType::Depth, 2, 1, false};
      break;
    case MTL::PixelFormatDepth32Float_Stencil8:
      info = {ResourceFormatType::D32S8, CompType::Depth, 2, 1, false};
      break;
    case MTL::PixelFormatX32_Stencil8:
      info = {ResourceFormatType::S8, CompType::Depth, 1, 1, false};
      break;
    case MTL::PixelFormatX24_Stencil8:
      info = {ResourceFormatType::S8, CompType::Depth, 1, 1, false};
      break;
    case MTL::PixelFormatBGRA10_XR:
      info = {ResourceFormatType::R10G10B10A2, CompType::UNorm, 4, 1, true};
      break;
    case MTL::PixelFormatBGRA10_XR_sRGB:
      info = {ResourceFormatType::R10G10B10A2, CompType::UNormSRGB, 4, 1, true};
      break;
    case MTL::PixelFormatBGR10_XR:
      info = {ResourceFormatType::R10G10B10A2, CompType::UNorm, 3, 1, true};
      break;
    case MTL::PixelFormatBGR10_XR_sRGB:
      info = {ResourceFormatType::R10G10B10A2, CompType::UNormSRGB, 3, 1, true};
      break;
    case MTL::PixelFormatInvalid: RDCERR("Unexpected MTL::PixelFormatInvalid"); break;
  };

  ret.type = info.type;
  ret.compType = info.compType;
  ret.compCount = info.compCount;
  ret.compByteWidth = info.compByteWidth;
  ret.SetBGRAOrder(info.bgra);

  if(IsYUVFormat(mtlFormat))
  {
    ret.SetYUVPlaneCount(1);

    switch(mtlFormat)
    {
      case MTL::PixelFormatGBGR422:
      case MTL::PixelFormatBGRG422: ret.SetYUVSubsampling(422); break;
      default: RDCERR("Unexpected YUV Format MTL::PixelFormat: %d", mtlFormat);
    }
  }

  return ret;
}

uint32_t GetByteSize(uint32_t width, uint32_t height, uint32_t depth, MTL::PixelFormat mtlFormat,
                     uint32_t mip)
{
  uint32_t planeCount = 1;
  uint32_t size = 0;
  for(uint32_t p = 0; p < planeCount; p++)
    size += GetPlaneByteSize(width, height, depth, mtlFormat, mip, p);
  return size;
}
