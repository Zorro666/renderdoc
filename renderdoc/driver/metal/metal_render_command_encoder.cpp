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

#include "metal_render_command_encoder.h"
#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_manager.h"
#include "metal_render_pipeline_state.h"
#include "metal_texture.h"

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(
    MTL::RenderCommandEncoder *realMTLRenderCommandEncoder, ResourceId objId,
    WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLRenderCommandEncoder, objId, wrappedMTLDevice,
                       wrappedMTLDevice->GetStateRef())
{
  AllocateObjCBridge(this);
}

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  m_ObjcBridge = NULL;
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_setRenderPipelineState(
    SerialiserType &ser, WrappedMTLRenderPipelineState *pipelineState)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, this);
  SERIALISE_ELEMENT(pipelineState).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(RenderCommandEncoder->m_CommandBuffer);
    // TODO: should this be the live ID
    ResourceId pipelineID = GetResID(pipelineState);

    if(IsLoading(m_State))
    {
      // m_Device->SetActiveRenderPipeline(pipelineID);

      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::PassBoundary;
      action.flags |= ActionFlags::BeginPass;

      AddAction(action);
    }
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    RDCLOG("M %s setRenderPipelineState",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(RenderCommandEncoder))).c_str());
    WrappedMTLRenderCommandEncoder *renderEncoder = m_Device->GetCurrentReplayRenderEncoder();
    Unwrap(renderEncoder)->setRenderPipelineState(Unwrap(pipelineState));
    MetalRenderState &renderstate = m_Device->GetCmdRenderState();
    renderstate.graphics.pipeline = pipelineID;
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setRenderPipelineState(WrappedMTLRenderPipelineState *pipelineState)
{
  SERIALISE_TIME_CALL(Unwrap(this)->setRenderPipelineState(Unwrap(pipelineState)));

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setRenderPipelineState);
      Serialise_setRenderPipelineState(ser, pipelineState);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(m_CommandBuffer);
    bufferRecord->AddChunk(chunk);
    bufferRecord->MarkResourceFrameReferenced(GetResID(pipelineState), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_setVertexBuffer(SerialiserType &ser,
                                                               WrappedMTLBuffer *buffer,
                                                               NS::UInteger offset,
                                                               NS::UInteger index)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, this);
  SERIALISE_ELEMENT(buffer).Important();
  SERIALISE_ELEMENT(offset);
  SERIALISE_ELEMENT(index).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(RenderCommandEncoder->m_CommandBuffer);
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    RDCLOG("M %s setVertexBuffer",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(RenderCommandEncoder))).c_str());
    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //    }
    WrappedMTLRenderCommandEncoder *renderEncoder = m_Device->GetCurrentReplayRenderEncoder();
    Unwrap(renderEncoder)->setVertexBuffer(Unwrap(buffer), offset, index);
    MetalRenderState &renderState = m_Device->GetCmdRenderState();
    renderState.graphics.vertexBuffers.push_back({GetResID(buffer), offset, index});
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setVertexBuffer(WrappedMTLBuffer *buffer, NS::UInteger offset,
                                                     NS::UInteger index)
{
  SERIALISE_TIME_CALL(Unwrap(this)->setVertexBuffer(Unwrap(buffer), offset, index));

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setVertexBuffer);
      Serialise_setVertexBuffer(ser, buffer, offset, index);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(m_CommandBuffer);
    bufferRecord->AddChunk(chunk);
    bufferRecord->MarkResourceFrameReferenced(GetResID(buffer), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_setFragmentBuffer(SerialiserType &ser,
                                                                 WrappedMTLBuffer *buffer,
                                                                 NS::UInteger offset,
                                                                 NS::UInteger index)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, this);
  SERIALISE_ELEMENT(buffer).Important();
  SERIALISE_ELEMENT(offset);
  SERIALISE_ELEMENT(index).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(RenderCommandEncoder->m_CommandBuffer);
    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //    }
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    RDCLOG("M %s setFragmentBuffer",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(RenderCommandEncoder))).c_str());
    WrappedMTLRenderCommandEncoder *renderEncoder = m_Device->GetCurrentReplayRenderEncoder();
    Unwrap(renderEncoder)->setFragmentBuffer(Unwrap(buffer), offset, index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setFragmentBuffer(WrappedMTLBuffer *buffer,
                                                       NS::UInteger offset, NS::UInteger index)
{
  SERIALISE_TIME_CALL(Unwrap(this)->setFragmentBuffer(Unwrap(buffer), offset, index));

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setFragmentBuffer);
      Serialise_setFragmentBuffer(ser, buffer, offset, index);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(m_CommandBuffer);
    bufferRecord->AddChunk(chunk);
    bufferRecord->MarkResourceFrameReferenced(GetResID(buffer), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_setFragmentTexture(SerialiserType &ser,
                                                                  WrappedMTLTexture *texture,
                                                                  NS::UInteger index)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, this);
  SERIALISE_ELEMENT(texture).Important();
  SERIALISE_ELEMENT(index).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(RenderCommandEncoder->m_CommandBuffer);
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //    }
    RDCLOG("M %s setFragmentTexture",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(RenderCommandEncoder))).c_str());
    WrappedMTLRenderCommandEncoder *renderEncoder = m_Device->GetCurrentReplayRenderEncoder();
    Unwrap(renderEncoder)->setFragmentTexture(Unwrap(texture), index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setFragmentTexture(WrappedMTLTexture *texture, NS::UInteger index)
{
  SERIALISE_TIME_CALL(Unwrap(this)->setFragmentTexture(Unwrap(texture), index));

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setFragmentTexture);
      Serialise_setFragmentTexture(ser, texture, index);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(m_CommandBuffer);
    bufferRecord->AddChunk(chunk);
    bufferRecord->MarkResourceFrameReferenced(GetResID(texture), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_setViewport(SerialiserType &ser,
                                                           MTL::Viewport &viewport)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, this);
  SERIALISE_ELEMENT(viewport).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(RenderCommandEncoder->m_CommandBuffer);
    //    if(IsLoading(m_State))
    //    {
    //      AddEvent();
    //    }
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    RDCLOG("M %s setViewport",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(RenderCommandEncoder))).c_str());
    WrappedMTLRenderCommandEncoder *renderEncoder = m_Device->GetCurrentReplayRenderEncoder();
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setViewport(MTL::Viewport &viewport)
{
  SERIALISE_TIME_CALL(Unwrap(this)->setViewport(viewport));

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setViewport);
      Serialise_setViewport(ser, viewport);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(m_CommandBuffer);
    bufferRecord->AddChunk(chunk);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_drawPrimitives(
    SerialiserType &ser, MTL::PrimitiveType primitiveType, NS::UInteger vertexStart,
    NS::UInteger vertexCount, NS::UInteger instanceCount, NS::UInteger baseInstance)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, this);
  SERIALISE_ELEMENT(primitiveType);
  SERIALISE_ELEMENT(vertexStart);
  SERIALISE_ELEMENT(vertexCount).Important();
  SERIALISE_ELEMENT(instanceCount);
  SERIALISE_ELEMENT(baseInstance);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(RenderCommandEncoder->m_CommandBuffer);
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::Drawcall;

      AddAction(action);
    }
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    RDCLOG("M %s drawPrimitives",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(RenderCommandEncoder))).c_str());
    WrappedMTLRenderCommandEncoder *renderEncoder = m_Device->GetCurrentReplayRenderEncoder();
    Unwrap(renderEncoder)
        ->drawPrimitives(primitiveType, vertexStart, vertexCount, instanceCount, baseInstance);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::drawPrimitives(MTL::PrimitiveType primitiveType,
                                                    NS::UInteger vertexStart,
                                                    NS::UInteger vertexCount,
                                                    NS::UInteger instanceCount,
                                                    NS::UInteger baseInstance)
{
  SERIALISE_TIME_CALL(Unwrap(this)->drawPrimitives(primitiveType, vertexStart, vertexCount,
                                                   instanceCount, baseInstance));

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_drawPrimitives_instanced);
      Serialise_drawPrimitives(ser, primitiveType, vertexStart, vertexCount, instanceCount,
                               baseInstance);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(m_CommandBuffer);
    bufferRecord->AddChunk(chunk);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

