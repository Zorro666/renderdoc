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

#include "metal_render_command_encoder.h"
#include "core/core.h"
#include "core/settings.h"
#include "metal_manager.h"
#include "metal_metal.h"

RDOC_EXTERN_CONFIG(bool, Metal_Debug_VerboseCommandRecording);

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(
    id_MTLRenderCommandEncoder realMTLRenderCommandEncoder, ResourceId objId,
    WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLRenderCommandEncoder, objId, wrappedMTLDevice),
      m_State(wrappedMTLDevice->GetStateRef())
{
  objc = CreateObjCWrappedMTLRenderCommandEncoder();
}

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice), m_State(wrappedMTLDevice->GetStateRef())
{
  objc = CreateObjCWrappedMTLRenderCommandEncoder();
}

void WrappedMTLRenderCommandEncoder::SetWrappedMTLCommandBuffer(
    WrappedMTLCommandBuffer *wrappedMTLCommandBuffer)
{
  m_WrappedMTLCommandBuffer = wrappedMTLCommandBuffer;
}

void WrappedMTLRenderCommandEncoder::setRenderPipelineState(id_MTLRenderPipelineState pipelineState)
{
  SERIALISE_TIME_CALL(real_setRenderPipelineState(pipelineState));

  MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

  if(IsCaptureMode(m_State))
  {
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
    id_MTLRenderPipelineState renderPipelineState =
        GetObjCWrappedResource<id_MTLRenderPipelineState>(GetResourceManager(), RenderPipelineState);
    encoder->real_setRenderPipelineState(renderPipelineState);
    ResourceId pipelineID = GetResourceManager()->GetLiveID(RenderPipelineState);

    MetalRenderState &renderstate = m_WrappedMTLDevice->GetCmdRenderState();
    renderstate.graphics.pipeline = pipelineID;
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setVertexBuffer(id_MTLBuffer buffer, NSUInteger offset,
                                                     NSUInteger index)
{
  SERIALISE_TIME_CALL(real_setVertexBuffer(buffer, offset, index));

  MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

  if(IsCaptureMode(m_State))
  {
    WrappedMTLBuffer *wrappedMTLBuffer = GetWrapped(buffer);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setVertexBuffer);
      Serialise_mtlRenderCommandEncoder_setVertexBuffer(
          ser, this, wrappedMTLBuffer, (NSUInteger_objc)offset, (NSUInteger_objc)index);
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
    NSUInteger_objc offset, NSUInteger_objc index)
{
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
    id_MTLBuffer buffer = GetObjCWrappedResource<id_MTLBuffer>(GetResourceManager(), Buffer);
    encoder->real_setVertexBuffer(buffer, (NSUInteger)offset, (NSUInteger)index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setFragmentBuffer(id_MTLBuffer buffer, NSUInteger offset,
                                                       NSUInteger index)
{
  SERIALISE_TIME_CALL(real_setFragmentBuffer(buffer, offset, index));

  MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

  if(IsCaptureMode(m_State))
  {
    WrappedMTLBuffer *wrappedMTLBuffer = GetWrapped(buffer);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setFragmentBuffer);
      Serialise_mtlRenderCommandEncoder_setFragmentBuffer(
          ser, this, wrappedMTLBuffer, (NSUInteger_objc)offset, (NSUInteger_objc)index);
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
    NSUInteger_objc offset, NSUInteger_objc index)
{
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
    id_MTLBuffer buffer = GetObjCWrappedResource<id_MTLBuffer>(GetResourceManager(), Buffer);
    WrappedMTLBuffer *wrappedBuffer = GetWrapped(buffer);
    memcpy(wrappedBuffer->real_contents(), pointer, length);
    encoder->real_setFragmentBuffer(buffer, (NSUInteger)offset, (NSUInteger)index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setFragmentTexture(id_MTLTexture texture, NSUInteger index)
{
  SERIALISE_TIME_CALL(real_setFragmentTexture(texture, index));

  MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

  if(IsCaptureMode(m_State))
  {
    WrappedMTLTexture *wrappedMTLTexture = GetWrapped(texture);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_setFragmentTexture);
      Serialise_mtlRenderCommandEncoder_setFragmentTexture(ser, this, wrappedMTLTexture,
                                                           (NSUInteger_objc)index);
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
    NSUInteger_objc index)
{
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
    id_MTLTexture texture = GetObjCWrappedResource<id_MTLTexture>(GetResourceManager(), Texture);
    encoder->real_setFragmentTexture(texture, (NSUInteger)index);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::setViewport(MTLViewport &viewport)
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

void WrappedMTLRenderCommandEncoder::drawPrimitives(MTLPrimitiveType primitiveType,
                                                    NSUInteger vertexStart, NSUInteger vertexCount,
                                                    NSUInteger instanceCount)
{
  SERIALISE_TIME_CALL(real_drawPrimitives(primitiveType, vertexStart, vertexCount, instanceCount));

  MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

  if(IsCaptureMode(m_State))
  {
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLRenderCommandEncoder_drawPrimitives);
      Serialise_mtlRenderCommandEncoder_drawPrimitives(
          ser, this, (MTLPrimitiveType_objc)primitiveType, (NSUInteger_objc)vertexStart,
          (NSUInteger_objc)vertexCount, (NSUInteger_objc)instanceCount);
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
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder, MTLPrimitiveType_objc primitiveType,
    NSUInteger_objc vertexStart, NSUInteger_objc vertexCount, NSUInteger_objc instanceCount)
{
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
    encoder->real_drawPrimitives((MTLPrimitiveType)primitiveType, (NSUInteger)vertexStart,
                                 (NSUInteger)vertexCount, (NSUInteger)instanceCount);
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::endEncoding()
{
  SERIALISE_TIME_CALL(real_endEncoding());

  MetalResourceRecord *encoderRecord = GetRecord(this);
  MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

  if(IsCaptureMode(m_State))
  {
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
    // TODO: implement RD MTL replay
  }
}

template <typename SerialiserType>
bool WrappedMTLRenderCommandEncoder::Serialise_mtlRenderCommandEncoder_endEncoding(
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    encoder =
        (WrappedMTLRenderCommandEncoder *)GetResourceManager()->GetLiveResource(RenderCommandEncoder);
    encoder->real_endEncoding();
  }
  return true;
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
                                NSUInteger_objc offset, NSUInteger_objc index);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_setFragmentBuffer,
                                WrappedMTLRenderCommandEncoder *encoder, WrappedMTLBuffer *buffer,
                                NSUInteger_objc offset, NSUInteger_objc index);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_setFragmentTexture,
                                WrappedMTLRenderCommandEncoder *encoder, WrappedMTLTexture *texture,
                                NSUInteger_objc index);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_drawPrimitives,
                                WrappedMTLRenderCommandEncoder *encoder,
                                MTLPrimitiveType_objc primitiveType, NSUInteger_objc vertexStart,
                                NSUInteger_objc vertexCount, NSUInteger_objc instanceCount);
