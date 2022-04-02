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

const char *debugShaders = R"(

using namespace metal;

#define HAS_BIT_CONVERSION

// See metal_shaders.metal for the C++
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

struct UVVertex
{
  float4 position [[position]];
  float2 uv;
};

float ConvertSRGBToLinear(float srgb)
{
  if(srgb <= 0.04045f)
    return srgb / 12.92f;
  else
    return pow((clamp(srgb, 0.0f, 1.0f) + 0.055f) / 1.055f, 2.4f);
}

vertex UVVertex blit_vertex(uint vid [[vertex_id]])
{
  const float4 verts[4] = {
                            float4(-1.0, -1.0, 0.5, 1.0),
                            float4(1.0, -1.0, 0.5, 1.0),
                            float4(-1.0, 1.0, 0.5, 1.0),
                            float4(1.0, 1.0, 0.5, 1.0)
                          };
  UVVertex vert;
  vert.position = verts[vid];
  vert.uv = vert.position.xy * 0.5f + 0.5f;
  return vert;
}

fragment float4 checkerboard_fragment(constant CheckerUBOData& checker [[buffer(0)]],
                                      UVVertex vert [[stage_in]])
{
  float2 RectRelativePos = vert.position.xy - checker.RectPosition;

  // if we have a border, and our pos is inside the border, return inner color
  if(checker.BorderWidth >= 0.0f)
  {
    if(RectRelativePos.x >= checker.BorderWidth &&
       RectRelativePos.x <= checker.RectSize.x - checker.BorderWidth &&
       RectRelativePos.y >= checker.BorderWidth &&
       RectRelativePos.y <= checker.RectSize.y - checker.BorderWidth)
    {
      return checker.InnerColor;
    }
  }

  float2 ab = fmod(RectRelativePos.xy, float2(checker.CheckerSquareDimension * 2.0f));

  bool checkerVariant =
  ((ab.x < checker.CheckerSquareDimension && ab.y < checker.CheckerSquareDimension) ||
   (ab.x > checker.CheckerSquareDimension && ab.y > checker.CheckerSquareDimension));

  // otherwise return checker pattern
  float4 color_out = checkerVariant ? checker.PrimaryColor : checker.SecondaryColor;
  return color_out;
}