void WrappedMTLRenderCommandEncoder::drawPrimitives(MTL::PrimitiveType primitiveType,
                                                    NS::UInteger vertexStart,
                                                    NS::UInteger vertexCount)
{
  drawPrimitives(primitiveType, vertexStart, vertexCount, 1, 0);
}

void WrappedMTLRenderCommandEncoder::drawPrimitives(MTL::PrimitiveType primitiveType,
                                                    NS::UInteger vertexStart,
                                                    NS::UInteger vertexCount,
                                                    NS::UInteger instanceCount)
{
  drawPrimitives(primitiveType, vertexStart, vertexCount, instanceCount, 0);
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_endEncoding(SerialiserType &ser)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, this);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_Device->SetCurrentCommandBuffer(RenderCommandEncoder->m_CommandBuffer);
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::PassBoundary;
      action.flags |= ActionFlags::EndPass;

      AddAction(action);
    }
    if(IsActiveReplaying(m_State))
    {
      if(!m_Device->IsCurrentCommandBufferEventInReplayRange())
        return true;
    }
    WrappedMTLRenderCommandEncoder *renderEncoder = m_Device->GetCurrentReplayRenderEncoder();
    RDCASSERT(renderEncoder);
    RDCLOG("M %s endEncoding",
           ToStr(GetResourceManager()->GetOriginalID(GetResID(RenderCommandEncoder))).c_str());
    Unwrap(renderEncoder)->endEncoding();
    m_Device->ClearActiveRenderCommandEncoder();
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::endEncoding()
{
  SERIALISE_TIME_CALL(Unwrap(this)->endEncoding());

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_endEncoding);
      Serialise_endEncoding(ser);
      chunk = scope.Get();
    }
    MetalResourceRecord *bufferRecord = GetRecord(m_CommandBuffer);
    bufferRecord->AddChunk(chunk);
  }
  else
  {
    RDCLOG("M %s endEncoding direct", ToStr(GetResID(this)).c_str());
  }
}

INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLRenderCommandEncoder, void, endEncoding);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLRenderCommandEncoder, void, setRenderPipelineState,
                                WrappedMTLRenderPipelineState *pipelineState);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLRenderCommandEncoder, void, setVertexBuffer,
                                WrappedMTLBuffer *buffer, NS::UInteger offset, NS::UInteger index);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLRenderCommandEncoder, void, setFragmentBuffer,
                                WrappedMTLBuffer *buffer, NS::UInteger offset, NS::UInteger index);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLRenderCommandEncoder, void, setFragmentTexture,
                                WrappedMTLTexture *texture, NS::UInteger index);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLRenderCommandEncoder, void, setViewport,
                                MTL::Viewport &viewport);
INSTANTIATE_FUNCTION_SERIALISED(WrappedMTLRenderCommandEncoder, void, drawPrimitives,
                                MTL::PrimitiveType primitiveType, NS::UInteger vertexStart,
                                NS::UInteger vertexCount, NS::UInteger instanceCount,
                                NS::UInteger baseInstance);
