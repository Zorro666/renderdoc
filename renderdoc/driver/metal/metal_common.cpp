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

#include "metal_common.h"
#include "metal_core.h"
#include "metal_resources.h"

#define IMPLEMENT_WRAPPED_TYPE_UNWRAP(TYPE) \
  id_##TYPE Unwrap(Wrapped##TYPE *obj) { return Unwrap<id_##TYPE>((WrappedMTLObject *)obj); }
#undef IMPLEMENT_WRAPPED_TYPE_UNWRAP

uint64_t MetalInitParams::GetSerialiseSize()
{
  // misc bytes and fixed integer members
  size_t ret = sizeof(uint64_t);

  return (uint64_t)ret;
}

bool MetalInitParams::IsSupportedVersion(uint64_t ver)
{
  if(ver == CurrentVersion)
    return true;

  return false;
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalInitParams &el)
{
}

INSTANTIATE_SERIALISE_TYPE(MetalInitParams);

BlendMultiplier MakeBlendMultiplier(MTLBlendFactor blend)
{
  switch(blend)
  {
    case MTLBlendFactorZero: return BlendMultiplier::Zero;
    case MTLBlendFactorOne: return BlendMultiplier::One;
    case MTLBlendFactorSourceColor: return BlendMultiplier::SrcCol;
    case MTLBlendFactorOneMinusSourceColor: return BlendMultiplier::InvSrcCol;
    case MTLBlendFactorDestinationColor: return BlendMultiplier::DstCol;
    case MTLBlendFactorOneMinusDestinationColor: return BlendMultiplier::InvDstCol;
    case MTLBlendFactorSourceAlpha: return BlendMultiplier::SrcAlpha;
    case MTLBlendFactorOneMinusSourceAlpha: return BlendMultiplier::InvSrcAlpha;
    case MTLBlendFactorDestinationAlpha: return BlendMultiplier::DstAlpha;
    case MTLBlendFactorOneMinusDestinationAlpha: return BlendMultiplier::InvDstAlpha;
    case MTLBlendFactorBlendColor: return BlendMultiplier::FactorRGB;
    case MTLBlendFactorOneMinusBlendColor: return BlendMultiplier::InvFactorRGB;
    case MTLBlendFactorBlendAlpha: return BlendMultiplier::FactorAlpha;
    case MTLBlendFactorOneMinusBlendAlpha: return BlendMultiplier::InvFactorAlpha;
    case MTLBlendFactorSourceAlphaSaturated: return BlendMultiplier::SrcAlphaSat;
    case MTLBlendFactorSource1Color: return BlendMultiplier::Src1Col;
    case MTLBlendFactorOneMinusSource1Color: return BlendMultiplier::InvSrc1Col;
    case MTLBlendFactorSource1Alpha: return BlendMultiplier::Src1Alpha;
    case MTLBlendFactorOneMinusSource1Alpha: return BlendMultiplier::InvSrc1Alpha;
    default: break;
  }

  return BlendMultiplier::One;
}

BlendOperation MakeBlendOp(MTLBlendOperation op)
{
  switch(op)
  {
    case MTLBlendOperationAdd: return BlendOperation::Add;
    case MTLBlendOperationSubtract: return BlendOperation::Subtract;
    case MTLBlendOperationReverseSubtract: return BlendOperation::ReversedSubtract;
    case MTLBlendOperationMin: return BlendOperation::Minimum;
    case MTLBlendOperationMax: return BlendOperation::Maximum;
    default: break;
  }

  return BlendOperation::Add;
}

byte MakeWriteMask(MTLColorWriteMask mask)
{
  byte ret = 0;

  if(mask & MTLColorWriteMaskRed)
    ret |= 0x1;
  if(mask & MTLColorWriteMaskGreen)
    ret |= 0x2;
  if(mask & MTLColorWriteMaskBlue)
    ret |= 0x4;
  if(mask & MTLColorWriteMaskAlpha)
    ret |= 0x8;

  return ret;
}

