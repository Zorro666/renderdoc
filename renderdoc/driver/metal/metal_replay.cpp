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

#include "metal_replay.h"
#include "serialise/rdcfile.h"
#include "metal_core.h"
#include "metal_device.h"

MetalReplay::MetalReplay()
{
  RDCWARN("MetalReplay::MetalReplay : Not implemented");
}

/* IRemoteDriver */
void MetalReplay::Shutdown()
{
  RDCWARN("MetalReplay::Shutdown : Not implemented");
}

APIProperties MetalReplay::GetAPIProperties()
{
  RDCWARN("Not implemented");
  return APIProperties();
}

rdcarray<ResourceDescription> MetalReplay::GetResources()
{
  RDCWARN("Not implemented");
  return rdcarray<ResourceDescription>();
}

rdcarray<BufferDescription> MetalReplay::GetBuffers()
{
  RDCWARN("Not implemented");
  return rdcarray<BufferDescription>();
}
BufferDescription MetalReplay::GetBuffer(ResourceId id)
{
  RDCWARN("Not implemented");
  return BufferDescription();
}

rdcarray<TextureDescription> MetalReplay::GetTextures()
{
  RDCWARN("Not implemented");
  return rdcarray<TextureDescription>();
}
TextureDescription MetalReplay::GetTexture(ResourceId id)
{
  RDCWARN("Not implemented");
  return TextureDescription();
}

rdcarray<DebugMessage> MetalReplay::GetDebugMessages()
{
  RDCWARN("Not implemented");
  return rdcarray<DebugMessage>();
}

rdcarray<ShaderEntryPoint> MetalReplay::GetShaderEntryPoints(ResourceId shader)
{
  RDCWARN("Not implemented");
  return rdcarray<ShaderEntryPoint>();
}

ShaderReflection *MetalReplay::GetShader(ResourceId pipeline, ResourceId shader,
                                         ShaderEntryPoint entry)
{
  RDCWARN("Not implemented");
  return NULL;
}

rdcarray<rdcstr> MetalReplay::GetDisassemblyTargets(bool withPipeline)
{
  RDCWARN("Not implemented");
  return rdcarray<rdcstr>();
}

rdcstr MetalReplay::DisassembleShader(ResourceId pipeline, const ShaderReflection *refl,
                                      const rdcstr &target)
{
  RDCWARN("Not implemented");
  return rdcstr();
}

rdcarray<EventUsage> MetalReplay::GetUsage(ResourceId id)
{
  RDCWARN("Not implemented");
  return rdcarray<EventUsage>();
}

void MetalReplay::SavePipelineState(uint32_t eventId)
{
  RDCWARN("Not implemented");
}

const D3D11Pipe::State *MetalReplay::GetD3D11PipelineState()
{
  RDCWARN("Not implemented");
  return NULL;
}

const D3D12Pipe::State *MetalReplay::GetD3D12PipelineState()
{
  RDCWARN("Not implemented");
  return NULL;
}

const GLPipe::State *MetalReplay::GetGLPipelineState()
{
  RDCWARN("Not implemented");
  return NULL;
}
const VKPipe::State *MetalReplay::GetVulkanPipelineState()
{
  RDCWARN("Not implemented");
  return NULL;
}
const MetalPipe::State *MetalReplay::GetMetalPipelineState()
{
  RDCWARN("Not implemented");
  return NULL;
}

FrameRecord MetalReplay::GetFrameRecord()
{
  RDCWARN("Not implemented");
  return FrameRecord();
}

ReplayStatus MetalReplay::ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers)
{
  RDCWARN("Not implemented");
  return ReplayStatus::APIUnsupported;
}

void MetalReplay::ReplayLog(uint32_t endEventID, ReplayLogType replayType)
{
  RDCWARN("Not implemented");
}

SDFile *MetalReplay::GetStructuredFile()
{
  RDCWARN("Not implemented");
  return m_wrappedMTLDevice->GetStructuredFile();
}

rdcarray<uint32_t> MetalReplay::GetPassEvents(uint32_t eventId)
{
  RDCWARN("Not implemented");
  return rdcarray<uint32_t>();
}

void MetalReplay::InitPostVSBuffers(uint32_t eventId)
{
  RDCWARN("Not implemented");
}

