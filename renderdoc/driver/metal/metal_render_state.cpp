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

#include "metal_render_state.h"
#include "metal_blit_command_encoder.h"
#include "metal_command_buffer.h"
#include "metal_debug_manager.h"
#include "metal_render_command_encoder.h"

void MetalStatePipeline::Init()
{
  pipeline = ResourceId();
}

MetalRenderState::MetalRenderState()
{
  Init();
}

void MetalRenderState::Init()
{
  graphics.Init();
  renderPass = ResourceId();
  viewports.clear();
  scissors.clear();
  blendColor[0] = 1.0f;
  blendColor[1] = 1.0f;
  blendColor[2] = 1.0f;
  blendColor[3] = 1.0f;
  depthClipMode = MTL::DepthClipModeClip;
  frontFacingWinding = MTL::WindingClockwise;
  fillMode = MTL::TriangleFillModeFill;
  cullMode = MTL::CullModeNone;
  depthStencilState = ResourceId();
  //  MTLDepthStencilDescriptor
  //  MTLCompareFunction depthCompareFunction;
  //  bool depthWriteEnabled;
  //  MTLStencilDescriptor frontFaceStencil;
  //  MTLStencilDescriptor backFaceStencil;
}

void MetalRenderState::BeginBlitPass(WrappedMTLDevice *device, WrappedMTLCommandBuffer *cmdBuffer)
{
  RDCASSERT(!blitCommandEncoder);
  RDCASSERT(!renderCommandEncoder);
  blitCommandEncoder = cmdBuffer->blitCommandEncoder();
}

void MetalRenderState::EndBlitPass()
{
  blitCommandEncoder->endEncoding();
  blitCommandEncoder = NULL;
}

void MetalRenderState::BeginRenderPassAndApplyState(WrappedMTLDevice *device,
                                                    WrappedMTLCommandBuffer *cmdBuffer,
                                                    PipelineBinding binding)
{
  RDCASSERT(!blitCommandEncoder);
  RDCASSERT(!renderCommandEncoder);
  const MetalCreationInfo::RenderPass &rpInfo =
      device->GetDebugManager()->GetRenderPassInfo(renderPass);

  RDMTL::RenderPassDescriptor descriptor;
  rpInfo.CopyTo(device->GetResourceManager(), descriptor);
  descriptor.colorAttachments[0].loadAction = MTL::LoadActionLoad;
  renderCommandEncoder = cmdBuffer->renderCommandEncoderWithDescriptor(descriptor);
  if(graphics.pipeline != ResourceId())
  {
    BindPipeline(device, cmdBuffer, binding);
  }
}

void MetalRenderState::BindPipeline(WrappedMTLDevice *device, WrappedMTLCommandBuffer *cmdBuffer,
                                    PipelineBinding binding)
{
  const MetalCreationInfo::Pipeline pipeInfo =
      device->GetDebugManager()->GetPipelineInfo(graphics.pipeline);

  RDMTL::RenderPipelineDescriptor descriptor;
  pipeInfo.CopyTo(device->GetResourceManager(), descriptor);

  NS::Error *error;
  WrappedMTLRenderPipelineState *pipelineState =
      device->newRenderPipelineStateWithDescriptor(descriptor, &error);
  renderCommandEncoder->setRenderPipelineState(pipelineState);

  for(int i = 0; i < graphics.vertexBuffers.count(); ++i)
  {
    MetalStatePipeline::BoundBuffer &vertexBuffer = graphics.vertexBuffers[i];
    WrappedMTLBuffer *buffer =
        device->GetResourceManager()->GetCurrentResourceTyped<MTL::Buffer *>(vertexBuffer.buffer);
    renderCommandEncoder->setVertexBuffer(buffer, vertexBuffer.offset, vertexBuffer.index);
  }
  for(int i = 0; i < graphics.fragmentBuffers.count(); ++i)
  {
    MetalStatePipeline::BoundBuffer &fragmentBuffer = graphics.fragmentBuffers[i];
    WrappedMTLBuffer *buffer =
        device->GetResourceManager()->GetCurrentResourceTyped<MTL::Buffer *>(fragmentBuffer.buffer);
    renderCommandEncoder->setFragmentBuffer(buffer, fragmentBuffer.offset, fragmentBuffer.index);
  }
  for(int i = 0; i < graphics.fragmentTextures.count(); ++i)
  {
    MetalStatePipeline::BoundTexture &fragmentTexture = graphics.fragmentTextures[i];
    WrappedMTLTexture *texture =
        device->GetResourceManager()->GetCurrentResourceTyped<MTL::Texture *>(fragmentTexture.texture);
    renderCommandEncoder->setFragmentTexture(texture, fragmentTexture.index);
  }
}

void MetalRenderState::EndRenderPass()
{
  renderCommandEncoder->endEncoding();
  renderCommandEncoder = NULL;
}
