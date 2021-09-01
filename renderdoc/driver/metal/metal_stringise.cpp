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
#include "metal_render_command_encoder.h"

template <>
rdcstr DoStringise(const MetalChunk &el)
{
  RDCCOMPILE_ASSERT((uint32_t)MetalChunk::Max == 1020, "Chunks changed without updating names");

  BEGIN_ENUM_STRINGISE(MetalChunk)
  {
    STRINGISE_ENUM_CLASS(MTLCreateSystemDefaultDevice);
    STRINGISE_ENUM_CLASS_NAMED(MTLBuffer_contents, "MTLBuffer::contents");
    STRINGISE_ENUM_CLASS_NAMED(MTLDevice_newBuffer, "MTLDevice::newBuffer");
    STRINGISE_ENUM_CLASS_NAMED(MTLDevice_newBufferWithLength, "MTLDevice::newBufferWithLength");
    STRINGISE_ENUM_CLASS_NAMED(MTLDevice_newCommandQueue, "MTLDevice::newCommandQueue");
    STRINGISE_ENUM_CLASS_NAMED(MTLDevice_newDefaultLibrary, "MTLDevice::newDefaultLibrary");
    STRINGISE_ENUM_CLASS_NAMED(MTLDevice_newLibraryWithSource,
                               "MTLDevice::newDefaultLibraryWithSource");
    STRINGISE_ENUM_CLASS_NAMED(MTLDevice_newRenderPipelineStateWithDescriptor,
                               "MTLDevice::newRenderPipelineStateWithDescriptor");
    STRINGISE_ENUM_CLASS_NAMED(MTLDevice_newTextureWithDescriptor,
                               "MTLDevice::newTextureWithDescriptor");
    STRINGISE_ENUM_CLASS_NAMED(MTLCommandBuffer_commit, "MTLCommandBuffer::commit");
    STRINGISE_ENUM_CLASS_NAMED(MTLCommandBuffer_presentDrawable,
                               "MTLCommandBuffer::presentDrawable");
    STRINGISE_ENUM_CLASS_NAMED(MTLCommandBuffer_renderCommandEncoderWithDescriptor,
                               "MTLCommandBuffer::renderCommandEncoderWithDescriptor");
    STRINGISE_ENUM_CLASS_NAMED(MTLCommandQueue_commandBuffer, "MTLCommandQueue::commandBuffer");
    STRINGISE_ENUM_CLASS_NAMED(MTLLibrary_newFunctionWithName, "MTLLibrary::newFunctionWithName");
    STRINGISE_ENUM_CLASS_NAMED(MTLRenderCommandEncoder_drawPrimitives,
                               "MTLRenderCommandEncoder::drawPrimitives");
    STRINGISE_ENUM_CLASS_NAMED(MTLRenderCommandEncoder_endEncoding,
                               "MTLRenderCommandEncoder::endEncoding");
    STRINGISE_ENUM_CLASS_NAMED(MTLRenderCommandEncoder_setRenderPipelineState,
                               "MTLRenderCommandEncoder::setRenderPipelineState");
    STRINGISE_ENUM_CLASS_NAMED(MTLRenderCommandEncoder_setFragmentBuffer,
                               "MTLRenderCommandEncoder::setFragmentBuffer");
    STRINGISE_ENUM_CLASS_NAMED(MTLRenderCommandEncoder_setVertexBuffer,
                               "MTLRenderCommandEncoder::setVertexBuffer");
    STRINGISE_ENUM_CLASS_NAMED(MTLRenderCommandEncoder_setFragmentTexture,
                               "MTLRenderCommandEncoder::setFragmentTexture");
    STRINGISE_ENUM_CLASS_NAMED(Max, "Max Chunk");
  }
  END_ENUM_STRINGISE()
}

#define STRINGISE_ENUM_CAST(TYPE, NAME) STRINGISE_ENUM_NAMED((TYPE)NAME, #NAME);