ResourceFormat MakeResourceFormat(MTLPixelFormat format)
{
  ResourceFormat ret;

  ret.type = ResourceFormatType::Regular;
  ret.compByteWidth = 0;
  ret.compCount = 0;
  ret.compType = CompType::Typeless;

  if(format == MTLPixelFormatInvalid)
  {
    ret.type = ResourceFormatType::Undefined;
    return ret;
  }

  switch(format)
  {
    case MTLPixelFormatA8Unorm: ret.type = ResourceFormatType::A8; break;
    case MTLPixelFormatB5G6R5Unorm: ret.type = ResourceFormatType::R5G6B5; break;
    case MTLPixelFormatA1BGR5Unorm:
    case MTLPixelFormatBGR5A1Unorm: ret.type = ResourceFormatType::R5G5B5A1; break;
    case MTLPixelFormatABGR4Unorm: ret.type = ResourceFormatType::R4G4B4A4; break;
    case MTLPixelFormatRGB10A2Unorm:
    case MTLPixelFormatRG11B10Float: ret.type = ResourceFormatType::R11G11B10; break;
    case MTLPixelFormatRGB9E5Float: ret.type = ResourceFormatType::R9G9B9E5; break;
    case MTLPixelFormatRGB10A2Uint:
    case MTLPixelFormatBGR10A2Unorm:
    case MTLPixelFormatBGR10_XR:
    case MTLPixelFormatBGR10_XR_sRGB:
    case MTLPixelFormatBGRA10_XR:
    case MTLPixelFormatBGRA10_XR_sRGB: ret.type = ResourceFormatType::R10G10B10A2; break;
    case MTLPixelFormatBC1_RGBA:
    case MTLPixelFormatBC1_RGBA_sRGB: ret.type = ResourceFormatType::BC1; break;
    case MTLPixelFormatBC2_RGBA:
    case MTLPixelFormatBC2_RGBA_sRGB: ret.type = ResourceFormatType::BC2; break;
    case MTLPixelFormatBC3_RGBA:
    case MTLPixelFormatBC3_RGBA_sRGB: ret.type = ResourceFormatType::BC3; break;
    case MTLPixelFormatBC4_RUnorm:
    case MTLPixelFormatBC4_RSnorm: ret.type = ResourceFormatType::BC4; break;
    case MTLPixelFormatBC5_RGUnorm:
    case MTLPixelFormatBC5_RGSnorm: ret.type = ResourceFormatType::BC5; break;
    case MTLPixelFormatBC6H_RGBFloat:
    case MTLPixelFormatBC6H_RGBUfloat: ret.type = ResourceFormatType::BC6; break;
    case MTLPixelFormatBC7_RGBAUnorm:
    case MTLPixelFormatBC7_RGBAUnorm_sRGB: ret.type = ResourceFormatType::BC7; break;
    case MTLPixelFormatPVRTC_RGB_2BPP:
    case MTLPixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGB_4BPP:
    case MTLPixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTLPixelFormatPVRTC_RGBA_2BPP:
    case MTLPixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGBA_4BPP:
    case MTLPixelFormatPVRTC_RGBA_4BPP_sRGB: ret.type = ResourceFormatType::PVRTC; break;
    case MTLPixelFormatEAC_R11Unorm:
    case MTLPixelFormatEAC_R11Snorm:
    case MTLPixelFormatEAC_RG11Unorm:
    case MTLPixelFormatEAC_RG11Snorm:
    case MTLPixelFormatEAC_RGBA8:
    case MTLPixelFormatEAC_RGBA8_sRGB: ret.type = ResourceFormatType::EAC; break;
    case MTLPixelFormatETC2_RGB8:
    case MTLPixelFormatETC2_RGB8_sRGB:
    case MTLPixelFormatETC2_RGB8A1:
    case MTLPixelFormatETC2_RGB8A1_sRGB: ret.type = ResourceFormatType::ETC2; break;
    case MTLPixelFormatASTC_4x4_sRGB:
    case MTLPixelFormatASTC_5x4_sRGB:
    case MTLPixelFormatASTC_5x5_sRGB:
    case MTLPixelFormatASTC_6x5_sRGB:
    case MTLPixelFormatASTC_6x6_sRGB:
    case MTLPixelFormatASTC_8x5_sRGB:
    case MTLPixelFormatASTC_8x6_sRGB:
    case MTLPixelFormatASTC_8x8_sRGB:
    case MTLPixelFormatASTC_10x5_sRGB:
    case MTLPixelFormatASTC_10x6_sRGB:
    case MTLPixelFormatASTC_10x8_sRGB:
    case MTLPixelFormatASTC_10x10_sRGB:
    case MTLPixelFormatASTC_12x10_sRGB:
    case MTLPixelFormatASTC_12x12_sRGB:
    case MTLPixelFormatASTC_4x4_LDR:
    case MTLPixelFormatASTC_5x4_LDR:
    case MTLPixelFormatASTC_5x5_LDR:
    case MTLPixelFormatASTC_6x5_LDR:
    case MTLPixelFormatASTC_6x6_LDR:
    case MTLPixelFormatASTC_8x5_LDR:
    case MTLPixelFormatASTC_8x6_LDR:
    case MTLPixelFormatASTC_8x8_LDR:
    case MTLPixelFormatASTC_10x5_LDR:
    case MTLPixelFormatASTC_10x6_LDR:
    case MTLPixelFormatASTC_10x8_LDR:
    case MTLPixelFormatASTC_10x10_LDR:
    case MTLPixelFormatASTC_12x10_LDR:
    case MTLPixelFormatASTC_12x12_LDR:
    case MTLPixelFormatASTC_4x4_HDR:
    case MTLPixelFormatASTC_5x4_HDR:
    case MTLPixelFormatASTC_5x5_HDR:
    case MTLPixelFormatASTC_6x5_HDR:
    case MTLPixelFormatASTC_6x6_HDR:
    case MTLPixelFormatASTC_8x5_HDR:
    case MTLPixelFormatASTC_8x6_HDR:
    case MTLPixelFormatASTC_8x8_HDR:
    case MTLPixelFormatASTC_10x5_HDR:
    case MTLPixelFormatASTC_10x6_HDR:
    case MTLPixelFormatASTC_10x8_HDR:
    case MTLPixelFormatASTC_10x10_HDR:
    case MTLPixelFormatASTC_12x10_HDR:
    case MTLPixelFormatASTC_12x12_HDR: ret.type = ResourceFormatType::ASTC; break;
    case MTLPixelFormatGBGR422:
    case MTLPixelFormatBGRG422: ret.type = ResourceFormatType::YUV16; break;
    case MTLPixelFormatDepth24Unorm_Stencil8: ret.type = ResourceFormatType::D24S8; break;
    case MTLPixelFormatDepth32Float_Stencil8: ret.type = ResourceFormatType::D32S8; break;
    case MTLPixelFormatStencil8: ret.type = ResourceFormatType::S8; break;
    case MTLPixelFormatX32_Stencil8:
    case MTLPixelFormatX24_Stencil8: ret.type = ResourceFormatType::S8; break;
    default: break;
  }

  // TODO: set the BGRA Order
  switch(format)
  {
    case MTLPixelFormatB5G6R5Unorm:
    case MTLPixelFormatA1BGR5Unorm:
    case MTLPixelFormatABGR4Unorm:
    case MTLPixelFormatBGR5A1Unorm:
    case MTLPixelFormatBGRA8Unorm:
    case MTLPixelFormatBGRA8Unorm_sRGB:
    case MTLPixelFormatBGR10A2Unorm:
    case MTLPixelFormatBGR10_XR:
    case MTLPixelFormatBGR10_XR_sRGB:
    case MTLPixelFormatBGRA10_XR:
    case MTLPixelFormatBGRA10_XR_sRGB:
    case MTLPixelFormatGBGR422:
    case MTLPixelFormatBGRG422: ret.SetBGRAOrder(true); break;
    default: break;
  }

  switch(format)
  {
    case MTLPixelFormatA8Unorm:
    case MTLPixelFormatR8Unorm:
    case MTLPixelFormatR8Unorm_sRGB:
    case MTLPixelFormatR8Snorm:
    case MTLPixelFormatR8Uint:
    case MTLPixelFormatR8Sint:
    case MTLPixelFormatR16Unorm:
    case MTLPixelFormatR16Snorm:
    case MTLPixelFormatR16Uint:
    case MTLPixelFormatR16Sint:
    case MTLPixelFormatR16Float:
    case MTLPixelFormatR32Uint:
    case MTLPixelFormatR32Sint:
    case MTLPixelFormatR32Float:
    case MTLPixelFormatEAC_R11Unorm:
    case MTLPixelFormatEAC_R11Snorm:
    case MTLPixelFormatBC4_RUnorm:
    case MTLPixelFormatBC4_RSnorm:
    case MTLPixelFormatDepth16Unorm:
    case MTLPixelFormatDepth32Float:
    case MTLPixelFormatStencil8:
    case MTLPixelFormatX32_Stencil8:
    case MTLPixelFormatX24_Stencil8: ret.compCount = 1; break;
    case MTLPixelFormatRG8Unorm:
    case MTLPixelFormatRG8Unorm_sRGB:
    case MTLPixelFormatRG8Snorm:
    case MTLPixelFormatRG8Uint:
    case MTLPixelFormatRG8Sint:
    case MTLPixelFormatRG16Unorm:
    case MTLPixelFormatRG16Snorm:
    case MTLPixelFormatRG16Uint:
    case MTLPixelFormatRG16Sint:
    case MTLPixelFormatRG16Float:
    case MTLPixelFormatRG32Uint:
    case MTLPixelFormatRG32Sint:
    case MTLPixelFormatRG32Float:
    case MTLPixelFormatBC5_RGUnorm:
    case MTLPixelFormatBC5_RGSnorm:
    case MTLPixelFormatEAC_RG11Unorm:
    case MTLPixelFormatEAC_RG11Snorm:
    case MTLPixelFormatGBGR422:
    case MTLPixelFormatBGRG422:
    case MTLPixelFormatDepth24Unorm_Stencil8:
    case MTLPixelFormatDepth32Float_Stencil8: ret.compCount = 2; break;
    case MTLPixelFormatB5G6R5Unorm:
    case MTLPixelFormatRG11B10Float:
    case MTLPixelFormatRGB9E5Float:
    case MTLPixelFormatBGR10_XR:
    case MTLPixelFormatBGR10_XR_sRGB:
    case MTLPixelFormatBC6H_RGBFloat:
    case MTLPixelFormatBC6H_RGBUfloat:
    case MTLPixelFormatPVRTC_RGB_2BPP:
    case MTLPixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGB_4BPP:
    case MTLPixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTLPixelFormatETC2_RGB8:
    case MTLPixelFormatETC2_RGB8_sRGB: ret.compCount = 3; break;
    case MTLPixelFormatA1BGR5Unorm:
    case MTLPixelFormatABGR4Unorm:
    case MTLPixelFormatBGR5A1Unorm:
    case MTLPixelFormatRGBA8Unorm:
    case MTLPixelFormatRGBA8Unorm_sRGB:
    case MTLPixelFormatRGBA8Snorm:
    case MTLPixelFormatRGBA8Uint:
    case MTLPixelFormatRGBA8Sint:
    case MTLPixelFormatBGRA8Unorm:
    case MTLPixelFormatBGRA8Unorm_sRGB:
    case MTLPixelFormatRGB10A2Unorm:
    case MTLPixelFormatRGB10A2Uint:
    case MTLPixelFormatBGR10A2Unorm:
    case MTLPixelFormatRGBA16Unorm:
    case MTLPixelFormatRGBA16Snorm:
    case MTLPixelFormatRGBA16Uint:
    case MTLPixelFormatRGBA16Sint:
    case MTLPixelFormatRGBA16Float:
    case MTLPixelFormatBGRA10_XR:
    case MTLPixelFormatBGRA10_XR_sRGB:
    case MTLPixelFormatRGBA32Uint:
    case MTLPixelFormatRGBA32Sint:
    case MTLPixelFormatRGBA32Float:
    case MTLPixelFormatBC1_RGBA:
    case MTLPixelFormatBC1_RGBA_sRGB:
    case MTLPixelFormatBC2_RGBA:
    case MTLPixelFormatBC2_RGBA_sRGB:
    case MTLPixelFormatBC3_RGBA:
    case MTLPixelFormatBC3_RGBA_sRGB:
    case MTLPixelFormatBC7_RGBAUnorm:
    case MTLPixelFormatBC7_RGBAUnorm_sRGB:
    case MTLPixelFormatPVRTC_RGBA_2BPP:
    case MTLPixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGBA_4BPP:
    case MTLPixelFormatPVRTC_RGBA_4BPP_sRGB:
    case MTLPixelFormatEAC_RGBA8:
    case MTLPixelFormatEAC_RGBA8_sRGB:
    case MTLPixelFormatETC2_RGB8A1:
    case MTLPixelFormatETC2_RGB8A1_sRGB:
    case MTLPixelFormatASTC_4x4_sRGB:
    case MTLPixelFormatASTC_5x4_sRGB:
    case MTLPixelFormatASTC_5x5_sRGB:
    case MTLPixelFormatASTC_6x5_sRGB:
    case MTLPixelFormatASTC_6x6_sRGB:
    case MTLPixelFormatASTC_8x5_sRGB:
    case MTLPixelFormatASTC_8x6_sRGB:
    case MTLPixelFormatASTC_8x8_sRGB:
    case MTLPixelFormatASTC_10x5_sRGB:
    case MTLPixelFormatASTC_10x6_sRGB:
    case MTLPixelFormatASTC_10x8_sRGB:
    case MTLPixelFormatASTC_10x10_sRGB:
    case MTLPixelFormatASTC_12x10_sRGB:
    case MTLPixelFormatASTC_12x12_sRGB:
    case MTLPixelFormatASTC_4x4_LDR:
    case MTLPixelFormatASTC_5x4_LDR:
    case MTLPixelFormatASTC_5x5_LDR:
    case MTLPixelFormatASTC_6x5_LDR:
    case MTLPixelFormatASTC_6x6_LDR:
    case MTLPixelFormatASTC_8x5_LDR:
    case MTLPixelFormatASTC_8x6_LDR:
    case MTLPixelFormatASTC_8x8_LDR:
    case MTLPixelFormatASTC_10x5_LDR:
    case MTLPixelFormatASTC_10x6_LDR:
    case MTLPixelFormatASTC_10x8_LDR:
    case MTLPixelFormatASTC_10x10_LDR:
    case MTLPixelFormatASTC_12x10_LDR:
    case MTLPixelFormatASTC_12x12_LDR:
    case MTLPixelFormatASTC_4x4_HDR:
    case MTLPixelFormatASTC_5x4_HDR:
    case MTLPixelFormatASTC_5x5_HDR:
    case MTLPixelFormatASTC_6x5_HDR:
    case MTLPixelFormatASTC_6x6_HDR:
    case MTLPixelFormatASTC_8x5_HDR:
    case MTLPixelFormatASTC_8x6_HDR:
    case MTLPixelFormatASTC_8x8_HDR:
    case MTLPixelFormatASTC_10x5_HDR:
    case MTLPixelFormatASTC_10x6_HDR:
    case MTLPixelFormatASTC_10x8_HDR:
    case MTLPixelFormatASTC_10x10_HDR:
    case MTLPixelFormatASTC_12x10_HDR:
    case MTLPixelFormatASTC_12x12_HDR: ret.compCount = 4; break;
    case MTLPixelFormatInvalid: ret.compCount = 1; break;
  }

  // TODO: complete setting compType
  switch(format)
  {
    case MTLPixelFormatA8Unorm:
    case MTLPixelFormatR8Unorm:
    case MTLPixelFormatR16Unorm:
    case MTLPixelFormatRG8Unorm:
    case MTLPixelFormatB5G6R5Unorm:
    case MTLPixelFormatA1BGR5Unorm:
    case MTLPixelFormatABGR4Unorm:
    case MTLPixelFormatBGR5A1Unorm:
    case MTLPixelFormatRG16Unorm:
    case MTLPixelFormatRGBA8Unorm:
    case MTLPixelFormatBGRA8Unorm:
    case MTLPixelFormatRGB10A2Unorm:
    case MTLPixelFormatBGR10A2Unorm:
    case MTLPixelFormatBGR10_XR:
    case MTLPixelFormatRGBA16Unorm:
    case MTLPixelFormatBGRA10_XR:
    case MTLPixelFormatBC1_RGBA:
    case MTLPixelFormatBC2_RGBA:
    case MTLPixelFormatBC3_RGBA:
    case MTLPixelFormatBC4_RUnorm:
    case MTLPixelFormatBC5_RGUnorm:
    case MTLPixelFormatBC7_RGBAUnorm:
    case MTLPixelFormatPVRTC_RGB_2BPP:
    case MTLPixelFormatPVRTC_RGB_4BPP:
    case MTLPixelFormatPVRTC_RGBA_2BPP:
    case MTLPixelFormatPVRTC_RGBA_4BPP:
    case MTLPixelFormatEAC_R11Unorm:
    case MTLPixelFormatEAC_RG11Unorm:
    case MTLPixelFormatEAC_RGBA8:
    case MTLPixelFormatETC2_RGB8:
    case MTLPixelFormatETC2_RGB8A1:
    case MTLPixelFormatASTC_4x4_LDR:
    case MTLPixelFormatASTC_5x4_LDR:
    case MTLPixelFormatASTC_5x5_LDR:
    case MTLPixelFormatASTC_6x5_LDR:
    case MTLPixelFormatASTC_6x6_LDR:
    case MTLPixelFormatASTC_8x5_LDR:
    case MTLPixelFormatASTC_8x6_LDR:
    case MTLPixelFormatASTC_8x8_LDR:
    case MTLPixelFormatASTC_10x5_LDR:
    case MTLPixelFormatASTC_10x6_LDR:
    case MTLPixelFormatASTC_10x8_LDR:
    case MTLPixelFormatASTC_10x10_LDR:
    case MTLPixelFormatASTC_12x10_LDR:
    case MTLPixelFormatASTC_12x12_LDR:
    case MTLPixelFormatASTC_4x4_HDR:
    case MTLPixelFormatASTC_5x4_HDR:
    case MTLPixelFormatASTC_5x5_HDR:
    case MTLPixelFormatASTC_6x5_HDR:
    case MTLPixelFormatASTC_6x6_HDR:
    case MTLPixelFormatASTC_8x5_HDR:
    case MTLPixelFormatASTC_8x6_HDR:
    case MTLPixelFormatASTC_8x8_HDR:
    case MTLPixelFormatASTC_10x5_HDR:
    case MTLPixelFormatASTC_10x6_HDR:
    case MTLPixelFormatASTC_10x8_HDR:
    case MTLPixelFormatASTC_10x10_HDR:
    case MTLPixelFormatASTC_12x10_HDR:
    case MTLPixelFormatASTC_12x12_HDR:
    case MTLPixelFormatGBGR422:
    case MTLPixelFormatBGRG422: ret.compType = CompType::UNorm; break;
    case MTLPixelFormatR8Unorm_sRGB:
    case MTLPixelFormatRG8Unorm_sRGB:
    case MTLPixelFormatRGBA8Unorm_sRGB:
    case MTLPixelFormatBGRA8Unorm_sRGB:
    case MTLPixelFormatBGR10_XR_sRGB:
    case MTLPixelFormatBGRA10_XR_sRGB:
    case MTLPixelFormatBC1_RGBA_sRGB:
    case MTLPixelFormatBC2_RGBA_sRGB:
    case MTLPixelFormatBC3_RGBA_sRGB:
    case MTLPixelFormatBC7_RGBAUnorm_sRGB:
    case MTLPixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTLPixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGBA_4BPP_sRGB:
    case MTLPixelFormatEAC_RGBA8_sRGB:
    case MTLPixelFormatETC2_RGB8_sRGB:
    case MTLPixelFormatETC2_RGB8A1_sRGB:
    case MTLPixelFormatASTC_4x4_sRGB:
    case MTLPixelFormatASTC_5x4_sRGB:
    case MTLPixelFormatASTC_5x5_sRGB:
    case MTLPixelFormatASTC_6x5_sRGB:
    case MTLPixelFormatASTC_6x6_sRGB:
    case MTLPixelFormatASTC_8x5_sRGB:
    case MTLPixelFormatASTC_8x6_sRGB:
    case MTLPixelFormatASTC_8x8_sRGB:
    case MTLPixelFormatASTC_10x5_sRGB:
    case MTLPixelFormatASTC_10x6_sRGB:
    case MTLPixelFormatASTC_10x8_sRGB:
    case MTLPixelFormatASTC_10x10_sRGB:
    case MTLPixelFormatASTC_12x10_sRGB:
    case MTLPixelFormatASTC_12x12_sRGB: ret.compType = CompType::UNormSRGB; break;
    case MTLPixelFormatR8Snorm:
    case MTLPixelFormatR16Snorm:
    case MTLPixelFormatRG8Snorm:
    case MTLPixelFormatRG16Snorm:
    case MTLPixelFormatRGBA8Snorm:
    case MTLPixelFormatRGBA16Snorm:
    case MTLPixelFormatBC4_RSnorm:
    case MTLPixelFormatBC5_RGSnorm:
    case MTLPixelFormatEAC_R11Snorm:
    case MTLPixelFormatEAC_RG11Snorm: ret.compType = CompType::SNorm; break;
    case MTLPixelFormatR8Uint:
    case MTLPixelFormatR16Uint:
    case MTLPixelFormatRG8Uint:
    case MTLPixelFormatR32Uint:
    case MTLPixelFormatRG16Uint:
    case MTLPixelFormatRGBA8Uint:
    case MTLPixelFormatRGB10A2Uint:
    case MTLPixelFormatRG32Uint:
    case MTLPixelFormatRGBA16Uint:
    case MTLPixelFormatRGBA32Uint: ret.compType = CompType::UInt; break;
    case MTLPixelFormatR8Sint:
    case MTLPixelFormatR16Sint:
    case MTLPixelFormatRG8Sint:
    case MTLPixelFormatR32Sint:
    case MTLPixelFormatRG16Sint:
    case MTLPixelFormatRGBA8Sint:
    case MTLPixelFormatRG32Sint:
    case MTLPixelFormatRGBA16Sint:
    case MTLPixelFormatRGBA32Sint: ret.compType = CompType::SInt; break;
    case MTLPixelFormatR16Float:
    case MTLPixelFormatR32Float:
    case MTLPixelFormatRG16Float:
    case MTLPixelFormatRG11B10Float:
    case MTLPixelFormatRGB9E5Float:
    case MTLPixelFormatRG32Float:
    case MTLPixelFormatRGBA16Float:
    case MTLPixelFormatRGBA32Float:
    case MTLPixelFormatBC6H_RGBFloat:
    case MTLPixelFormatBC6H_RGBUfloat: ret.compType = CompType::Float; break;
    case MTLPixelFormatDepth16Unorm:
    case MTLPixelFormatDepth32Float:
    case MTLPixelFormatStencil8:
    case MTLPixelFormatDepth24Unorm_Stencil8:
    case MTLPixelFormatDepth32Float_Stencil8:
    case MTLPixelFormatX32_Stencil8:
    case MTLPixelFormatX24_Stencil8: ret.compType = CompType::Depth; break;
    case MTLPixelFormatInvalid: ret.compType = CompType::Typeless; break;
  }

  switch(format)
  {
    case MTLPixelFormatA8Unorm:
    case MTLPixelFormatR8Unorm:
    case MTLPixelFormatR8Unorm_sRGB:
    case MTLPixelFormatR8Snorm:
    case MTLPixelFormatR8Uint:
    case MTLPixelFormatR8Sint:
    case MTLPixelFormatRG8Unorm:
    case MTLPixelFormatRG8Unorm_sRGB:
    case MTLPixelFormatRG8Snorm:
    case MTLPixelFormatRG8Uint:
    case MTLPixelFormatRG8Sint:
    case MTLPixelFormatRGBA8Unorm:
    case MTLPixelFormatRGBA8Unorm_sRGB:
    case MTLPixelFormatRGBA8Snorm:
    case MTLPixelFormatRGBA8Uint:
    case MTLPixelFormatRGBA8Sint:
    case MTLPixelFormatBGRA8Unorm:
    case MTLPixelFormatBGRA8Unorm_sRGB:
    case MTLPixelFormatRGB10A2Unorm:
    case MTLPixelFormatRGB10A2Uint:
    case MTLPixelFormatRG11B10Float:
    case MTLPixelFormatRGB9E5Float:
    case MTLPixelFormatBGR10A2Unorm:
    case MTLPixelFormatBGR10_XR:
    case MTLPixelFormatBGR10_XR_sRGB:
    case MTLPixelFormatBGRA10_XR:
    case MTLPixelFormatBGRA10_XR_sRGB:
    case MTLPixelFormatStencil8:
    case MTLPixelFormatDepth32Float_Stencil8:
    case MTLPixelFormatX32_Stencil8:
    case MTLPixelFormatX24_Stencil8: ret.compByteWidth = 1; break;
    case MTLPixelFormatR16Unorm:
    case MTLPixelFormatR16Snorm:
    case MTLPixelFormatR16Uint:
    case MTLPixelFormatR16Sint:
    case MTLPixelFormatR16Float:
    case MTLPixelFormatB5G6R5Unorm:
    case MTLPixelFormatA1BGR5Unorm:
    case MTLPixelFormatABGR4Unorm:
    case MTLPixelFormatBGR5A1Unorm:
    case MTLPixelFormatRG16Unorm:
    case MTLPixelFormatRG16Snorm:
    case MTLPixelFormatRG16Uint:
    case MTLPixelFormatRG16Sint:
    case MTLPixelFormatRG16Float:
    case MTLPixelFormatRGBA16Unorm:
    case MTLPixelFormatRGBA16Snorm:
    case MTLPixelFormatRGBA16Uint:
    case MTLPixelFormatRGBA16Sint:
    case MTLPixelFormatRGBA16Float:
    case MTLPixelFormatGBGR422:
    case MTLPixelFormatBGRG422:
    case MTLPixelFormatDepth16Unorm: ret.compByteWidth = 2; break;

    case MTLPixelFormatDepth24Unorm_Stencil8: ret.compByteWidth = 3; break;

    case MTLPixelFormatR32Uint:
    case MTLPixelFormatR32Sint:
    case MTLPixelFormatR32Float:
    case MTLPixelFormatRG32Uint:
    case MTLPixelFormatRG32Sint:
    case MTLPixelFormatRG32Float:
    case MTLPixelFormatRGBA32Uint:
    case MTLPixelFormatRGBA32Sint:
    case MTLPixelFormatRGBA32Float:
    case MTLPixelFormatDepth32Float: ret.compByteWidth = 4; break;
    case MTLPixelFormatBC1_RGBA:
    case MTLPixelFormatBC1_RGBA_sRGB:
    case MTLPixelFormatBC2_RGBA:
    case MTLPixelFormatBC2_RGBA_sRGB:
    case MTLPixelFormatBC3_RGBA:
    case MTLPixelFormatBC3_RGBA_sRGB:
    case MTLPixelFormatBC4_RUnorm:
    case MTLPixelFormatBC4_RSnorm:
    case MTLPixelFormatBC5_RGUnorm:
    case MTLPixelFormatBC5_RGSnorm:
    case MTLPixelFormatBC6H_RGBFloat:
    case MTLPixelFormatBC6H_RGBUfloat:
    case MTLPixelFormatBC7_RGBAUnorm:
    case MTLPixelFormatBC7_RGBAUnorm_sRGB:
    case MTLPixelFormatPVRTC_RGB_2BPP:
    case MTLPixelFormatPVRTC_RGB_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGB_4BPP:
    case MTLPixelFormatPVRTC_RGB_4BPP_sRGB:
    case MTLPixelFormatPVRTC_RGBA_2BPP:
    case MTLPixelFormatPVRTC_RGBA_2BPP_sRGB:
    case MTLPixelFormatPVRTC_RGBA_4BPP:
    case MTLPixelFormatPVRTC_RGBA_4BPP_sRGB:
    case MTLPixelFormatEAC_R11Unorm:
    case MTLPixelFormatEAC_R11Snorm:
    case MTLPixelFormatEAC_RG11Unorm:
    case MTLPixelFormatEAC_RG11Snorm:
    case MTLPixelFormatEAC_RGBA8:
    case MTLPixelFormatEAC_RGBA8_sRGB:
    case MTLPixelFormatETC2_RGB8:
    case MTLPixelFormatETC2_RGB8_sRGB:
    case MTLPixelFormatETC2_RGB8A1:
    case MTLPixelFormatETC2_RGB8A1_sRGB:
    case MTLPixelFormatASTC_4x4_sRGB:
    case MTLPixelFormatASTC_5x4_sRGB:
    case MTLPixelFormatASTC_5x5_sRGB:
    case MTLPixelFormatASTC_6x5_sRGB:
    case MTLPixelFormatASTC_6x6_sRGB:
    case MTLPixelFormatASTC_8x5_sRGB:
    case MTLPixelFormatASTC_8x6_sRGB:
    case MTLPixelFormatASTC_8x8_sRGB:
    case MTLPixelFormatASTC_10x5_sRGB:
    case MTLPixelFormatASTC_10x6_sRGB:
    case MTLPixelFormatASTC_10x8_sRGB:
    case MTLPixelFormatASTC_10x10_sRGB:
    case MTLPixelFormatASTC_12x10_sRGB:
    case MTLPixelFormatASTC_12x12_sRGB:
    case MTLPixelFormatASTC_4x4_LDR:
    case MTLPixelFormatASTC_5x4_LDR:
    case MTLPixelFormatASTC_5x5_LDR:
    case MTLPixelFormatASTC_6x5_LDR:
    case MTLPixelFormatASTC_6x6_LDR:
    case MTLPixelFormatASTC_8x5_LDR:
    case MTLPixelFormatASTC_8x6_LDR:
    case MTLPixelFormatASTC_8x8_LDR:
    case MTLPixelFormatASTC_10x5_LDR:
    case MTLPixelFormatASTC_10x6_LDR:
    case MTLPixelFormatASTC_10x8_LDR:
    case MTLPixelFormatASTC_10x10_LDR:
    case MTLPixelFormatASTC_12x10_LDR:
    case MTLPixelFormatASTC_12x12_LDR:
    case MTLPixelFormatASTC_4x4_HDR:
    case MTLPixelFormatASTC_5x4_HDR:
    case MTLPixelFormatASTC_5x5_HDR:
    case MTLPixelFormatASTC_6x5_HDR:
    case MTLPixelFormatASTC_6x6_HDR:
    case MTLPixelFormatASTC_8x5_HDR:
    case MTLPixelFormatASTC_8x6_HDR:
    case MTLPixelFormatASTC_8x8_HDR:
    case MTLPixelFormatASTC_10x5_HDR:
    case MTLPixelFormatASTC_10x6_HDR:
    case MTLPixelFormatASTC_10x8_HDR:
    case MTLPixelFormatASTC_10x10_HDR:
    case MTLPixelFormatASTC_12x10_HDR:
    case MTLPixelFormatASTC_12x12_HDR: ret.compByteWidth = 1; break;
    case MTLPixelFormatInvalid: ret.compByteWidth = 1; break;
  }

  // TODO: YUV format support
  //  if(IsYUVFormat(format))
  //  {
  //    ret.SetYUVPlaneCount(GetYUVPlaneCount(fmt));
  //
  //    switch(fmt)
  //    {
  //      case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
  //      case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
  //      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
  //      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
  //      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
  //      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
  //      case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
  //      case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: ret.SetYUVSubsampling(420); break;
  //      case VK_FORMAT_G8B8G8R8_422_UNORM:
  //      case VK_FORMAT_B8G8R8G8_422_UNORM:
  //      case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
  //      case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
  //      case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
  //      case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
  //      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
  //      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
  //      case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
  //      case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
  //      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
  //      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
  //      case VK_FORMAT_G16B16G16R16_422_UNORM:
  //      case VK_FORMAT_B16G16R16G16_422_UNORM:
  //      case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
  //      case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: ret.SetYUVSubsampling(422); break;
  //      case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
  //      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
  //      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
  //      case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
  //      case VK_FORMAT_R10X6_UNORM_PACK16:
  //      case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
  //      case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
  //      case VK_FORMAT_R12X4_UNORM_PACK16:
  //      case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
  //      case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16: ret.SetYUVSubsampling(444); break;
  //      default: break;
  //    }
  //  }

  return ret;
}

