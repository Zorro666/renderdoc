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

#pragma once

#include "replay/replay_driver.h"
#include "metal_common.h"
#include "metal_shaders.h"

enum TexDisplayFlags
{
  eTexDisplay_16Render = 0x1,
  eTexDisplay_32Render = 0x2,
  eTexDisplay_BlendAlpha = 0x4,
  eTexDisplay_MipShift = 0x8,
  eTexDisplay_GreenOnly = 0x10,
  eTexDisplay_RemapFloat = 0x20,
  eTexDisplay_RemapUInt = 0x40,
  eTexDisplay_RemapSInt = 0x80,
};

class MetalReplay : public IReplayDriver
{
public:
  MetalReplay(WrappedMTLDevice *wrappedMTLDevice);
  virtual ~MetalReplay() {}
  void SetProxy(bool p) { m_Proxy = p; }
  ResourceDescription &GetResourceDesc(ResourceId id);
  void CreateResources();
  WrappedMTLLibrary *GetDebugShaderLibrary() { return m_debugLibrary; }
  /* IRemoteDriver */
  void Shutdown();

  APIProperties GetAPIProperties();

  rdcarray<ResourceDescription> GetResources();

  rdcarray<BufferDescription> GetBuffers();
  BufferDescription GetBuffer(ResourceId id);

  rdcarray<TextureDescription> GetTextures();
  TextureDescription GetTexture(ResourceId id);

  rdcarray<DebugMessage> GetDebugMessages();

  rdcarray<ShaderEntryPoint> GetShaderEntryPoints(ResourceId shader);
  ShaderReflection *GetShader(ResourceId pipeline, ResourceId shader, ShaderEntryPoint entry);

  rdcarray<rdcstr> GetDisassemblyTargets(bool withPipeline);
  rdcstr DisassembleShader(ResourceId pipeline, const ShaderReflection *refl, const rdcstr &target);

  void SetPipelineStates(D3D11Pipe::State *d3d11, D3D12Pipe::State *d3d12, GLPipe::State *gl,
                         VKPipe::State *vk, MetalPipe::State *metal)
  {
    m_MetalPipelineState = metal;
  }

  rdcarray<EventUsage> GetUsage(ResourceId id);

  void SavePipelineState(uint32_t eventId);

  FrameRecord GetFrameRecord();

