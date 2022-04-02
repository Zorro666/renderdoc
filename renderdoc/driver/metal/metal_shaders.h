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

#pragma once

#include "common/common.h"

#define float2 Vec2f
#define float3 Vec3f
#define float4 Vec4f

#define uint2 Vec2u
#define uint3 Vec3u
#define uint4 Vec4u

// See metal_shaders.metal for the Metal shader
// versions of the constants and UBO structs
#define HEATMAP_DISABLED 0
#define HEATMAP_LINEAR 1
#define HEATMAP_TRISIZE 2

#define HEATMAP_RAMPSIZE 22

#define TEXDISPLAY_TYPEMASK 0xF
#define TEXDISPLAY_UINT_TEX 0x10
#define TEXDISPLAY_SINT_TEX 0x20
#define TEXDISPLAY_NANS 0x80
#define TEXDISPLAY_CLIPPING 0x100
#define TEXDISPLAY_GAMMA_CURVE 0x200

#define RESTYPE_TEX1D 0x1
#define RESTYPE_TEX2D 0x2
#define RESTYPE_TEX3D 0x3
#define RESTYPE_TEXCUBE 0x4
#define RESTYPE_TEX1DARRAY 0x5
#define RESTYPE_TEX2DARRAY 0x6
#define RESTYPE_TEXCUBEARRAY 0x7
#define RESTYPE_TEXRECT 0x8
#define RESTYPE_TEXBUFFER 0x9
#define RESTYPE_TEX2DMS 0xA
#define RESTYPE_TEX2DMSARRAY 0xB
#define RESTYPE_TEXTYPEMAX 0xB

struct CheckerUBOData
{
  float2 RectPosition;
  float2 RectSize;

  float4 PrimaryColor;
  float4 SecondaryColor;
  float4 InnerColor;

  float CheckerSquareDimension;
  float BorderWidth;
};

struct HeatmapData
{
  int HeatmapMode;
  int DummyA;
  int DummyB;
  int DummyC;

  // must match size of colorRamp on C++ side
  float4 ColorRamp[22];
};

struct TexDisplayUBOData
{
  float2 Position;
  float Scale;
  float HDRMul;

  float4 Channels;

  float RangeMinimum;
  float InverseRangeSize;
  int MipLevel;
  int FlipY;

  float4 TextureResolutionPS;

  float2 OutputRes;
  int RawOutput;
  float Slice;

  int SampleIdx;
  float MipShift;
  int DecodeYUV;
  int OutputDisplayFormat;

  uint4 YUVDownsampleRate;
  uint4 YUVAChannels;
};

#undef float2
#undef float3
#undef float4

#undef uint2
#undef uint3
#undef uint4
