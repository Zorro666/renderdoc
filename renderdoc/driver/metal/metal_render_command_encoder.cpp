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
#include "core/settings.h"
#include "metal_buffer.h"
#include "metal_command_buffer.h"
#include "metal_manager.h"
#include "metal_render_pipeline_state.h"
#include "metal_texture.h"

RDOC_EXTERN_CONFIG(bool, Metal_Debug_VerboseCommandRecording);

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(
    MTL::RenderCommandEncoder *realMTLRenderCommandEncoder, ResourceId objId,
    WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLRenderCommandEncoder, objId, wrappedMTLDevice,
                       wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = AllocateObjCWrapper(this);
}

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = NULL;
}

void WrappedMTLRenderCommandEncoder::SetWrappedMTLCommandBuffer(
    WrappedMTLCommandBuffer *wrappedMTLCommandBuffer)
{
  m_WrappedMTLCommandBuffer = wrappedMTLCommandBuffer;
}

void WrappedMTLRenderCommandEncoder::setRenderPipelineState(MTL::RenderPipelineState *pipelineState)
{
  SERIALISE_TIME_CALL(real_setRenderPipelineState(pipelineState));

  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);
    WrappedMTLRenderPipelineState *wrappedPipelineState = GetWrapped(pipelineState);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setRenderPipelineState);
      Serialise_mtlRenderCommandEncoder_setRenderPipelineState(ser, this, wrappedPipelineState);
      bufferRecord->AddChunk(scope.Get(bufferRecord->cmdInfo->alloc));
    }
    GetResourceManager()->MarkResourceFrameReferenced(GetResID(wrappedPipelineState), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_mtlRenderCommandEncoder_setRenderPipelineState(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder,
    WrappedMTLRenderPipelineState *pipelineState)
{
  RDCASSERT(m_WrappedMTLCommandBuffer);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT_LOCAL(RenderPipelineState, GetResID(pipelineState))
      .TypedAs("MTLRenderPipelineState"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    encoder =
        (WrappedMTLRenderCommandEncoder *)GetResourceManager()->GetLiveResource(RenderCommandEncoder);
    MTL::RenderPipelineState *renderPipelineState =
        GetWrappedObjCResource<MTL::RenderPipelineState *>(GetResourceManager(), RenderPipelineState);
    encoder->real_setRenderPipelineState(renderPipelineState);
    ResourceId pipelineID = GetResourceManager()->GetLiveID(RenderPipelineState);

    MetalRenderState &renderstate = m_WrappedMTLDevice->GetCmdRenderState();
    renderstate.graphics.pipeline = pipelineID;
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setVertexBuffer(MTL::Buffer *buffer, NS::UInteger offset,
                                                     NS::UInteger index)
{
  SERIALISE_TIME_CALL(real_setVertexBuffer(buffer, offset, index));

  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);
    WrappedMTLBuffer *wrappedMTLBuffer = GetWrapped(buffer);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setVertexBuffer);
      Serialise_mtlRenderCommandEncoder_setVertexBuffer(ser, this, wrappedMTLBuffer, offset, index);
      bufferRecord->AddChunk(scope.Get(bufferRecord->cmdInfo->alloc));
    }
    GetResourceManager()->MarkResourceFrameReferenced(GetResID(wrappedMTLBuffer), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_mtlRenderCommandEncoder_setVertexBuffer(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder, WrappedMTLBuffer *buffer,
    NS::UInteger offset, NS::UInteger index)
{
  RDCASSERT(m_WrappedMTLCommandBuffer);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT_LOCAL(Buffer, GetResID(buffer)).TypedAs("MTLBuffer"_lit);
  SERIALISE_ELEMENT(offset);
  SERIALISE_ELEMENT(index);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    encoder =
        (WrappedMTLRenderCommandEncoder *)GetResourceManager()->GetLiveResource(RenderCommandEncoder);
    MTL::Buffer *buffer = GetWrappedObjCResource<MTL::Buffer *>(GetResourceManager(), Buffer);
    encoder->real_setVertexBuffer(buffer, (NS::UInteger)offset, (NS::UInteger)index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setFragmentBuffer(MTL::Buffer *buffer, NS::UInteger offset,
                                                       NS::UInteger index)
{
  SERIALISE_TIME_CALL(real_setFragmentBuffer(buffer, offset, index));

  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);
    WrappedMTLBuffer *wrappedMTLBuffer = GetWrapped(buffer);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setFragmentBuffer);
      Serialise_mtlRenderCommandEncoder_setFragmentBuffer(ser, this, wrappedMTLBuffer, offset, index);
      bufferRecord->AddChunk(scope.Get(bufferRecord->cmdInfo->alloc));
    }
    GetResourceManager()->MarkResourceFrameReferenced(GetResID(wrappedMTLBuffer), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_mtlRenderCommandEncoder_setFragmentBuffer(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder, WrappedMTLBuffer *buffer,
    NS::UInteger offset, NS::UInteger index)
{
  RDCASSERT(m_WrappedMTLCommandBuffer);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT_LOCAL(Buffer, GetResID(buffer)).TypedAs("MTLBuffer"_lit);
  void *pointer = buffer ? buffer->real_contents() : NULL;
  uint64_t length = buffer ? buffer->real_length() : 0;
  SERIALISE_ELEMENT_ARRAY(pointer, length);
  SERIALISE_ELEMENT(length);
  SERIALISE_ELEMENT(offset);
  SERIALISE_ELEMENT(index);

  SERIALISE_CHECK_READ_ERRORS();
  // RDCASSERT(buffer->length() == length);

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    encoder =
        (WrappedMTLRenderCommandEncoder *)GetResourceManager()->GetLiveResource(RenderCommandEncoder);
    MTL::Buffer *buffer = GetWrappedObjCResource<MTL::Buffer *>(GetResourceManager(), Buffer);
    WrappedMTLBuffer *wrappedBuffer = GetWrapped(buffer);
    memcpy(wrappedBuffer->real_contents(), pointer, length);
    encoder->real_setFragmentBuffer(buffer, (NS::UInteger)offset, (NS::UInteger)index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setFragmentTexture(MTL::Texture *texture, NS::UInteger index)
{
  SERIALISE_TIME_CALL(real_setFragmentTexture(texture, index));

  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);
    WrappedMTLTexture *wrappedMTLTexture = GetWrapped(texture);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setFragmentTexture);
      Serialise_mtlRenderCommandEncoder_setFragmentTexture(ser, this, wrappedMTLTexture, index);
      bufferRecord->AddChunk(scope.Get(bufferRecord->cmdInfo->alloc));
    }
    GetResourceManager()->MarkResourceFrameReferenced(GetResID(wrappedMTLTexture), eFrameRef_Read);
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_mtlRenderCommandEncoder_setFragmentTexture(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder, WrappedMTLTexture *texture,
    NS::UInteger index)
{
  RDCASSERT(m_WrappedMTLCommandBuffer);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT_LOCAL(Texture, GetResID(texture)).TypedAs("MTLTexture"_lit);
  SERIALISE_ELEMENT(index);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    encoder =
        (WrappedMTLRenderCommandEncoder *)GetResourceManager()->GetLiveResource(RenderCommandEncoder);
    MTL::Texture *texture = GetWrappedObjCResource<MTL::Texture *>(GetResourceManager(), Texture);
    encoder->real_setFragmentTexture(texture, (NS::UInteger)index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setViewport(MTL::Viewport &viewport)
{
  SERIALISE_TIME_CALL(real_setViewport(viewport));

  if(IsCaptureMode(m_State))
  {
    METAL_NOT_IMPLEMENTED();
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

void WrappedMTLRenderCommandEncoder::drawPrimitives(MTL::PrimitiveType primitiveType,
                                                    NS::UInteger vertexStart,
                                                    NS::UInteger vertexCount,
                                                    NS::UInteger instanceCount)
{
  SERIALISE_TIME_CALL(real_drawPrimitives(primitiveType, vertexStart, vertexCount, instanceCount));

  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_drawPrimitives);
      Serialise_mtlRenderCommandEncoder_drawPrimitives(ser, this, primitiveType, vertexStart,
                                                       vertexCount, instanceCount);
      bufferRecord->AddChunk(scope.Get(bufferRecord->cmdInfo->alloc));
    }
  }
  else
  {
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_mtlRenderCommandEncoder_drawPrimitives(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder, MTL::PrimitiveType primitiveType,
    NS::UInteger vertexStart, NS::UInteger vertexCount, NS::UInteger instanceCount)
{
  RDCASSERT(m_WrappedMTLCommandBuffer);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT(primitiveType);
  SERIALISE_ELEMENT(vertexStart);
  SERIALISE_ELEMENT(vertexCount).Important();
  SERIALISE_ELEMENT(instanceCount);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    if(IsLoading(m_State))
    {
      AddEvent();

      ActionDescription action;
      action.flags |= ActionFlags::Drawcall;

      AddAction(action);
    }
    encoder =
        (WrappedMTLRenderCommandEncoder *)GetResourceManager()->GetLiveResource(RenderCommandEncoder);
    encoder->real_drawPrimitives(primitiveType, (NS::UInteger)vertexStart,
                                 (NS::UInteger)vertexCount, (NS::UInteger)instanceCount);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::endEncoding()
{
  SERIALISE_TIME_CALL(real_endEncoding());

  if(IsCaptureMode(m_State))
  {
    MetalResourceRecord *encoderRecord = GetRecord(this);
    MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

    if(Metal_Debug_VerboseCommandRecording())
    {
      RDCLOG("End RenderCommandEncoder %s CommandBuffer %s",
             ToStr(encoderRecord->GetResourceID()).c_str(),
             ToStr(bufferRecord->GetResourceID()).c_str());
    }

    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_endEncoding);
      Serialise_mtlRenderCommandEncoder_endEncoding(ser, this);
      bufferRecord->AddChunk(scope.Get(bufferRecord->cmdInfo->alloc));
    }
    bufferRecord->cmdInfo->isEncoding = false;
  }
  else
  {
    // TODO: should this be in "real_endEncoding"
    m_WrappedMTLDevice->SetRenderPassActiveState(WrappedMTLDevice::Primary, false);
    // TODO: ASSERT active render encoder == this encoder
    m_WrappedMTLDevice->SetActiveRenderCommandEnvoder(NULL);
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_mtlRenderCommandEncoder_endEncoding(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder)
{
  RDCASSERT(m_WrappedMTLCommandBuffer);
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    m_WrappedMTLDevice->SetRenderPassActiveState(WrappedMTLDevice::Primary, false);
    // TODO: ASSERT active render encoder == this encoder
    m_WrappedMTLDevice->SetActiveRenderCommandEnvoder(NULL);
    encoder =
        (WrappedMTLRenderCommandEncoder *)GetResourceManager()->GetLiveResource(RenderCommandEncoder);
    // TODO: should this call "endEncoding" to unify active render pass state tracking
    encoder->real_endEncoding();
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::real_setRenderPipelineState(MTL::RenderPipelineState *pipelineState)
{
  MTL::RenderPipelineState *realRenderPipelineState = GetReal(pipelineState);
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder = Unwrap<MTL::RenderCommandEncoder *>(this);
  realMTLRenderCommandEncoder->setRenderPipelineState(realRenderPipelineState);
}

void WrappedMTLRenderCommandEncoder::real_setVertexBuffer(MTL::Buffer *buffer, NS::UInteger offset,
                                                          NS::UInteger index)
{
  MTL::Buffer *realBuffer = GetReal(buffer);
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder = Unwrap<MTL::RenderCommandEncoder *>(this);
  realMTLRenderCommandEncoder->setVertexBuffer(realBuffer, offset, index);
}

void WrappedMTLRenderCommandEncoder::real_setFragmentBuffer(MTL::Buffer *buffer,
                                                            NS::UInteger offset, NS::UInteger index)
{
  MTL::Buffer *realBuffer = GetReal(buffer);
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder = Unwrap<MTL::RenderCommandEncoder *>(this);
  realMTLRenderCommandEncoder->setFragmentBuffer(realBuffer, offset, index);
}

void WrappedMTLRenderCommandEncoder::real_setFragmentTexture(MTL::Texture *texture, NS::UInteger index)
{
  MTL::Texture *realTexture = GetReal(texture);
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder = Unwrap<MTL::RenderCommandEncoder *>(this);
  realMTLRenderCommandEncoder->setFragmentTexture(realTexture, index);
}

void WrappedMTLRenderCommandEncoder::real_setViewport(MTL::Viewport &viewport)
{
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder = Unwrap<MTL::RenderCommandEncoder *>(this);
  realMTLRenderCommandEncoder->setViewport(viewport);
}

void WrappedMTLRenderCommandEncoder::real_drawPrimitives(MTL::PrimitiveType primitiveType,
                                                         NS::UInteger vertexStart,
                                                         NS::UInteger vertexCount,
                                                         NS::UInteger instanceCount)
{
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder = Unwrap<MTL::RenderCommandEncoder *>(this);
  realMTLRenderCommandEncoder->drawPrimitives(primitiveType, vertexStart, vertexCount, instanceCount);
}

void WrappedMTLRenderCommandEncoder::real_endEncoding()
{
  MTL::RenderCommandEncoder *realMTLRenderCommandEncoder = Unwrap<MTL::RenderCommandEncoder *>(this);
  realMTLRenderCommandEncoder->endEncoding();
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_endEncoding,
                                WrappedMTLRenderCommandEncoder *encoder);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_setRenderPipelineState,
                                WrappedMTLRenderCommandEncoder *encoder,
                                WrappedMTLRenderPipelineState *pipelineState);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_setVertexBuffer,
                                WrappedMTLRenderCommandEncoder *encoder, WrappedMTLBuffer *buffer,
                                NS::UInteger offset, NS::UInteger index);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_setFragmentBuffer,
                                WrappedMTLRenderCommandEncoder *encoder, WrappedMTLBuffer *buffer,
                                NS::UInteger offset, NS::UInteger index);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_setFragmentTexture,
                                WrappedMTLRenderCommandEncoder *encoder, WrappedMTLTexture *texture,
                                NS::UInteger index);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_drawPrimitives,
                                WrappedMTLRenderCommandEncoder *encoder,
                                MTL::PrimitiveType primitiveType, NS::UInteger vertexStart,
                                NS::UInteger vertexCount, NS::UInteger instanceCount);
