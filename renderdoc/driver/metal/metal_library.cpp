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

#include "metal_library.h"
#include "metal_function.h"
#include "metal_manager.h"

WrappedMTLLibrary::WrappedMTLLibrary(MTL::Library *realMTLLibrary, ResourceId objId,
                                     WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(realMTLLibrary, objId, wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = AllocateObjCWrapper(this);
}

WrappedMTLLibrary::WrappedMTLLibrary(WrappedMTLDevice *wrappedMTLDevice)
    : WrappedMTLObject(wrappedMTLDevice, wrappedMTLDevice->GetStateRef())
{
  wrappedObjC = NULL;
}

MTL::Function *WrappedMTLLibrary::newFunctionWithName(NS::String *functionName)
{
  MTL::Function *realMTLFunction;
  SERIALISE_TIME_CALL(realMTLFunction = real_newFunctionWithName(functionName));

  MetalResourceManager::UnwrapHelper<MTL::Function *>::Outer *wrappedMTLFunction;
  ResourceId id = GetResourceManager()->WrapResource(realMTLFunction, wrappedMTLFunction);

  if(IsCaptureMode(m_State))
  {
    Chunk *chunk = NULL;
    {
      CACHE_THREAD_SERIALISER();
      SCOPED_SERIALISE_CHUNK(MetalChunk::MTLLibrary_newFunctionWithName);
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
  return UnwrapObjC<MTL::Function *>(wrappedMTLFunction);
}

template <typename SerialiserType>
bool WrappedMTLLibrary::Serialise_mtlLibrary_newFunctionWithName(SerialiserType &ser,
                                                                 WrappedMTLLibrary *library,
                                                                 NS::String *FunctionName,
                                                                 WrappedMTLFunction *function)
{
  SERIALISE_ELEMENT_LOCAL(Library, GetResID(library)).TypedAs("MTLLibrary"_lit);
  SERIALISE_ELEMENT_LOCAL(Function, GetResID(function)).TypedAs("MTLFunction"_lit);
  SERIALISE_ELEMENT(FunctionName).Important();

  SERIALISE_CHECK_READ_ERRORS();

  // TODO: implement RD MTL replay
  if(IsReplayingAndReading())
  {
    library = (WrappedMTLLibrary *)GetResourceManager()->GetLiveResource(Library);
    MTL::Function *realMTLFunction = library->real_newFunctionWithName(FunctionName);
    MetalResourceManager::UnwrapHelper<MTL::Function *>::Outer *wrappedMTLFunction;
    GetResourceManager()->WrapResource(realMTLFunction, wrappedMTLFunction);
    GetResourceManager()->AddLiveResource(Function, wrappedMTLFunction);
    m_WrappedMTLDevice->AddResource(Function, ResourceType::Shader, "Function");
    m_WrappedMTLDevice->DerivedResource(library, Function);
  }
  return true;
}

MTL::Function *WrappedMTLLibrary::real_newFunctionWithName(NS::String *functionName)
{
  MTL::Library *realMTLLibrary = Unwrap<MTL::Library *>(this);
  MTL::Function *realMTLFunction = realMTLLibrary->newFunction(functionName);
  return realMTLFunction;
}

INSTANTIATE_FUNCTION_SERIALISED(bool, WrappedMTLLibrary, mtlLibrary_newFunctionWithName,
                                WrappedMTLLibrary *library, NS::String *functionName,
                                WrappedMTLFunction *function);