// TODO: YUV format support
uint32_t GetYUVPlaneCount(MTLPixelFormat format)
{
  switch(format)
  {
    default: break;
  }

  return 1;
}

// TODO: YUV support
MTLSize GetPlaneShape(uint32_t width, uint32_t height, MTLPixelFormat format, uint32_t plane)
{
  switch(format)
  {
    default: return {width, height};
  }
}

// The shape of blocks in (a plane of) an image format.
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
BlockShape GetBlockShape(MTLPixelFormat format, uint32_t plane)
{
  switch(format)
  {
    case MTLPixelFormatBGRA8Unorm:
    case MTLPixelFormatRGBA8Unorm:
    case MTLPixelFormatRGBA8Unorm_sRGB:
    case MTLPixelFormatRGBA8Snorm:
    case MTLPixelFormatRGBA8Uint: return {1, 1, 4};

    default: RDCERR("Unrecognised MTLPixelFormat: %d", format);
  }
  return {1, 1, 1};
}

uint32_t GetPlaneByteSize(uint32_t width, uint32_t height, uint32_t depth, MTLPixelFormat format,
                          uint32_t mip, uint32_t plane)
{
  uint32_t mipWidth = RDCMAX(width >> mip, 1U);
  uint32_t mipHeight = RDCMAX(height >> mip, 1U);
  uint32_t mipDepth = RDCMAX(depth >> mip, 1U);

  MTLSize planeShape = GetPlaneShape(mipWidth, mipHeight, format, plane);

  BlockShape blockShape = GetBlockShape(format, plane);

  uint32_t widthInBlocks = (planeShape.width + blockShape.width - 1) / blockShape.width;
  uint32_t heightInBlocks = (planeShape.height + blockShape.height - 1) / blockShape.height;

  return blockShape.bytes * widthInBlocks * heightInBlocks * mipDepth;
}

uint32_t GetByteSize(uint32_t width, uint32_t height, uint32_t depth, MTLPixelFormat format,
                     uint32_t mip)
{
  uint32_t planeCount = GetYUVPlaneCount(format);
  uint32_t size = 0;
  for(uint32_t p = 0; p < planeCount; p++)
    size += GetPlaneByteSize(width, height, depth, format, mip, p);
  return size;
}