  RDResult ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers);
  void ReplayLog(uint32_t endEventID, ReplayLogType replayType);
  SDFile *GetStructuredFile();

  rdcarray<uint32_t> GetPassEvents(uint32_t eventId);

  void InitPostVSBuffers(uint32_t eventId);
  void InitPostVSBuffers(const rdcarray<uint32_t> &passEvents);

  ResourceId GetLiveID(ResourceId id);

  MeshFormat GetPostVSBuffers(uint32_t eventId, uint32_t instID, uint32_t viewID,
                              MeshDataStage stage);

  void GetBufferData(ResourceId buff, uint64_t offset, uint64_t len, bytebuf &retData);
  void GetTextureData(ResourceId tex, const Subresource &sub, const GetTextureDataParams &params,
                      bytebuf &data);

  void BuildTargetShader(ShaderEncoding sourceEncoding, const bytebuf &source, const rdcstr &entry,
                         const ShaderCompileFlags &compileFlags, ShaderStage type, ResourceId &id,
                         rdcstr &errors);
  rdcarray<ShaderEncoding> GetTargetShaderEncodings();
  void ReplaceResource(ResourceId from, ResourceId to);
  void RemoveReplacement(ResourceId id);
  void FreeTargetResource(ResourceId id);

  rdcarray<GPUCounter> EnumerateCounters();
  CounterDescription DescribeCounter(GPUCounter counterID);
  rdcarray<CounterResult> FetchCounters(const rdcarray<GPUCounter> &counterID);

  void FillCBufferVariables(ResourceId pipeline, ResourceId shader, ShaderStage stage,
                            rdcstr entryPoint, uint32_t cbufSlot, rdcarray<ShaderVariable> &outvars,
                            const bytebuf &data);

  rdcarray<PixelModification> PixelHistory(rdcarray<EventUsage> events, ResourceId target, uint32_t x,
                                           uint32_t y, const Subresource &sub, CompType typeCast);
  ShaderDebugTrace *DebugVertex(uint32_t eventId, uint32_t vertid, uint32_t instid, uint32_t idx,
                                uint32_t view);
  ShaderDebugTrace *DebugPixel(uint32_t eventId, uint32_t x, uint32_t y, uint32_t sample,
                               uint32_t primitive);
  ShaderDebugTrace *DebugThread(uint32_t eventId, const rdcfixedarray<uint32_t, 3> &groupid,
                                const rdcfixedarray<uint32_t, 3> &threadid);
  rdcarray<ShaderDebugState> ContinueDebug(ShaderDebugger *debugger);
  void FreeDebugger(ShaderDebugger *debugger);

  ResourceId RenderOverlay(ResourceId texid, FloatVector clearCol, DebugOverlay overlay,
                           uint32_t eventId, const rdcarray<uint32_t> &passEvents);

  bool IsRenderOutput(ResourceId id);

  void FileChanged();
  RDResult FatalErrorCheck();

  bool NeedRemapForFetch(const ResourceFormat &format);

  DriverInformation GetDriverInfo();

  rdcarray<GPUDevice> GetAvailableGPUs();
  /* End IRemoteDriver */

  /* IReplayDriver */
  bool IsRemoteProxy() { return m_Proxy; }
  IReplayDriver *MakeDummyDriver();

  rdcarray<WindowingSystem> GetSupportedWindowSystems();

  AMDRGPControl *GetRGPControl();

  uint64_t MakeOutputWindow(WindowingData window, bool depth);
  void DestroyOutputWindow(uint64_t id);
  bool CheckResizeOutputWindow(uint64_t id);
  void SetOutputWindowDimensions(uint64_t id, int32_t w, int32_t h);
  void GetOutputWindowDimensions(uint64_t id, int32_t &w, int32_t &h);
  void GetOutputWindowData(uint64_t id, bytebuf &retData);
  void ClearOutputWindowColor(uint64_t id, FloatVector col);
  void ClearOutputWindowDepth(uint64_t id, float depth, uint8_t stencil);
  void BindOutputWindow(uint64_t id, bool depth);
  bool IsOutputWindowVisible(uint64_t id);
  void FlipOutputWindow(uint64_t id);

  bool GetMinMax(ResourceId texid, const Subresource &sub, CompType typeCast, float *minval,
                 float *maxval);
  bool GetHistogram(ResourceId texid, const Subresource &sub, CompType typeCast, float minval,
                    float maxval, const rdcfixedarray<bool, 4> &channels,
                    rdcarray<uint32_t> &histogram);
  void PickPixel(ResourceId texture, uint32_t x, uint32_t y, const Subresource &sub,
                 CompType typeCast, float pixel[4]);

  ResourceId CreateProxyTexture(const TextureDescription &templateTex);
  void SetProxyTextureData(ResourceId texid, const Subresource &sub, byte *data, size_t dataSize);
  bool IsTextureSupported(const TextureDescription &tex);

  ResourceId CreateProxyBuffer(const BufferDescription &templateBuf);
  void SetProxyBufferData(ResourceId bufid, byte *data, size_t dataSize);

  void RenderMesh(uint32_t eventId, const rdcarray<MeshFormat> &secondaryDraws,
                  const MeshDisplay &cfg);
  bool RenderTexture(TextureDisplay cfg);

  void SetCustomShaderIncludes(const rdcarray<rdcstr> &directories);
  void BuildCustomShader(ShaderEncoding sourceEncoding, const bytebuf &source, const rdcstr &entry,
                         const ShaderCompileFlags &compileFlags, ShaderStage type, ResourceId &id,
                         rdcstr &errors);
  rdcarray<ShaderEncoding> GetCustomShaderEncodings();
  rdcarray<ShaderSourcePrefix> GetCustomShaderSourcePrefixes();
  ResourceId ApplyCustomShader(TextureDisplay &display);
  void FreeCustomShader(ResourceId id);

  void RenderCheckerboard(FloatVector dark, FloatVector light);

  void RenderHighlightBox(float w, float h, float scale);

  uint32_t PickVertex(uint32_t eventId, int32_t width, int32_t height, const MeshDisplay &cfg,
                      uint32_t x, uint32_t y);
  /* End IReplayDriver */

  FrameRecord &WriteFrameRecord() { return m_FrameRecord; }