void MetalReplay::InitPostVSBuffers(const rdcarray<uint32_t> &passEvents)
{
  RDCWARN("Not implemented");
}

ResourceId MetalReplay::GetLiveID(ResourceId id)
{
  RDCWARN("Not implemented");
  return ResourceId();
}

MeshFormat MetalReplay::GetPostVSBuffers(uint32_t eventId, uint32_t instID, uint32_t viewID,
                                         MeshDataStage stage)
{
  RDCWARN("Not implemented");
  return MeshFormat();
}

void MetalReplay::GetBufferData(ResourceId buff, uint64_t offset, uint64_t len, bytebuf &retData)
{
  RDCWARN("Not implemented");
}

void MetalReplay::GetTextureData(ResourceId tex, const Subresource &sub,
                                 const GetTextureDataParams &params, bytebuf &data)
{
  RDCWARN("Not implemented");
}

void MetalReplay::BuildTargetShader(ShaderEncoding sourceEncoding, const bytebuf &source,
                                    const rdcstr &entry, const ShaderCompileFlags &compileFlags,
                                    ShaderStage type, ResourceId &id, rdcstr &errors)
{
  RDCWARN("Not implemented");
}

rdcarray<ShaderEncoding> MetalReplay::GetTargetShaderEncodings()
{
  RDCWARN("Not implemented");
  return rdcarray<ShaderEncoding>();
}

void MetalReplay::ReplaceResource(ResourceId from, ResourceId to)
{
  RDCWARN("Not implemented");
}

void MetalReplay::RemoveReplacement(ResourceId id)
{
  RDCWARN("Not implemented");
}

void MetalReplay::FreeTargetResource(ResourceId id)
{
  RDCWARN("Not implemented");
}

rdcarray<GPUCounter> MetalReplay::EnumerateCounters()
{
  RDCWARN("Not implemented");
  return rdcarray<GPUCounter>();
}

CounterDescription MetalReplay::DescribeCounter(GPUCounter counterID)
{
  RDCWARN("Not implemented");
  return CounterDescription();
}

rdcarray<CounterResult> MetalReplay::FetchCounters(const rdcarray<GPUCounter> &counterID)
{
  RDCWARN("Not implemented");
  return rdcarray<CounterResult>();
}

void MetalReplay::FillCBufferVariables(ResourceId pipeline, ResourceId shader, rdcstr entryPoint,
                                       uint32_t cbufSlot, rdcarray<ShaderVariable> &outvars,
                                       const bytebuf &data)
{
  RDCWARN("Not implemented");
}

rdcarray<PixelModification> MetalReplay::PixelHistory(rdcarray<EventUsage> events,
                                                      ResourceId target, uint32_t x, uint32_t y,
                                                      const Subresource &sub, CompType typeCast)
{
  RDCWARN("Not implemented");
  return rdcarray<PixelModification>();
}

ShaderDebugTrace *MetalReplay::DebugVertex(uint32_t eventId, uint32_t vertid, uint32_t instid,
                                           uint32_t idx, uint32_t view)
{
  RDCWARN("Not implemented");
  return NULL;
}

ShaderDebugTrace *MetalReplay::DebugPixel(uint32_t eventId, uint32_t x, uint32_t y, uint32_t sample,
                                          uint32_t primitive)
{
  RDCWARN("Not implemented");
  return NULL;
}

ShaderDebugTrace *MetalReplay::DebugThread(uint32_t eventId,
                                           const rdcfixedarray<uint32_t, 3> &groupid,
                                           const rdcfixedarray<uint32_t, 3> &threadid)
{
  RDCWARN("Not implemented");
  return NULL;
}

rdcarray<ShaderDebugState> MetalReplay::ContinueDebug(ShaderDebugger *debugger)
{
  RDCWARN("Not implemented");
  return rdcarray<ShaderDebugState>();
}

void MetalReplay::FreeDebugger(ShaderDebugger *debugger)
{
  RDCWARN("Not implemented");
}

ResourceId MetalReplay::RenderOverlay(ResourceId texid, FloatVector clearCol, DebugOverlay overlay,
                                      uint32_t eventId, const rdcarray<uint32_t> &passEvents)
{
  RDCWARN("Not implemented");
  return ResourceId();
}

