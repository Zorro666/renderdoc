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

#include "metal_replay.h"
#include "serialise/rdcfile.h"
#include "metal_debug_manager.h"
#include "metal_device.h"

MetalReplay::MetalReplay(WrappedMTLDevice *wrappedMTLDevice)
{
  m_pDriver = wrappedMTLDevice;
  m_Proxy = false;
  m_MetalPipelineState = NULL;
  m_debugLibrary = NULL;
  m_checkerboardPipeline = NULL;

  m_OutputWinID = 1;
  m_ActiveWinID = 0;
  m_BindDepth = false;

  m_DebugWidth = m_DebugHeight = 1;

  RDCEraseEl(m_DriverInfo);
}

/* IRemoteDriver */
void MetalReplay::Shutdown()
{
  METAL_NOT_IMPLEMENTED("only partial implementation");
  ShutdownDebugRenderer();
  m_PickPixel.Init(m_pDriver);
  m_TexRender.Destroy(m_pDriver);
}

APIProperties MetalReplay::GetAPIProperties()
{
  METAL_NOT_IMPLEMENTED("only partial implementation");
  APIProperties ret;

  ret.pipelineType = GraphicsAPI::Metal;
  ret.localRenderer = GraphicsAPI::Metal;
  ret.degraded = false;
  ret.shadersMutable = false;
  ret.rgpCapture = false;
  ret.shaderDebugging = false;
  ret.pixelHistory = false;

  return ret;
}

ResourceDescription &MetalReplay::GetResourceDesc(ResourceId id)
{
  auto it = m_ResourceIdx.find(id);
  if(it == m_ResourceIdx.end())
  {
    m_ResourceIdx[id] = m_Resources.size();
    m_Resources.push_back(ResourceDescription());
    m_Resources.back().resourceId = id;
    return m_Resources.back();
  }

  return m_Resources[it->second];
}

rdcarray<ResourceDescription> MetalReplay::GetResources()
{
  return m_Resources;
}

rdcarray<BufferDescription> MetalReplay::GetBuffers()
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<BufferDescription>();
}

BufferDescription MetalReplay::GetBuffer(ResourceId id)
{
  METAL_NOT_IMPLEMENTED();
  return BufferDescription();
}

rdcarray<TextureDescription> MetalReplay::GetTextures()
{
  rdcarray<TextureDescription> texs;

  const MetalTextureStates &textureStates = m_pDriver->GetTextureStates();
  for(auto it = textureStates.begin(); it != textureStates.end(); ++it)
  {
    // skip textures that aren't from the capture
    if(m_pDriver->GetResourceManager()->GetOriginalID(it->first) == it->first)
      continue;

    texs.push_back(GetTexture(it->first));
  }

  return texs;
}

TextureDescription MetalReplay::GetTexture(ResourceId id)
{
  MetalCreationInfo::Texture &texInfo = m_pDriver->GetCreationInfo().m_Texture[id];

  TextureDescription ret = {};
  ret.resourceId = m_pDriver->GetResourceManager()->GetOriginalID(id);
  ret.arraysize = (uint32_t)texInfo.arrayLength;
  ret.creationFlags = texInfo.creationFlags;
  ret.cubemap = texInfo.cube;
  ret.width = (uint32_t)texInfo.width;
  ret.height = (uint32_t)texInfo.height;
  ret.depth = (uint32_t)texInfo.depth;
  ret.mips = (uint32_t)texInfo.mipmapLevelCount;

  ret.byteSize = 0;
  for(uint32_t s = 0; s < ret.mips; s++)
    ret.byteSize += GetByteSize(ret.width, ret.height, ret.depth, texInfo.pixelFormat, s);
  ret.byteSize *= ret.arraysize;

  ret.msQual = 0;
  ret.msSamp = RDCMAX(1U, (uint32_t)texInfo.sampleCount);

  ret.byteSize *= ret.msSamp;

  ret.format = MakeResourceFormat(texInfo.pixelFormat);

  switch(texInfo.textureType)
  {
    case MTL::TextureType1D:
      ret.type = texInfo.arrayLength > 1 ? TextureType::Texture1DArray : TextureType::Texture1D;
      ret.dimension = 1;
      break;
    case MTL::TextureType2D:
      if(ret.msSamp > 1)
        ret.type = texInfo.arrayLength > 1 ? TextureType::Texture2DMSArray : TextureType::Texture2DMS;
      else if(ret.cubemap)
        ret.type = texInfo.arrayLength > 6 ? TextureType::TextureCubeArray : TextureType::TextureCube;
      else
        ret.type = texInfo.arrayLength > 1 ? TextureType::Texture2DArray : TextureType::Texture2D;
      ret.dimension = 2;
      break;
    case MTL::TextureType3D:
      ret.type = TextureType::Texture3D;
      ret.dimension = 3;
      break;
    default:
      ret.dimension = 2;
      RDCERR("Unexpected texture type");
      break;
  }

  return ret;
}