// MTLRenderCommandEncoder.h
template <>
rdcstr DoStringise(const MTLPrimitiveType_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLPrimitiveType_objc)
  {
    STRINGISE_ENUM_CAST(MTLPrimitiveType_objc, MTLPrimitiveTypePoint);
    STRINGISE_ENUM_CAST(MTLPrimitiveType_objc, MTLPrimitiveTypeLine);
    STRINGISE_ENUM_CAST(MTLPrimitiveType_objc, MTLPrimitiveTypeLineStrip);
    STRINGISE_ENUM_CAST(MTLPrimitiveType_objc, MTLPrimitiveTypeTriangle);
    STRINGISE_ENUM_CAST(MTLPrimitiveType_objc, MTLPrimitiveTypeTriangleStrip);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLPixelFormat_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLPixelFormat_objc)
  {
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatInvalid)

    /* Normal 8 bit formats */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatA8Unorm);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR8Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR8Unorm_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR8Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR8Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR8Sint);

    /* Normal 16 bit formats */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR16Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR16Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR16Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR16Sint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR16Float);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG8Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG8Unorm_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG8Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG8Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG8Sint);

    /* Packed 16 bit formats */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatB5G6R5Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatA1BGR5Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatABGR4Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGR5A1Unorm);

    /* Normal 32 bit formats */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR32Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR32Sint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatR32Float);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG16Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG16Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG16Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG16Sint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG16Float);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA8Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA8Unorm_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA8Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA8Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA8Sint);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGRA8Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGRA8Unorm_sRGB);

    /* Packed 32 bit formats */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGB10A2Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGB10A2Uint);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG11B10Float);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGB9E5Float);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGR10A2Unorm);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGR10_XR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGR10_XR_sRGB);

    /* Normal 64 bit formats */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG32Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG32Sint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRG32Float);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA16Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA16Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA16Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA16Sint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA16Float);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGRA10_XR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGRA10_XR_sRGB);

    /* Normal 128 bit formats */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA32Uint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA32Sint);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatRGBA32Float);

    /* Compressed formats. */

    /* S3TC/DXT */
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC1_RGBA);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC1_RGBA_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC2_RGBA);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC2_RGBA_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC3_RGBA);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC3_RGBA_sRGB);

    /* RGTC */
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC4_RUnorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC4_RSnorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC5_RGUnorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC5_RGSnorm);

    /* BPTC */
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC6H_RGBFloat);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC6H_RGBUfloat);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC7_RGBAUnorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBC7_RGBAUnorm_sRGB);

    /* PVRTC */
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGB_2BPP);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGB_2BPP_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGB_4BPP);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGB_4BPP_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGBA_2BPP);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGBA_2BPP_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGBA_4BPP);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatPVRTC_RGBA_4BPP_sRGB);

    /* ETC2 */
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatEAC_R11Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatEAC_R11Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatEAC_RG11Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatEAC_RG11Snorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatEAC_RGBA8);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatEAC_RGBA8_sRGB);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatETC2_RGB8);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatETC2_RGB8_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatETC2_RGB8A1);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatETC2_RGB8A1_sRGB);

    /* ASTC */
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_4x4_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_5x4_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_5x5_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_6x5_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_6x6_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x5_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x6_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x8_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x5_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x6_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x8_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x10_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_12x10_sRGB);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_12x12_sRGB);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_4x4_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_5x4_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_5x5_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_6x5_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_6x6_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x5_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x6_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x8_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x5_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x6_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x8_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x10_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_12x10_LDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_12x12_LDR);

    // ASTC HDR (High Dynamic Range) Formats
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_4x4_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_5x4_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_5x5_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_6x5_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_6x6_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x5_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x6_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_8x8_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x5_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x6_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x8_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_10x10_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_12x10_HDR);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatASTC_12x12_HDR);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatGBGR422);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatBGRG422);

    /* Depth */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatDepth16Unorm);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatDepth32Float);

    /* Stencil */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatStencil8);

    /* Depth Stencil */

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatDepth24Unorm_Stencil8);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatDepth32Float_Stencil8);

    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatX32_Stencil8);
    STRINGISE_ENUM_CAST(MTLPixelFormat_objc, MTLPixelFormatX24_Stencil8);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLPrimitiveTopologyClass_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLPrimitiveTopologyClass_objc)
  {
    STRINGISE_ENUM_CAST(MTLPrimitiveTopologyClass_objc, MTLPrimitiveTopologyClassUnspecified);
    STRINGISE_ENUM_CAST(MTLPrimitiveTopologyClass_objc, MTLPrimitiveTopologyClassPoint);
    STRINGISE_ENUM_CAST(MTLPrimitiveTopologyClass_objc, MTLPrimitiveTopologyClassLine);
    STRINGISE_ENUM_CAST(MTLPrimitiveTopologyClass_objc, MTLPrimitiveTopologyClassTriangle);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLWinding_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLWinding_objc)
  {
    STRINGISE_ENUM_CAST(MTLWinding_objc, MTLWindingClockwise);
    STRINGISE_ENUM_CAST(MTLWinding_objc, MTLWindingCounterClockwise);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLDepthClipMode_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLDepthClipMode_objc);
  {
    STRINGISE_ENUM_CAST(MTLDepthClipMode_objc, MTLDepthClipModeClip);
    STRINGISE_ENUM_CAST(MTLDepthClipMode_objc, MTLDepthClipModeClamp);
  }
  END_ENUM_STRINGISE();
}