bool MetalReplay::IsRenderOutput(ResourceId id)
{
  RDCWARN("Not implemented");
  return false;
}

void MetalReplay::FileChanged()
{
  RDCWARN("Not implemented");
}

ReplayStatus MetalReplay::FatalErrorCheck()
{
  RDCWARN("Not implemented");
  return ReplayStatus::APIUnsupported;
}

bool MetalReplay::NeedRemapForFetch(const ResourceFormat &format)
{
  RDCWARN("Not implemented");
  return false;
}

DriverInformation MetalReplay::GetDriverInfo()
{
  RDCWARN("Not implemented");
  return DriverInformation();
}

rdcarray<GPUDevice> MetalReplay::GetAvailableGPUs()
{
  RDCWARN("Not implemented");
  return rdcarray<GPUDevice>();
}

/* IReplayDriver */
IReplayDriver *MetalReplay::MakeDummyDriver()
{
  RDCWARN("Not implemented");
  return NULL;
}

rdcarray<WindowingSystem> MetalReplay::GetSupportedWindowSystems()
{
  RDCWARN("Not implemented");
  return rdcarray<WindowingSystem>();
}

AMDRGPControl *MetalReplay::GetRGPControl()
{
  RDCWARN("Not implemented");
  return NULL;
}

uint64_t MetalReplay::MakeOutputWindow(WindowingData window, bool depth)
{
  RDCWARN("Not implemented");
  return 0;
}

void MetalReplay::DestroyOutputWindow(uint64_t id)
{
  RDCWARN("Not implemented");
}

bool MetalReplay::CheckResizeOutputWindow(uint64_t id)
{
  RDCWARN("Not implemented");
  return false;
}

void MetalReplay::SetOutputWindowDimensions(uint64_t id, int32_t w, int32_t h)
{
  RDCWARN("Not implemented");
}

void MetalReplay::GetOutputWindowDimensions(uint64_t id, int32_t &w, int32_t &h)
{
  RDCWARN("Not implemented");
}

void MetalReplay::GetOutputWindowData(uint64_t id, bytebuf &retData)
{
  RDCWARN("Not implemented");
}

void MetalReplay::ClearOutputWindowColor(uint64_t id, FloatVector col)
{
  RDCWARN("Not implemented");
}

void MetalReplay::ClearOutputWindowDepth(uint64_t id, float depth, uint8_t stencil)
{
  RDCWARN("Not implemented");
}

void MetalReplay::BindOutputWindow(uint64_t id, bool depth)
{
  RDCWARN("Not implemented");
}

bool MetalReplay::IsOutputWindowVisible(uint64_t id)
{
  RDCWARN("Not implemented");
  return false;
}

void MetalReplay::FlipOutputWindow(uint64_t id)
{
  RDCWARN("Not implemented");
}

bool MetalReplay::GetMinMax(ResourceId texid, const Subresource &sub, CompType typeCast,
                            float *minval, float *maxval)
{
  RDCWARN("Not implemented");
  return false;
}

bool MetalReplay::GetHistogram(ResourceId texid, const Subresource &sub, CompType typeCast,
                               float minval, float maxval, const rdcfixedarray<bool, 4> &channels,
                               rdcarray<uint32_t> &histogram)
{
  RDCWARN("Not implemented");
  return false;
}

void MetalReplay::PickPixel(ResourceId texture, uint32_t x, uint32_t y, const Subresource &sub,
                            CompType typeCast, float pixel[4])
{
  RDCWARN("Not implemented");
}

ResourceId MetalReplay::CreateProxyTexture(const TextureDescription &templateTex)
{
  RDCWARN("Not implemented");
  return ResourceId();
}

void MetalReplay::SetProxyTextureData(ResourceId texid, const Subresource &sub, byte *data,
                                      size_t dataSize)
{
  RDCWARN("Not implemented");
}

bool MetalReplay::IsTextureSupported(const TextureDescription &tex)
{
  RDCWARN("Not implemented");
  return false;
}

ResourceId MetalReplay::CreateProxyBuffer(const BufferDescription &templateBuf)
{
  RDCWARN("Not implemented");
  return ResourceId();
}

void MetalReplay::SetProxyBufferData(ResourceId bufid, byte *data, size_t dataSize)
{
  RDCWARN("Not implemented");
}