rdcarray<DebugMessage> MetalReplay::GetDebugMessages()
{
  METAL_NOT_IMPLEMENTED_ONCE();
  return rdcarray<DebugMessage>();
}

rdcarray<ShaderEntryPoint> MetalReplay::GetShaderEntryPoints(ResourceId shader)
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<ShaderEntryPoint>();
}

ShaderReflection *MetalReplay::GetShader(ResourceId pipeline, ResourceId shader,
                                         ShaderEntryPoint entry)
{
  METAL_NOT_IMPLEMENTED();
  return NULL;
}

rdcarray<rdcstr> MetalReplay::GetDisassemblyTargets(bool withPipeline)
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<rdcstr>();
}

rdcstr MetalReplay::DisassembleShader(ResourceId pipeline, const ShaderReflection *refl,
                                      const rdcstr &target)
{
  METAL_NOT_IMPLEMENTED();
  return rdcstr();
}

rdcarray<EventUsage> MetalReplay::GetUsage(ResourceId id)
{
  return m_pDriver->GetUsage(id);
}

void MetalReplay::SavePipelineState(uint32_t eventId)
{
  if(!m_MetalPipelineState)
    return;

  const MetalRenderState &state = m_pDriver->GetRenderState();
  MetalCreationInfo &c = m_pDriver->GetCreationInfo();

  MetalPipe::State &ret = *m_MetalPipelineState;

  MetalResourceManager *rm = m_pDriver->GetResourceManager();

  ret = MetalPipe::State();

  // General pipeline properties
  ret.compute.pipelineResourceId = ResourceId();
  ret.graphics.pipelineResourceId = rm->GetUnreplacedOriginalID(state.graphics.pipeline);

  {
    ret.compute.pipelineLayoutResourceId = ResourceId();
    ret.compute.flags = 0;
    ret.computeShader = MetalPipe::Shader();
  }

  if(state.graphics.pipeline != ResourceId())
  {
    const MetalCreationInfo::Pipeline &p = c.m_Pipeline[state.graphics.pipeline];

    // Viewport/Scissors
    size_t numViewScissors = state.viewports.size();
    ret.viewportScissor.viewportScissors.resize(numViewScissors);
    for(size_t i = 0; i < numViewScissors; i++)
    {
      if(i < state.viewports.size())
      {
        ret.viewportScissor.viewportScissors[i].vp.x = state.viewports[i].originX;
        ret.viewportScissor.viewportScissors[i].vp.y = state.viewports[i].originY;
        ret.viewportScissor.viewportScissors[i].vp.width = state.viewports[i].width;
        ret.viewportScissor.viewportScissors[i].vp.height = state.viewports[i].height;
        ret.viewportScissor.viewportScissors[i].vp.minDepth = state.viewports[i].znear;
        ret.viewportScissor.viewportScissors[i].vp.maxDepth = state.viewports[i].zfar;
      }
      else
      {
        RDCEraseEl(ret.viewportScissor.viewportScissors[i].vp);
      }

      if(i < state.scissors.size())
      {
        ret.viewportScissor.viewportScissors[i].scissor.x = (int32_t)state.scissors[i].x;
        ret.viewportScissor.viewportScissors[i].scissor.y = (int32_t)state.scissors[i].y;
        ret.viewportScissor.viewportScissors[i].scissor.width = (int32_t)state.scissors[i].width;
        ret.viewportScissor.viewportScissors[i].scissor.height = (int32_t)state.scissors[i].height;
      }
      else
      {
        RDCEraseEl(ret.viewportScissor.viewportScissors[i].scissor);
      }
    }

    // Rasterizer
    ret.rasterizer.depthClampEnable = (state.depthClipMode == MTL::DepthClipModeClamp);
    ret.rasterizer.depthClipEnable = (state.depthClipMode == MTL::DepthClipModeClip);
    ret.rasterizer.rasterizerDiscardEnable = false;
    ret.rasterizer.frontCCW = (state.frontFacingWinding == MTL::WindingCounterClockwise);

    ret.rasterizer.conservativeRasterization = ConservativeRaster::Disabled;
    ret.rasterizer.lineRasterMode = LineRaster::Default;

    switch(state.fillMode)
    {
      case MTL::TriangleFillModeLines: ret.rasterizer.fillMode = FillMode::Wireframe; break;
      case MTL::TriangleFillModeFill: ret.rasterizer.fillMode = FillMode::Solid; break;
      default:
        ret.rasterizer.fillMode = FillMode::Solid;
        RDCERR("Unexpected value for FillMode %x", state.fillMode);
        break;
    }

    switch(state.cullMode)
    {
      case MTL::CullModeNone: ret.rasterizer.cullMode = CullMode::NoCull; break;
      case MTL::CullModeFront: ret.rasterizer.cullMode = CullMode::Front; break;
      case MTL::CullModeBack: ret.rasterizer.cullMode = CullMode::Back; break;
      default:
        ret.rasterizer.cullMode = CullMode::NoCull;
        RDCERR("Unexpected value for CullMode %x", state.cullMode);
        break;
    }

    // Color Blend
    ret.colorBlend.alphaToCoverageEnable = p.alphaToCoverageEnabled;
    ret.colorBlend.alphaToOneEnable = p.alphaToOneEnabled;
    ret.rasterizer.rasterizerDiscardEnable = !p.rasterizationEnabled;

    ret.colorBlend.blends.resize(p.colorAttachments.size());
    for(size_t i = 0; i < p.colorAttachments.size(); i++)
    {
      const MetalCreationInfo::Pipeline::Attachment &colorAttachment = p.colorAttachments[i];
      ret.colorBlend.blends[i].enabled = colorAttachment.blendingEnabled;

      // TODO: RenderDoc Metal support for logic operation
      ret.colorBlend.blends[i].logicOperationEnabled = false;
      ret.colorBlend.blends[i].logicOperation = LogicOperation::NoOp;

      ret.colorBlend.blends[i].colorBlend.source =
          MakeBlendMultiplier(colorAttachment.sourceRGBBlendFactor);
      ret.colorBlend.blends[i].colorBlend.destination =
          MakeBlendMultiplier(colorAttachment.destinationRGBBlendFactor);
      ret.colorBlend.blends[i].colorBlend.operation = MakeBlendOp(colorAttachment.rgbBlendOperation);

      ret.colorBlend.blends[i].alphaBlend.source =
          MakeBlendMultiplier(colorAttachment.sourceAlphaBlendFactor);
      ret.colorBlend.blends[i].alphaBlend.destination =
          MakeBlendMultiplier(colorAttachment.destinationAlphaBlendFactor);
      ret.colorBlend.blends[i].alphaBlend.operation =
          MakeBlendOp(colorAttachment.alphaBlendOperation);

      ret.colorBlend.blends[i].writeMask = MakeWriteMask(colorAttachment.writeMask);
    }

    ret.colorBlend.blendFactor = state.blendColor;

    // Depth Stencil
    // TODO: Depth Stencil state saving
    //    ret.depthStencil.depthTestEnable = state.depthTestEnable != VK_FALSE;
    //    ret.depthStencil.depthWriteEnable = state.depthWriteEnable != VK_FALSE;
    //    ret.depthStencil.depthBoundsEnable = state.depthBoundsTestEnable != VK_FALSE;
    //    ret.depthStencil.depthFunction = MakeCompareFunc(state.depthCompareOp);
    //    ret.depthStencil.stencilTestEnable = state.stencilTestEnable != VK_FALSE;
    //
    //    ret.depthStencil.frontFace.passOperation = MakeStencilOp(state.front.passOp);
    //    ret.depthStencil.frontFace.failOperation = MakeStencilOp(state.front.failOp);
    //    ret.depthStencil.frontFace.depthFailOperation = MakeStencilOp(state.front.depthFailOp);
    //    ret.depthStencil.frontFace.function = MakeCompareFunc(state.front.compareOp);
    //
    //    ret.depthStencil.backFace.passOperation = MakeStencilOp(state.back.passOp);
    //    ret.depthStencil.backFace.failOperation = MakeStencilOp(state.back.failOp);
    //    ret.depthStencil.backFace.depthFailOperation = MakeStencilOp(state.back.depthFailOp);
    //    ret.depthStencil.backFace.function = MakeCompareFunc(state.back.compareOp);

    ret.depthStencil.minDepthBounds = 0.0f;
    ret.depthStencil.maxDepthBounds = 1.0f;

    //    ret.depthStencil.frontFace.reference = state.front.ref;
    //    ret.depthStencil.frontFace.compareMask = state.front.readMask;
    //    ret.depthStencil.frontFace.writeMask = state.front.writeMask;
    //
    //    ret.depthStencil.backFace.reference = state.back.ref;
    //    ret.depthStencil.backFace.compareMask = state.back.compare;
    //    ret.depthStencil.backFace.writeMask = state.back.write;
  }
  else
  {
    ret.graphics.pipelineLayoutResourceId = ResourceId();

    ret.graphics.flags = 0;

    ret.vertexInput.attributes.clear();
    ret.vertexInput.bindings.clear();
    ret.vertexInput.vertexBuffers.clear();

    //    MetalPipe::Shader *stages[] = {
    //        &ret.vertexShader,   &ret.tessControlShader, &ret.tessEvalShader,
    //        &ret.geometryShader, &ret.fragmentShader,
    //    };
    //
    //    for(size_t i = 0; i < ARRAY_COUNT(stages); i++)
    //      *stages[i] = MetalPipe::Shader();

    ret.viewportScissor.viewportScissors.clear();
    ret.viewportScissor.discardRectangles.clear();
    ret.viewportScissor.discardRectanglesExclusive = true;

    ret.colorBlend.blends.clear();
  }

  if(state.renderPass != ResourceId())
  {
    // Renderpass
    const MetalCreationInfo::RenderPass &rp = c.m_RenderPass[state.renderPass];
    ret.currentPass.renderpass.resourceId = rm->GetOriginalID(state.renderPass);
    ret.currentPass.renderpass.subpass = 0;
    //      ret.currentPass.renderpass.inputAttachments = rp.inputAttachments;
    ret.currentPass.renderpass.colorAttachments.resize(1);
    ret.currentPass.renderpass.colorAttachments[0] = 0;
    //      ret.currentPass.renderpass.resolveAttachments = rp.resolveAttachments;
    //      ret.currentPass.renderpass.depthstencilAttachment = rp.depthstencilAttachment;
    //      ret.currentPass.renderpass.fragmentDensityAttachment = rp.fragmentDensityAttachment;
    //      ret.currentPass.renderpass.multiviews = rpmultiviews;

    const int countAttachments = rp.colorAttachments.count();
    ret.currentPass.framebuffer.attachments.resize(countAttachments);
    for(int i = 0; i < countAttachments; i++)
    {
      ResourceId texID = rp.colorAttachments[i].texture;
      //          ret.currentPass.framebuffer.attachments[i].viewResourceId =
      //          rm->GetOriginalID(viewid);
      ret.currentPass.framebuffer.attachments[i].imageResourceId = rm->GetOriginalID(texID);
      ret.currentPass.framebuffer.attachments[i].viewFormat =
          MakeResourceFormat(c.m_Texture[texID].pixelFormat);
      //          ret.currentPass.framebuffer.attachments[i].firstMip =
      //              c.m_ImageView[viewid].range.baseMipLevel;
      //          ret.currentPass.framebuffer.attachments[i].firstSlice =
      //              c.m_ImageView[viewid].range.baseArrayLayer;
      //          ret.currentPass.framebuffer.attachments[i].numMips =
      //          c.m_ImageView[viewid].range.levelCount;
      //          ret.currentPass.framebuffer.attachments[i].numSlices =
      //              c.m_ImageView[viewid].range.layerCount;
      //
      //          Convert(ret.currentPass.framebuffer.attachments[i].swizzle,
      //                  c.m_ImageView[viewid].componentMapping);
    }
    if(state.graphics.pipeline != ResourceId())
    {
      const MetalCreationInfo::Pipeline &p = c.m_Pipeline[state.graphics.pipeline];
      //      ret.currentPass.framebuffer.width = c.m_Framebuffer[fb].width;
      //      ret.currentPass.framebuffer.height = c.m_Framebuffer[fb].height;
      //      ret.currentPass.framebuffer.layers = c.m_Framebuffer[fb].layers;
    }
    else
    {
      ret.currentPass.framebuffer.width = 0;
      ret.currentPass.framebuffer.height = 0;
      ret.currentPass.framebuffer.layers = 0;
    }

    //    ret.currentPass.renderArea.x = state.renderArea.offset.x;
    //    ret.currentPass.renderArea.y = state.renderArea.offset.y;
    //    ret.currentPass.renderArea.width = state.renderArea.extent.width;
    //    ret.currentPass.renderArea.height = state.renderArea.extent.height;
  }
  else
  {
    ret.currentPass.renderpass.resourceId = ResourceId();
    ret.currentPass.renderpass.subpass = 0;
    ret.currentPass.renderpass.inputAttachments.clear();
    ret.currentPass.renderpass.colorAttachments.clear();
    ret.currentPass.renderpass.resolveAttachments.clear();
    ret.currentPass.renderpass.depthstencilAttachment = -1;
    ret.currentPass.renderpass.fragmentDensityAttachment = -1;

    ret.currentPass.framebuffer.attachments.clear();
  }

  // image layouts
  {
    size_t i = 0;
    MetalTextureStates &textureStates = m_pDriver->GetTextureStates();
    ret.images.resize(textureStates.size());
    for(auto it = textureStates.begin(); it != textureStates.end(); ++it)
    {
      MetalPipe::ImageData &image = ret.images[i];

      if(rm->GetOriginalID(it->first) == it->first)
        continue;

      image.resourceId = rm->GetOriginalID(it->first);

      MetalTextureState &imState = it->second;
      image.layouts.push_back(MetalPipe::ImageLayout());
      image.layouts[0].name = "Unknown";

      i++;
    }

    ret.images.resize(i);
  }
}