template <>
rdcstr DoStringise(const MTLTriangleFillMode_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLTriangleFillMode_objc);
  {
    STRINGISE_ENUM_CAST(MTLTriangleFillMode_objc, MTLTriangleFillModeFill);
    STRINGISE_ENUM_CAST(MTLTriangleFillMode_objc, MTLTriangleFillModeLines);
  }
  END_ENUM_STRINGISE();
}

template <>
rdcstr DoStringise(const MTLCullMode_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLCullMode_objc);
  {
    STRINGISE_ENUM_CAST(MTLCullMode_objc, MTLCullModeNone);
    STRINGISE_ENUM_CAST(MTLCullMode_objc, MTLCullModeFront);
    STRINGISE_ENUM_CAST(MTLCullMode_objc, MTLCullModeBack);
  }
  END_ENUM_STRINGISE();
};

template <>
rdcstr DoStringise(const MTLTessellationFactorFormat_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLTessellationFactorFormat_objc)
  {
    STRINGISE_ENUM_CAST(MTLTessellationFactorFormat_objc, MTLTessellationFactorFormatHalf);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLTessellationControlPointIndexType_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLTessellationControlPointIndexType_objc)
  {
    STRINGISE_ENUM_CAST(MTLTessellationControlPointIndexType_objc,
                        MTLTessellationControlPointIndexTypeNone);
    STRINGISE_ENUM_CAST(MTLTessellationControlPointIndexType_objc,
                        MTLTessellationControlPointIndexTypeUInt16);
    STRINGISE_ENUM_CAST(MTLTessellationControlPointIndexType_objc,
                        MTLTessellationControlPointIndexTypeUInt32);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLTessellationFactorStepFunction_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLTessellationFactorStepFunction_objc)
  {
    STRINGISE_ENUM_CAST(MTLTessellationFactorStepFunction_objc,
                        MTLTessellationFactorStepFunctionConstant);
    STRINGISE_ENUM_CAST(MTLTessellationFactorStepFunction_objc,
                        MTLTessellationFactorStepFunctionPerPatch);
    STRINGISE_ENUM_CAST(MTLTessellationFactorStepFunction_objc,
                        MTLTessellationFactorStepFunctionPerInstance);
    STRINGISE_ENUM_CAST(MTLTessellationFactorStepFunction_objc,
                        MTLTessellationFactorStepFunctionPerPatchAndPerInstance);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLTessellationPartitionMode_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLTessellationPartitionMode_objc)
  {
    STRINGISE_ENUM_CAST(MTLTessellationPartitionMode_objc, MTLTessellationPartitionModePow2);
    STRINGISE_ENUM_CAST(MTLTessellationPartitionMode_objc, MTLTessellationPartitionModeInteger);
    STRINGISE_ENUM_CAST(MTLTessellationPartitionMode_objc, MTLTessellationPartitionModeFractionalOdd);
    STRINGISE_ENUM_CAST(MTLTessellationPartitionMode_objc,
                        MTLTessellationPartitionModeFractionalEven);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLCPUCacheMode_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLCPUCacheMode_objc)
  {
    STRINGISE_ENUM_CAST(MTLCPUCacheMode_objc, MTLCPUCacheModeDefaultCache);
    STRINGISE_ENUM_CAST(MTLCPUCacheMode_objc, MTLCPUCacheModeWriteCombined);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLStorageMode_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLStorageMode_objc)
  {
    STRINGISE_ENUM_CAST(MTLStorageMode_objc, MTLStorageModeShared);
    STRINGISE_ENUM_CAST(MTLStorageMode_objc, MTLStorageModeManaged);
    STRINGISE_ENUM_CAST(MTLStorageMode_objc, MTLStorageModePrivate);
    STRINGISE_ENUM_CAST(MTLStorageMode_objc, MTLStorageModeMemoryless);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLHazardTrackingMode_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLHazardTrackingMode_objc)
  {
    STRINGISE_ENUM_CAST(MTLHazardTrackingMode_objc, MTLHazardTrackingModeDefault);
    STRINGISE_ENUM_CAST(MTLHazardTrackingMode_objc, MTLHazardTrackingModeUntracked);
    STRINGISE_ENUM_CAST(MTLHazardTrackingMode_objc, MTLHazardTrackingModeTracked);
  }
  END_ENUM_STRINGISE()
}