private:
  void InitDebugRenderer();
  void ShutdownDebugRenderer();
  bool RenderTextureInternal(TextureDisplay cfg, const MetalTextureState &textureState,
                             WrappedMTLRenderPipelineState *rPipeline,
                             MTL::RenderPassDescriptor *rPassDesc, WrappedMTLBuffer *debugCBuffer,
                             int flags);
  void SetDriverInfo();

  WrappedMTLDevice *m_pDriver = NULL;
  MetalPipe::State *m_MetalPipelineState = NULL;

  rdcarray<ResourceDescription> m_Resources;
  std::map<ResourceId, size_t> m_ResourceIdx;

  struct RenderPipePass
  {
    void Init(WrappedMTLDevice *driver, WrappedMTLFunction *vertexFunction,
              WrappedMTLFunction *fragmentFunction, WrappedMTLTexture *texture,
              MTL::LoadAction loadAction, MTL::StoreAction storeAction, float clearR, float clearG,
              float clearB, float clearA);
    void Destroy(WrappedMTLDevice *driver);

    WrappedMTLRenderPipelineState *Pipeline = NULL;
    MTL::RenderPassDescriptor *MTLPassDesc = NULL;
  };

  struct OutputWindow
  {
    OutputWindow();

    void Create(WrappedMTLDevice *device, bool depth);
    void Destroy(WrappedMTLDevice *device);

    void CreateSurface(WrappedMTLDevice *device);
    void SetWindowHandle(WindowingData window);
    bool NoOutput();

    enum
    {
      BUFFER_CHECKERBOARD = 0,
      BUFFER_RENDERTEXTURE,
      BUFFER_COUNT
    };

    CA::MetalLayer *MetalLayer = NULL;
    WrappedMTLTexture *FB = NULL;
    WrappedMTLBuffer *CBuffers[BUFFER_COUNT];
    MetalResourceManager *ResourceManager;
    MTL::RenderPassDescriptor *MTLPassDesc = NULL;

    WindowingSystem WindowSystem = WindowingSystem::Unknown;
    uint32_t Width = 0;
    uint32_t Height = 0;

    bool HasDepth = false;
  };

  struct TextureRendering
  {
    void Init(WrappedMTLDevice *driver);
    void Destroy(WrappedMTLDevice *driver);
    WrappedMTLRenderPipelineState *Pipeline = NULL;
  } m_TexRender;

  struct PickPixel
  {
    void Init(WrappedMTLDevice *driver);
    void Destroy(WrappedMTLDevice *driver);
    WrappedMTLBuffer *CBuffer = NULL;
    WrappedMTLTexture *FB = NULL;
    RenderPipePass PipePass;
  } m_PickPixel;

  std::map<uint64_t, OutputWindow> m_OutputWindows;
  uint64_t m_OutputWinID;
  uint64_t m_ActiveWinID;
  bool m_BindDepth = false;
  uint32_t m_DebugWidth = 0;
  uint32_t m_DebugHeight = 0;

  WrappedMTLLibrary *m_debugLibrary = NULL;
  WrappedMTLRenderPipelineState *m_checkerboardPipeline = NULL;
  WrappedMTLRenderPipelineState *m_outlinePipeline = NULL;

  FrameRecord m_FrameRecord;
  DriverInformation m_DriverInfo;
  bool m_Proxy = false;
};

void FillRenderPassDescriptor(WrappedMTLTexture *fb, MTL::LoadAction loadAction,
                              MTL::StoreAction storeAction, float clearR, float clearG,
                              float clearB, float clearA, RDMTL::RenderPassDescriptor &rPassDesc);