FrameRecord MetalReplay::GetFrameRecord()
{
  return m_FrameRecord;
}

RDResult MetalReplay::ReadLogInitialisation(RDCFile *rdc, bool storeStructuredBuffers)
{
  return m_pDriver->ReadLogInitialisation(rdc, storeStructuredBuffers);
}

void MetalReplay::ReplayLog(uint32_t endEventID, ReplayLogType replayType)
{
  m_pDriver->ReplayLog(0, endEventID, replayType);
}

SDFile *MetalReplay::GetStructuredFile()
{
  return m_pDriver->GetStructuredFile();
}

rdcarray<uint32_t> MetalReplay::GetPassEvents(uint32_t eventId)
{
  rdcarray<uint32_t> passEvents;

  const ActionDescription *action = m_pDriver->GetAction(eventId);

  if(!action)
    return passEvents;

  // for Metal a pass == a render encoder scope ie. renderpass, if we're not inside a
  // renderpass then there are no pass events.
  const ActionDescription *start = action;
  while(start)
  {
    // if we've come to the beginning of a pass, break out of the loop, we've
    // found the start.
    if(start->flags & ActionFlags::BeginPass)
      break;

    // if we come to the END of a pass, since we were iterating backwards that
    // means we started outside of a pass, so return empty set.
    if(start->flags & ActionFlags::EndPass)
      return passEvents;

    // if we've come to the start of the log we were outside of a render pass
    // to start with
    if(start->previous == NULL)
      return passEvents;

    // step back
    start = start->previous;
  }

  // store all the action eventIDs up to the one specified at the start
  while(start)
  {
    if(start->eventId >= action->eventId)
      break;

    // include pass boundaries, these will be filtered out later
    // so we don't actually do anything (init postvs/action overlay)
    // but it's useful to have the first part of the pass as part
    // of the list
    if(start->flags & (ActionFlags::Drawcall | ActionFlags::PassBoundary))
      passEvents.push_back(start->eventId);

    start = start->next;
  }

  return passEvents;
}