#define STRINGISE_BITFIELD_BIT_CAST(TYPE, NAME) STRINGISE_BITFIELD_BIT_NAMED((TYPE)NAME, #NAME);

template <>
rdcstr DoStringise(const MTLResourceOptions_objc &el)
{
  BEGIN_BITFIELD_STRINGISE(MTLResourceOptions_objc)
  {
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceCPUCacheModeDefaultCache);
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceCPUCacheModeWriteCombined);

    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceStorageModeShared);
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceStorageModeManaged);
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceStorageModePrivate);
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceStorageModeMemoryless);

    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceHazardTrackingModeDefault);
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceHazardTrackingModeUntracked);
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceHazardTrackingModeTracked);

    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceOptionCPUCacheModeDefault);
    STRINGISE_BITFIELD_BIT_CAST(MTLResourceOptions_objc, MTLResourceOptionCPUCacheModeWriteCombined);
  }
  END_BITFIELD_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLTextureType_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLTextureType_objc)
  {
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureType1D);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureType1DArray);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureType2D);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureType2DArray);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureType2DMultisample);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureTypeCube);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureTypeCubeArray);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureType3D);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureType2DMultisampleArray);
    STRINGISE_ENUM_CAST(MTLTextureType_objc, MTLTextureTypeTextureBuffer);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLTextureUsage_objc &el)
{
  BEGIN_BITFIELD_STRINGISE(MTLTextureUsage_objc)
  {
    STRINGISE_BITFIELD_BIT_CAST(MTLTextureUsage_objc, MTLTextureUsageUnknown);
    STRINGISE_BITFIELD_BIT_CAST(MTLTextureUsage_objc, MTLTextureUsageShaderRead);
    STRINGISE_BITFIELD_BIT_CAST(MTLTextureUsage_objc, MTLTextureUsageShaderWrite);
    STRINGISE_BITFIELD_BIT_CAST(MTLTextureUsage_objc, MTLTextureUsageRenderTarget);
    STRINGISE_BITFIELD_BIT_CAST(MTLTextureUsage_objc, MTLTextureUsagePixelFormatView);
  }
  END_BITFIELD_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLTextureSwizzle_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLTextureSwizzle_objc)
  {
    STRINGISE_ENUM_CAST(MTLTextureSwizzle_objc, MTLTextureSwizzleZero);
    STRINGISE_ENUM_CAST(MTLTextureSwizzle_objc, MTLTextureSwizzleOne);
    STRINGISE_ENUM_CAST(MTLTextureSwizzle_objc, MTLTextureSwizzleRed);
    STRINGISE_ENUM_CAST(MTLTextureSwizzle_objc, MTLTextureSwizzleGreen);
    STRINGISE_ENUM_CAST(MTLTextureSwizzle_objc, MTLTextureSwizzleBlue);
    STRINGISE_ENUM_CAST(MTLTextureSwizzle_objc, MTLTextureSwizzleAlpha);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLLoadAction_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLLoadAction_objc)
  {
    STRINGISE_ENUM_CAST(MTLLoadAction_objc, MTLLoadActionDontCare);
    STRINGISE_ENUM_CAST(MTLLoadAction_objc, MTLLoadActionLoad);
    STRINGISE_ENUM_CAST(MTLLoadAction_objc, MTLLoadActionClear);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLStoreAction_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLStoreAction_objc)
  {
    STRINGISE_ENUM_CAST(MTLStoreAction_objc, MTLStoreActionDontCare);
    STRINGISE_ENUM_CAST(MTLStoreAction_objc, MTLStoreActionStore);
    STRINGISE_ENUM_CAST(MTLStoreAction_objc, MTLStoreActionMultisampleResolve);
    STRINGISE_ENUM_CAST(MTLStoreAction_objc, MTLStoreActionStoreAndMultisampleResolve);
    STRINGISE_ENUM_CAST(MTLStoreAction_objc, MTLStoreActionUnknown);
    STRINGISE_ENUM_CAST(MTLStoreAction_objc, MTLStoreActionCustomSampleDepthStore);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLStoreActionOptions_objc &el)
{
  BEGIN_BITFIELD_STRINGISE(MTLStoreActionOptions_objc)
  {
    STRINGISE_BITFIELD_BIT_CAST(MTLStoreActionOptions_objc, MTLStoreActionOptionNone);
    STRINGISE_BITFIELD_BIT_CAST(MTLStoreActionOptions_objc,
                                MTLStoreActionOptionCustomSamplePositions);
  }
  END_BITFIELD_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLBlendFactor_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLBlendFactor_objc)
  {
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorZero);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOne);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorSourceColor);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusSourceColor);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorSourceAlpha);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusSourceAlpha);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorDestinationColor);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusDestinationColor);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusDestinationAlpha);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorSourceAlphaSaturated);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorBlendColor);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusBlendColor);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorBlendAlpha);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusBlendAlpha);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorSource1Color);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusSource1Color);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorSource1Alpha);
    STRINGISE_ENUM_CAST(MTLBlendFactor_objc, MTLBlendFactorOneMinusSource1Alpha);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLBlendOperation_objc &el)
{
  BEGIN_ENUM_STRINGISE(MTLBlendOperation_objc)
  {
    STRINGISE_ENUM_CAST(MTLBlendOperation_objc, MTLBlendOperationAdd);
    STRINGISE_ENUM_CAST(MTLBlendOperation_objc, MTLBlendOperationSubtract);
    STRINGISE_ENUM_CAST(MTLBlendOperation_objc, MTLBlendOperationReverseSubtract);
    STRINGISE_ENUM_CAST(MTLBlendOperation_objc, MTLBlendOperationMin);
    STRINGISE_ENUM_CAST(MTLBlendOperation_objc, MTLBlendOperationMax);
  }
  END_ENUM_STRINGISE()
}