fragment float4 texdisplay_fragment(constant TexDisplayUBOData& texdisplay [[buffer(0)]],
                                    constant HeatmapData& heatmap [[buffer(1)]],
                                    UVVertex vert [[stage_in]],
                                    texture2d<float> colorTexture [[texture(0)]])
{
  bool uintTex = (texdisplay.OutputDisplayFormat & TEXDISPLAY_UINT_TEX) != 0;
  bool sintTex = (texdisplay.OutputDisplayFormat & TEXDISPLAY_SINT_TEX) != 0;

  int texType = (texdisplay.OutputDisplayFormat & TEXDISPLAY_TYPEMASK);

  float4 col;
  uint4 ucol;
  int4 scol;

  // calc screen co-ords with origin top left, modified by Position
  float2 scr = vert.position.xy;

  scr -= texdisplay.Position.xy;

  scr /= texdisplay.TextureResolutionPS.xy;

  scr /= texdisplay.Scale;

  scr /= float2(texdisplay.MipShift, texdisplay.MipShift);
  float2 scr2 = scr;

  if(texType == RESTYPE_TEX1D)
  {
    // by convention display 1D textures as 100 high
    if(scr2.x < 0.0f || scr2.x > 1.0f || scr2.y < 0.0f || scr2.y > 100.0f)
    {
      return float4(0, 0, 0, 0);
    }
  }
  else
  {
    if(scr2.x < 0.0f || scr2.y < 0.0f || scr2.x > 1.0f || scr2.y > 1.0f)
    {
      return float4(0, 0, 0, 0);
    }
  }

  const int defaultFlipY = 0;
  if(texdisplay.FlipY != defaultFlipY)
    scr.y = 1.0f - scr.y;

  if(uintTex)
  {
//    ucol = SampleTextureUInt4(texType, scr, texdisplay.Slice, texdisplay.MipLevel,
//                              texdisplay.SampleIdx, texdisplay.TextureResolutionPS);
    ucol = 0;
  }
  else if(sintTex)
  {
//    scol = SampleTextureSInt4(texType, scr, texdisplay.Slice, texdisplay.MipLevel,
//                              texdisplay.SampleIdx, texdisplay.TextureResolutionPS);
    scol = 0;
  }
  else
  {
//    col = SampleTextureFloat4(texType, scr, texdisplay.Slice, texdisplay.MipLevel,
//                              texdisplay.SampleIdx, texdisplay.TextureResolutionPS,
//                              texdisplay.YUVDownsampleRate, texdisplay.YUVAChannels);
    constexpr sampler textureSampler (mag_filter::nearest, min_filter::nearest);
    col = colorTexture.sample(textureSampler, scr);
  }

  if(texdisplay.RawOutput != 0)
  {
#ifdef HAS_BIT_CONVERSION
    if(uintTex)
      return as_type<float4>(ucol);
    else if(sintTex)
      return as_type<float4>(scol);
#else
    // without being able to alias bits, we won't get accurate results.
    // a cast is better than nothing though
    if(uintTex)
      return float4(ucol);
    else if(sintTex)
      return float4(scol);
#endif
    return col;
  }

  if(heatmap.HeatmapMode != HEATMAP_DISABLED)
  {
    if(heatmap.HeatmapMode == HEATMAP_LINEAR)
    {
      // cast the float value to an integer with safe rounding, then return the
      int bucket = int(floor(col.x + 0.25f));

      bucket = max(bucket, 0);
      bucket = min(bucket, HEATMAP_RAMPSIZE - 1);

      if(bucket == 0)
        discard_fragment();

      return heatmap.ColorRamp[bucket];
    }
    else if(heatmap.HeatmapMode == HEATMAP_TRISIZE)
    {
      // uninitialised regions have alpha=0
      if(col.w < 0.5f)
        discard_fragment();

      float area = max(col.x, 0.001f);

      int bucket = 2 + int(floor(20.0f - 20.1f * (1.0f - exp(-0.4f * area))));

      bucket = max(bucket, 0);
      bucket = min(bucket, HEATMAP_RAMPSIZE - 1);

      return heatmap.ColorRamp[bucket];
    }
    else
    {
      // error! invalid heatmap mode
    }
  }

  // YUV decoding
  if(texdisplay.DecodeYUV != 0)
  {
    // assume col is now YUVA, perform a default conversion to RGBA
    const float Kr = 0.2126f;
    const float Kb = 0.0722f;

    float L = col.g;
    float Pb = col.b - 0.5f;
    float Pr = col.r - 0.5f;

    col.b = L + (Pb / 0.5f) * (1 - Kb);
    col.r = L + (Pr / 0.5f) * (1 - Kr);
    col.g = (L - Kr * col.r - Kb * col.b) / (1.0f - Kr - Kb);
  }

  // RGBM encoding
  if(texdisplay.HDRMul > 0.0f)
  {
    if(uintTex)
      col = float4(float3(ucol.rgb * ucol.a * uint(texdisplay.HDRMul)), 1.0f);
    else if(sintTex)
      col = float4(float3(scol.rgb * scol.a * int(texdisplay.HDRMul)), 1.0f);
    else
      col = float4(float3(col.rgb * col.a * texdisplay.HDRMul), 1.0f);
  }

  if(uintTex)
    col = float4(ucol);
  else if(sintTex)
    col = float4(scol);

  float4 pre_range_col = col;

  col = ((col - texdisplay.RangeMinimum) * texdisplay.InverseRangeSize);

  if(texdisplay.Channels.x < 0.5f)
    col.x = pre_range_col.x = 0.0f;
  if(texdisplay.Channels.y < 0.5f)
    col.y = pre_range_col.y = 0.0f;
  if(texdisplay.Channels.z < 0.5f)
    col.z = pre_range_col.z = 0.0f;
  if(texdisplay.Channels.w < 0.5f)
    col.w = pre_range_col.w = 1.0f;

  if((texdisplay.OutputDisplayFormat & TEXDISPLAY_NANS) > 0)
  {
    if(isnan(pre_range_col.r) || isnan(pre_range_col.g) || isnan(pre_range_col.b) ||
       isnan(pre_range_col.a))
    {
      return float4(1, 0, 0, 1);
    }

    if(isinf(pre_range_col.r) || isinf(pre_range_col.g) || isinf(pre_range_col.b) ||
       isinf(pre_range_col.a))
    {
      return float4(0, 1, 0, 1);
    }

    if(pre_range_col.r < 0.0f || pre_range_col.g < 0.0f || pre_range_col.b < 0.0f ||
       pre_range_col.a < 0.0f)
    {
      return float4(0, 0, 1, 1);
    }

    col = float4(float3(dot(col.xyz, float3(0.2126, 0.7152, 0.0722))), 1);
  }
  else if((texdisplay.OutputDisplayFormat & TEXDISPLAY_CLIPPING) > 0)
  {
    if(col.r < 0.0f || col.g < 0.0f || col.b < 0.0f || col.a < 0.0f)
    {
      return float4(1, 0, 0, 1);
    }

    if(col.r > (1.0f + FLT_EPSILON) || col.g > (1.0f + FLT_EPSILON) ||
       col.b > (1.0f + FLT_EPSILON) || col.a > (1.0f + FLT_EPSILON))
    {
      return float4(0, 1, 0, 1);
    }

    col = float4(float3(dot(col.xyz, float3(0.2126, 0.7152, 0.0722))), 1);
  }
  else
  {
    // if only one channel is selected
    if(dot(texdisplay.Channels, float4(1.0f)) == 1.0f)
    {
      // if it's alpha, just move it into rgb
      // otherwise, select the channel that's on and replicate it across all channels
      if(texdisplay.Channels.a == 1.0f)
        col = float4(col.aaa, 1);
      else
        // this is a splat because col has already gone through the if(texdisplay.Channels.x < 0.5f)
        // statements
        col = float4(float3(dot(col.rgb, float3(1.0f))), 1.0f);
    }
  }

  if((texdisplay.OutputDisplayFormat & TEXDISPLAY_GAMMA_CURVE) > 0)
  {
    col.rgb =
        float3(ConvertSRGBToLinear(col.r), ConvertSRGBToLinear(col.g), ConvertSRGBToLinear(col.b));
  }

  return col;
}

vertex float4 fullscreen_vertex(uint vid [[vertex_id]])
{
  const float4 verts[4] = {
                            float4(-1.0, -1.0, 0.5, 1.0),
                            float4(1.0, -1.0, 0.5, 1.0),
                            float4(-1.0, 1.0, 0.5, 1.0),
                            float4(1.0, 1.0, 0.5, 1.0)
                          };
  return verts[vid];
}

fragment float4 colour_fragment(constant float4 &colour [[buffer(0)]])
{
    return colour;
}

)";