void MetalReplay::InitPostVSBuffers(uint32_t eventId)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::InitPostVSBuffers(const rdcarray<uint32_t> &passEvents)
{
  METAL_NOT_IMPLEMENTED();
}

ResourceId MetalReplay::GetLiveID(ResourceId id)
{
  if(!m_pDriver->GetResourceManager()->HasLiveResource(id))
    return ResourceId();
  return m_pDriver->GetResourceManager()->GetLiveID(id);
}

MeshFormat MetalReplay::GetPostVSBuffers(uint32_t eventId, uint32_t instID, uint32_t viewID,
                                         MeshDataStage stage)
{
  METAL_NOT_IMPLEMENTED();
  return MeshFormat();
}

void MetalReplay::GetBufferData(ResourceId buff, uint64_t offset, uint64_t len, bytebuf &retData)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::GetTextureData(ResourceId tex, const Subresource &sub,
                                 const GetTextureDataParams &params, bytebuf &data)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::BuildTargetShader(ShaderEncoding sourceEncoding, const bytebuf &source,
                                    const rdcstr &entry, const ShaderCompileFlags &compileFlags,
                                    ShaderStage type, ResourceId &id, rdcstr &errors)
{
  METAL_NOT_IMPLEMENTED();
}

rdcarray<ShaderEncoding> MetalReplay::GetTargetShaderEncodings()
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<ShaderEncoding>();
}

