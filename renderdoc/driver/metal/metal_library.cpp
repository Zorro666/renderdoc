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

#include "metal_library.h"
#include "metal_function.h"
#include "metal_manager.h"

WrappedMTLLibrary::WrappedMTLLibrary(id_MTLLibrary realMTLLibrary, ResourceId objId,
                                     WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLLibrary, objId, wrappedMTLDevice),
      m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLLibrary = CreateObjCWrappedMTLLibrary();
}

WrappedMTLLibrary::WrappedMTLLibrary(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice), m_State(wrappedMTLDevice->GetStateRef())
{
  m_ObjCWrappedMTLLibrary = CreateObjCWrappedMTLLibrary();
}

id_MTLFunction WrappedMTLLibrary::newFunctionWithName(NSString *functionName)
{
  id_MTLFunction realMTLFunction;
  SERIALISE_TIME_CALL(realMTLFunction = real_newFunctionWithName(functionName));

  MetalResourceManager::UnwrapHelper<id_MTLFunction>::Outer *wrappedMTLFunction;
  ResourceId id = GetResourceManager()->WrapResource(realMTLFunction, wrappedMTLFunction);

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::mtlLibrary_newFunctionWithName);
      Serialise_mtlLibrary_newFunctionWithName(ser, this, functionName, wrappedMTLFunction);
      chunk = scope.Get();
    }
    MetalResourceRecord *record = GetResourceManager()->AddResourceRecord(wrappedMTLFunction);
    record->AddChunk(chunk);
    GetResourceManager()->MarkResourceFrameReferenced(id, eFrameRef_Read);
    record->AddParent(GetRecord(this));
  }
  else
  {
    // TODO: implement RD MTL replay
  }
  return wrappedMTLFunction->GetObjCWrappedMTLFunction();
}

template <typename SerialiserType>
bool WrappedMTLLibrary::Serialise_mtlLibrary_newFunctionWithName(SerialiserType &ser,
                                                                 WrappedMTLLibrary *library,
                                                                 NSString *functionName,
                                                                 WrappedMTLFunction *function)
{
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);
  SERIALISE_ELEMENT_LOCAL(FunctionName, apple_GetUTF8CStringFromNSString(functionName))
      .TypedAs("NSString"_lit);
  SERIALISE_ELEMENT_LOCAL(Function, GetResID(function)).TypedAs("MTLFunction"_lit);

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
  }
  return true;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLLibrary, mtlLibrary_newFunctionWithName,
                                WrappedMTLLibrary *library, NSString *functionName,
                                WrappedMTLFunction *function);