void MetalReplay::RenderMesh(uint32_t eventId, const rdcarray<MeshFormat> &secondaryDraws,
                             const MeshDisplay &cfg)
{
  RDCWARN("Not implemented");
}

bool MetalReplay::RenderTexture(TextureDisplay cfg)
{
  RDCWARN("Not implemented");
  return false;
}

void MetalReplay::SetCustomShaderIncludes(const rdcarray<rdcstr> &directories)
{
  RDCWARN("Not implemented");
}

void MetalReplay::BuildCustomShader(ShaderEncoding sourceEncoding, const bytebuf &source,
                                    const rdcstr &entry, const ShaderCompileFlags &compileFlags,
                                    ShaderStage type, ResourceId &id, rdcstr &errors)
{
  RDCWARN("Not implemented");
}

rdcarray<ShaderEncoding> MetalReplay::GetCustomShaderEncodings()
{
  RDCWARN("Not implemented");
  return rdcarray<ShaderEncoding>();
}

ResourceId MetalReplay::ApplyCustomShader(TextureDisplay &display)
{
  RDCWARN("Not implemented");
  return ResourceId();
}

void MetalReplay::FreeCustomShader(ResourceId id)
{
  RDCWARN("Not implemented");
}

void MetalReplay::RenderCheckerboard(FloatVector dark, FloatVector light)
{
  RDCWARN("Not implemented");
}

void MetalReplay::RenderHighlightBox(float w, float h, float scale)
{
  RDCWARN("Not implemented");
}

uint32_t MetalReplay::PickVertex(uint32_t eventId, int32_t width, int32_t height,
                                 const MeshDisplay &cfg, uint32_t x, uint32_t y)
{
  RDCWARN("Not implemented");
  return 0;
}

ReplayStatus Metal_CreateReplayDevice(RDCFile *rdc, const ReplayOptions &opts, IReplayDriver **driver)
{
  RDCDEBUG("Creating a MetalReplay replay device");

  // TODO: Could manually load the Metal framework here
  MetalInitParams initParams;
  uint64_t ver = MetalInitParams::CurrentVersion;

  // if we have an RDCFile, open the frame capture section and serialise the init params.
  // if not, we're creating a proxy-capable device so use default-initialised init params.
  if(rdc)
  {
    int sectionIdx = rdc->SectionIndex(SectionType::FrameCapture);

    if(sectionIdx < 0)
      return ReplayStatus::InternalError;

    ver = rdc->GetSectionProperties(sectionIdx).version;

    if(!MetalInitParams::IsSupportedVersion(ver))
    {
      RDCERR("Incompatible Metal serialise version %llu", ver);
      return ReplayStatus::APIIncompatibleVersion;
    }

    StreamReader *reader = rdc->ReadSection(sectionIdx);

    ReadSerialiser ser(reader, Ownership::Stream);

    ser.SetVersion(ver);

    SystemChunk chunk = ser.ReadChunk<SystemChunk>();

    if(chunk != SystemChunk::DriverInit)
    {
      RDCERR("Expected to get a DriverInit chunk, instead got %u", chunk);
      return ReplayStatus::FileCorrupted;
    }

    SERIALISE_ELEMENT(initParams);

    if(ser.IsErrored())
    {
      RDCERR("Failed reading driver init params.");
      return ReplayStatus::FileIOFailed;
    }
  }

  const bool isProxy = (rdc == NULL);

  WrappedMTLDevice *mtlDevice = new WrappedMTLDevice();

  MetalReplay *replay = mtlDevice->GetReplay();
  replay->SetProxy(isProxy);

  // TODO: implement RD MTL replay
  // ReplayStatus status = mtlDevice->Initialise(initParams, ver, opts);
  ReplayStatus status = ReplayStatus::APIUnsupported;

  if(status != ReplayStatus::Succeeded)
  {
    delete mtlDevice;
    return status;
  }

  RDCLOG("Created device.");
  *driver = (IReplayDriver *)replay;

  return ReplayStatus::Succeeded;
}

struct MetalDriverRegistration
{
  MetalDriverRegistration()
  {
    RenderDoc::Inst().RegisterReplayProvider(RDCDriver::Metal, &Metal_CreateReplayDevice);
  }
};

static MetalDriverRegistration MetalDriverRegistration;