void MetalReplay::ReplaceResource(ResourceId from, ResourceId to)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::RemoveReplacement(ResourceId id)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::FreeTargetResource(ResourceId id)
{
  METAL_NOT_IMPLEMENTED();
}

rdcarray<GPUCounter> MetalReplay::EnumerateCounters()
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<GPUCounter>();
}

CounterDescription MetalReplay::DescribeCounter(GPUCounter counterID)
{
  METAL_NOT_IMPLEMENTED();
  return CounterDescription();
}

rdcarray<CounterResult> MetalReplay::FetchCounters(const rdcarray<GPUCounter> &counterID)
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<CounterResult>();
}

void MetalReplay::FillCBufferVariables(ResourceId pipeline, ResourceId shader, ShaderStage stage,
                                       rdcstr entryPoint, uint32_t cbufSlot,
                                       rdcarray<ShaderVariable> &outvars, const bytebuf &data)
{
  METAL_NOT_IMPLEMENTED();
}

rdcarray<PixelModification> MetalReplay::PixelHistory(rdcarray<EventUsage> events,
                                                      ResourceId target, uint32_t x, uint32_t y,
                                                      const Subresource &sub, CompType typeCast)
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<PixelModification>();
}

ShaderDebugTrace *MetalReplay::DebugVertex(uint32_t eventId, uint32_t vertid, uint32_t instid,
                                           uint32_t idx, uint32_t view)
{
  METAL_NOT_IMPLEMENTED();
  return NULL;
}