template <>
rdcstr DoStringise(const MTLColorWriteMask_objc &el)
{
  BEGIN_BITFIELD_STRINGISE(MTLColorWriteMask_objc)
  {
    STRINGISE_BITFIELD_BIT_CAST(MTLColorWriteMask_objc, MTLColorWriteMaskNone);
    STRINGISE_BITFIELD_BIT_CAST(MTLColorWriteMask_objc, MTLColorWriteMaskRed);
    STRINGISE_BITFIELD_BIT_CAST(MTLColorWriteMask_objc, MTLColorWriteMaskGreen);
    STRINGISE_BITFIELD_BIT_CAST(MTLColorWriteMask_objc, MTLColorWriteMaskBlue);
    STRINGISE_BITFIELD_BIT_CAST(MTLColorWriteMask_objc, MTLColorWriteMaskAlpha);
    STRINGISE_BITFIELD_BIT_CAST(MTLColorWriteMask_objc, MTLColorWriteMaskAll);
  }
  END_BITFIELD_STRINGISE()
}

template <>
rdcstr DoStringise(const NSInteger_objc &el)
{
  RDCCOMPILE_ASSERT(sizeof(NSInteger_objc) == sizeof(NSInteger), "NSInteger size does not match");
  RDCCOMPILE_ASSERT(sizeof(NSInteger_objc) == sizeof(int64_t), "NSInteger size does not match");
  return DoStringise((int64_t)el);
}

template <>
rdcstr DoStringise(const NSUInteger_objc &el)
{
  RDCCOMPILE_ASSERT(sizeof(NSUInteger_objc) == sizeof(NSUInteger),
                    "NSUInteger size does not match");
  RDCCOMPILE_ASSERT(sizeof(NSUInteger_objc) == sizeof(uint64_t), "NSInteger size does not match");
  return DoStringise((uint64_t)el);
}

template <>
rdcstr DoStringise(const MetalResourceType &el)
{
  BEGIN_ENUM_STRINGISE(MetalResourceType);
  {
    STRINGISE_ENUM(eResUnknown);
    STRINGISE_ENUM(eResBuffer);
    STRINGISE_ENUM(eResCommandBuffer);
    STRINGISE_ENUM(eResCommandQueue);
    STRINGISE_ENUM(eResDevice);
    STRINGISE_ENUM(eResLibrary);
    STRINGISE_ENUM(eResFunction);
    STRINGISE_ENUM(eResRenderCommandEncoder);
    STRINGISE_ENUM(eResRenderPipelineState);
    STRINGISE_ENUM(eResTexture);
  }
  END_ENUM_STRINGISE();
}
