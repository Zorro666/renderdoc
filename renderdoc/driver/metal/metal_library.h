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

#pragma once

#include "metal_common.h"
#include "metal_device.h"

class WrappedMTLLibrary : public WrappedMTLObject
{
public:
  WrappedMTLLibrary(id_MTLLibrary realMTLLibrary, ResourceId objId,
                    WrappedMTLDevice *wrappedMTLDevice);
  WrappedMTLLibrary(WrappedMTLDevice *wrappedMTLDevice);

  id_MTLLibrary GetObjCWrappedMTLLibrary();

  DECLARE_WRAPPED_API(id_MTLFunction, newFunctionWithName, NSString *functionName);

  DECLARE_FUNCTION_SERIALISED(mtlLibrary_newFunctionWithName, WrappedMTLLibrary *library,
                              NSString *functionName, WrappedMTLFunction *function);

  enum
  {
    TypeEnum = eResLibrary
  };

private:
  id_MTLLibrary CreateObjCWrappedMTLLibrary();
  id_MTLLibrary m_ObjCWrappedMTLLibrary;
  CaptureState &m_State;
};

inline id_MTLLibrary WrappedMTLLibrary::GetObjCWrappedMTLLibrary()
{
  return m_ObjCWrappedMTLLibrary;
}