ShaderDebugTrace *MetalReplay::DebugPixel(uint32_t eventId, uint32_t x, uint32_t y, uint32_t sample,
                                          uint32_t primitive)
{
  METAL_NOT_IMPLEMENTED();
  return NULL;
}

ShaderDebugTrace *MetalReplay::DebugThread(uint32_t eventId,
                                           const rdcfixedarray<uint32_t, 3> &groupid,
                                           const rdcfixedarray<uint32_t, 3> &threadid)
{
  METAL_NOT_IMPLEMENTED();
  return NULL;
}

rdcarray<ShaderDebugState> MetalReplay::ContinueDebug(ShaderDebugger *debugger)
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<ShaderDebugState>();
}

void MetalReplay::FreeDebugger(ShaderDebugger *debugger)
{
  METAL_NOT_IMPLEMENTED();
}

ResourceId MetalReplay::RenderOverlay(ResourceId texid, FloatVector clearCol, DebugOverlay overlay,
                                      uint32_t eventId, const rdcarray<uint32_t> &passEvents)
{
  METAL_NOT_IMPLEMENTED();
  return ResourceId();
}

bool MetalReplay::IsRenderOutput(ResourceId id)
{
  METAL_NOT_IMPLEMENTED();
  return false;
}

void MetalReplay::FileChanged()
{
  METAL_NOT_IMPLEMENTED();
}

RDResult MetalReplay::FatalErrorCheck()
{
  return m_pDriver->FatalErrorCheck();
}

bool MetalReplay::NeedRemapForFetch(const ResourceFormat &format)
{
  METAL_NOT_IMPLEMENTED();
  return false;
}

DriverInformation MetalReplay::GetDriverInfo()
{
  return m_DriverInfo;
}

