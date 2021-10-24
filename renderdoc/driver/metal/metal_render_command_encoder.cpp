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
#include "metal_render_pipeline_state.h"

RDOC_EXTERN_CONFIG(bool, Metal_Debug_VerboseCommandRecording);

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(
    id_MTLRenderCommandEncoder realMTLRenderCommandEncoder, ResourceId objId,
    WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLRenderCommandEncoder, objId, wrappedMTLDevice),
      m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLRenderCommandEncoder = CreateObjCWrappedMTLRenderCommandEncoder();
}

WrappedMTLRenderCommandEncoder::WrappedMTLRenderCommandEncoder(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice), m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLRenderCommandEncoder = CreateObjCWrappedMTLRenderCommandEncoder();
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
    WrappedMTLRenderPipelineState *wrappedPipelineState = GetWrappedFromObjC(pipelineState);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlRenderCommandEncoder_setRenderPipelineState);
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
    WrappedMTLBuffer *wrappedMTLBuffer = GetWrappedFromObjC(buffer);
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlRenderCommandEncoder_setVertexBuffer);
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
    NSUInteger offset, NSUInteger index)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  SERIALISE_ELEMENT_LOCAL(Buffer, GetResID(buffer)).TypedAs("MTLBuffer"_lit);
  uint64_t offset_u64(offset);
  uint64_t index_u64(index);
  {
    SERIALISE_ELEMENT_LOCAL(offset, offset_u64).TypedAs("uint64_t"_lit);
    SERIALISE_ELEMENT_LOCAL(index, index_u64).TypedAs("uint64_t"_lit);
  }

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
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
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlRenderCommandEncoder_drawPrimitives);
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
    SerialiserType &ser, WrappedMTLRenderCommandEncoder *encoder, MTLPrimitiveType primitiveType,
    NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount)
{
  SERIALISE_ELEMENT_LOCAL(RenderCommandEncoder, GetResID(encoder))
      .TypedAs("MTLRenderCommandEncoder"_lit);
  uint64_t primitiveType_u64((uint64_t)primitiveType);
  uint64_t vertexStart_u64(vertexStart);
  uint64_t vertexCount_u64(vertexCount);
  uint64_t instanceCount_u64(instanceCount);
  {
    SERIALISE_ELEMENT_LOCAL(primitiveType, primitiveType_u64).TypedAs("MTLPrimitiveType"_lit);
    SERIALISE_ELEMENT_LOCAL(vertexStart, vertexStart_u64).TypedAs("uint64_t"_lit);
    SERIALISE_ELEMENT_LOCAL(vertexCount, vertexCount_u64).TypedAs("uint64_t"_lit);
    SERIALISE_ELEMENT_LOCAL(instanceCount, instanceCount_u64).TypedAs("uint64_t"_lit);
  }

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

void WrappedMTLRenderCommandEncoder::endEncoding()
{
  SERIALISE_TIME_CALL(real_endEncoding());

  MetalResourceRecord *encoderRecord = GetRecord(this);
  MetalResourceRecord *bufferRecord = GetRecord(m_WrappedMTLCommandBuffer);

  if(Metal_Debug_VerboseCommandRecording())
  {
    RDCLOG("End RenderCommandEncoder %s CommandBuffer %s",
           ToStr(encoderRecord->GetResourceID()).c_str(),
           ToStr(bufferRecord->GetResourceID()).c_str());
  }

  if(IsCaptureMode(m_State))
  {
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlRenderCommandEncoder_endEncoding);
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
                                NSUInteger offset, NSUInteger index);
INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLRenderCommandEncoder,
                                mtlRenderCommandEncoder_drawPrimitives,
                                WrappedMTLRenderCommandEncoder *encoder,
                                MTLPrimitiveType primitiveType, NSUInteger vertexStart,
                                NSUInteger vertexCount, NSUInteger instanceCount);
