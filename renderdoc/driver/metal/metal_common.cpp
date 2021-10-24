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

#include "metal_common.h"
#include "metal_core.h"
#include "metal_resources.h"

#define IMPLEMENT_WRAPPED_TYPE_UNWRAP(TYPE) \
  id_##TYPE Unwrap(Wrapped##TYPE *obj) { return Unwrap<id_##TYPE>((WrappedMTLObject *)obj); }
// METAL_WRAPPED_TYPES(IMPLEMENT_WRAPPED_TYPE_UNWRAP)
#undef IMPLEMENT_WRAPPED_TYPE_UNWRAP

uint64_t MetalInitParams::GetSerialiseSize()
{
  // misc bytes and fixed integer members
  size_t ret = 28;

  return (uint64_t)ret;
}

bool MetalInitParams::IsSupportedVersion(uint64_t ver)
{
  if(ver == CurrentVersion)
    return true;

  return false;
}

template <typename SerialiserType>
void DoSerialise(SerialiserType &ser, MetalInitParams &el)
{
}

INSTANTIATE_SERIALISE_TYPE(MetalInitParams);