rdcarray<GPUDevice> MetalReplay::GetAvailableGPUs()
{
  METAL_NOT_IMPLEMENTED();
  rdcarray<GPUDevice> ret;

  /*  NSArray<id<MTLDevice>> * MTLCopyAllDevices(void); */
  /* name */
  /* uint64 registryID */
  /* MTLDeviceLocation location */
  /* NSUInteger locationNumber */
  /* bool lowPower */
  /* bool headless */
  GPUDevice dev;
  dev.vendor = GPUVendor::Unknown;
  dev.deviceID = 0x1234;
  dev.name = "M1";
  dev.apis = {GraphicsAPI::Metal};
  dev.driver = "Metal 1.x";

  ret.push_back(dev);

  return ret;
}

/* IReplayDriver */
IReplayDriver *MetalReplay::MakeDummyDriver()
{
  METAL_NOT_IMPLEMENTED();
  return NULL;
}

AMDRGPControl *MetalReplay::GetRGPControl()
{
  return NULL;
}

bool MetalReplay::GetMinMax(ResourceId texid, const Subresource &sub, CompType typeCast,
                            float *minval, float *maxval)
{
  METAL_NOT_IMPLEMENTED();
  return false;
}

bool MetalReplay::GetHistogram(ResourceId texid, const Subresource &sub, CompType typeCast,
                               float minval, float maxval, const rdcfixedarray<bool, 4> &channels,
                               rdcarray<uint32_t> &histogram)
{
  METAL_NOT_IMPLEMENTED();
  return false;
}

void MetalReplay::PickPixel(ResourceId texture, uint32_t x, uint32_t y, const Subresource &sub,
                            CompType typeCast, float pixel[4])
{
  const MetalTextureState *textureState = m_pDriver->FindConstTextureState(texture);
  if(!textureState)
  {
    RDCWARN("Could not find texture info for texture %s", ToStr(texture).c_str());
    return;
  }
  MetalCreationInfo::Texture &texInfo = m_pDriver->GetCreationInfo().m_Texture[texture];

  TextureDisplay texDisplay;
  texDisplay.red = texDisplay.green = texDisplay.blue = texDisplay.alpha = true;
  texDisplay.hdrMultiplier = -1.0f;
  texDisplay.linearDisplayAsGamma = true;
  texDisplay.flipY = false;
  texDisplay.subresource = sub;
  texDisplay.customShaderId = ResourceId();
  texDisplay.overlay = DebugOverlay::NoOverlay;
  texDisplay.rangeMin = 0.0f;
  texDisplay.rangeMax = 1.0f;
  texDisplay.scale = 1.0f;
  texDisplay.resourceId = texture;
  texDisplay.typeCast = typeCast;
  texDisplay.rawOutput = true;

  uint32_t mipWidth = RDCMAX(1U, (uint32_t)texInfo.width >> sub.mip);
  uint32_t mipHeight = RDCMAX(1U, (uint32_t)texInfo.height >> sub.mip);

  texDisplay.xOffset = -(float(x) / float(mipWidth)) * texInfo.width;
  texDisplay.yOffset = -(float(y) / float(mipHeight)) * texInfo.height;

  int flags = eTexDisplay_32Render | eTexDisplay_MipShift;
  RenderTextureInternal(texDisplay, *textureState, m_PickPixel.PipePass.Pipeline,
                        m_PickPixel.PipePass.MTLPassDesc, m_PickPixel.CBuffer, flags);
  m_pDriver->SubmitCmds();
  m_pDriver->FlushQ();
  MTL::Region region = MTL::Region::Make2D(0, 0, 1, 1);
  Unwrap(m_PickPixel.FB)->getBytes(pixel, 16, region, 0);
}

ResourceId MetalReplay::CreateProxyTexture(const TextureDescription &templateTex)
{
  METAL_NOT_IMPLEMENTED();
  return ResourceId();
}

void MetalReplay::SetProxyTextureData(ResourceId texid, const Subresource &sub, byte *data,
                                      size_t dataSize)
{
  METAL_NOT_IMPLEMENTED();
}

bool MetalReplay::IsTextureSupported(const TextureDescription &tex)
{
  METAL_NOT_IMPLEMENTED();
  return false;
}

ResourceId MetalReplay::CreateProxyBuffer(const BufferDescription &templateBuf)
{
  METAL_NOT_IMPLEMENTED();
  return ResourceId();
}

