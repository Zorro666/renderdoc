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

#include "metal_types.h"
#include "metal_stringise.h"

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, NSString *&el)
{
  const char *cstr = MTL::GetUTF8CString(el);
  SERIALISE_ELEMENT(cstr);

  if(ser.IsReading())
  {
    el = MTL::NewNSStringFromUTF8(cstr);
  }
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLClearColor &el)
{
  SERIALISE_MEMBER(red);
  SERIALISE_MEMBER(green);
  SERIALISE_MEMBER(blue);
  SERIALISE_MEMBER(alpha);
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MTLSize &el)
{
  NSUInteger_objc width = (NSUInteger_objc)el.width;
  NSUInteger_objc height = (NSUInteger_objc)el.height;
  NSUInteger_objc depth = (NSUInteger_objc)el.depth;
  SERIALISE_ELEMENT(width);
  SERIALISE_ELEMENT(height);
  SERIALISE_ELEMENT(depth);
  if(ser.IsReading())
  {
    el.width = width;
    el.height = height;
    el.depth = depth;
  }
}

INSTANTIATE_SERIALISE_TYPE(NSString *);
INSTANTIATE_SERIALISE_TYPE(MTLClearColor);
INSTANTIATE_SERIALISE_TYPE(MTLSize);