void MetalReplay::SetProxyBufferData(ResourceId bufid, byte *data, size_t dataSize)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::RenderMesh(uint32_t eventId, const rdcarray<MeshFormat> &secondaryDraws,
                             const MeshDisplay &cfg)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::SetCustomShaderIncludes(const rdcarray<rdcstr> &directories)
{
  METAL_NOT_IMPLEMENTED();
}

void MetalReplay::BuildCustomShader(ShaderEncoding sourceEncoding, const bytebuf &source,
                                    const rdcstr &entry, const ShaderCompileFlags &compileFlags,
                                    ShaderStage type, ResourceId &id, rdcstr &errors)
{
  METAL_NOT_IMPLEMENTED();
}

rdcarray<ShaderEncoding> MetalReplay::GetCustomShaderEncodings()
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<ShaderEncoding>();
}

rdcarray<ShaderSourcePrefix> MetalReplay::GetCustomShaderSourcePrefixes()
{
  METAL_NOT_IMPLEMENTED();
  return rdcarray<ShaderSourcePrefix>();
}

ResourceId MetalReplay::ApplyCustomShader(TextureDisplay &display)
{
  METAL_NOT_IMPLEMENTED();
  return ResourceId();
}

void MetalReplay::FreeCustomShader(ResourceId id)
{
  METAL_NOT_IMPLEMENTED();
}

uint32_t MetalReplay::PickVertex(uint32_t eventId, int32_t width, int32_t height,
                                 const MeshDisplay &cfg, uint32_t x, uint32_t y)
{
  METAL_NOT_IMPLEMENTED();
  return 0;
}

void MetalReplay::CreateResources()
{
  SetDriverInfo();
  InitDebugRenderer();
  m_TexRender.Init(m_pDriver);
  m_PickPixel.Init(m_pDriver);
}

void MetalReplay::SetDriverInfo()
{
  m_DriverInfo.vendor = GPUVendor::Unknown;
  rdcstr versionString = Unwrap(m_pDriver)->name()->utf8String();
  versionString.resize(RDCMIN(versionString.size(), ARRAY_COUNT(m_DriverInfo.version) - 1));
  memcpy(m_DriverInfo.version, versionString.c_str(), versionString.size());
}

RDResult Metal_CreateReplayDevice(RDCFile *rdc, const ReplayOptions &opts, IReplayDriver **driver)
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
      RETURN_ERROR_RESULT(ResultCode::FileCorrupted, "File does not contain captured API data");

    ver = rdc->GetSectionProperties(sectionIdx).version;

    if(!MetalInitParams::IsSupportedVersion(ver))
    {
      RETURN_ERROR_RESULT(ResultCode::APIIncompatibleVersion,
                          "Metal capture is incompatible version %llu, newest supported by this "
                          "build of RenderDoc is %llu",
                          ver, MetalInitParams::CurrentVersion);
    }

    StreamReader *reader = rdc->ReadSection(sectionIdx);

    ReadSerialiser ser(reader, Ownership::Stream);

    ser.SetVersion(ver);

    SystemChunk chunk = ser.ReadChunk<SystemChunk>();

    if(chunk != SystemChunk::DriverInit)
    {
      RETURN_ERROR_RESULT(ResultCode::FileCorrupted,
                          "Expected to get a DriverInit chunk, instead got %u", chunk);
    }

    SERIALISE_ELEMENT(initParams);

    if(ser.IsErrored())
    {
      return ser.GetError();
    }
  }

  const bool isProxy = (rdc == NULL);

  WrappedMTLDevice *wrappedMTLDevice = new WrappedMTLDevice();

  MetalReplay *replay = wrappedMTLDevice->GetReplay();
  replay->SetProxy(isProxy);

  // TODO: implement RD MTL replay
  RDResult status = wrappedMTLDevice->Initialise(initParams, ver, opts);

  if(status != ResultCode::Succeeded)
  {
    delete wrappedMTLDevice;
    return status;
  }

  RDCLOG("Created device.");
  *driver = (IReplayDriver *)replay;

  return ResultCode::Succeeded;
}

struct MetalDriverRegistration
{
  MetalDriverRegistration()
  {
    RenderDoc::Inst().RegisterReplayProvider(RDCDriver::Metal, &Metal_CreateReplayDevice);
  }
};

static MetalDriverRegistration MetalDriverRegistration;
