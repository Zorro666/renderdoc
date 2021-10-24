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

#include "common/timing.h"
#include "serialise/serialiser.h"
#include "metal_common.h"

// must be at the start of any function that serialises
#define CACHE_THREAD_SERIALISER() WriteSerialiser &ser = m_WrappedMTLDevice->GetThreadSerialiser();

#define SERIALISE_TIME_CALL(...)                                                                \
  {                                                                                             \
    WriteSerialiser &ser = m_WrappedMTLDevice->GetThreadSerialiser();                           \
    ser.ChunkMetadata().timestampMicro = Timing::GetTick();                                     \
    __VA_ARGS__;                                                                                \
    ser.ChunkMetadata().durationMicro = Timing::GetTick() - ser.ChunkMetadata().timestampMicro; \
  }

#define DECLARE_WRAPPED_API(ret, API, ...) \
private:                                   \
  ret CONCAT(real_, API)(__VA_ARGS__);     \
                                           \
public:                                    \
  ret API(__VA_ARGS__);

#define DECLARE_FUNCTION_SERIALISED(API, ...) \
  template <typename SerialiserType>          \
  bool CONCAT(Serialise_, API)(SerialiserType & ser, __VA_ARGS__);

#define INSTANTIATE_FUNCTION_SERIALISED(ret, CLASS, func, ...)                     \
  template bool CLASS::CONCAT(Serialise_, func(ReadSerialiser &ser, __VA_ARGS__)); \
  template bool CLASS::CONCAT(Serialise_, func(WriteSerialiser &ser, __VA_ARGS__));

// A handy macro to say "is the serialiser reading and we're doing replay-mode stuff?"
// The reason we check both is that checking the first allows the compiler to eliminate the other
// path at compile-time, and the second because we might be just struct-serialising in which case we
// should be doing no work to restore states.
// Writing is unambiguously during capture mode, so we don't have to check both in that case.
#define IsReplayingAndReading() (ser.IsReading() && IsReplayMode(m_WrappedMTLDevice->GetState()))

struct MetalPackedVersion
{
  MetalPackedVersion(uint32_t v = 0) : version(v) {}
  uint32_t version;

  bool operator<(uint32_t v) const { return version < v; }
  bool operator>(uint32_t v) const { return version > v; }
  bool operator<=(uint32_t v) const { return version <= v; }
  bool operator>=(uint32_t v) const { return version >= v; }
  bool operator==(uint32_t v) const { return version == v; }
  bool operator!=(uint32_t v) const { return version != v; }
  // int overloads because MTEAL_MAKE_VERSION is type int...
  bool operator<(int v) const { return version < (uint32_t)v; }
  bool operator>(int v) const { return version > (uint32_t)v; }
  bool operator<=(int v) const { return version <= (uint32_t)v; }
  bool operator>=(int v) const { return version >= (uint32_t)v; }
  bool operator==(int v) const { return version == (uint32_t)v; }
  bool operator!=(int v) const { return version != (uint32_t)v; }
  operator uint32_t() const { return version; }
  MetalPackedVersion &operator=(uint32_t v)
  {
    version = v;
    return *this;
  }
};

DECLARE_REFLECTION_STRUCT(MetalPackedVersion);

struct MetalInitParams
{
  // remember to update this function if you add more members
  uint64_t GetSerialiseSize();

  // check if a frame capture section version is supported
  static const uint64_t CurrentVersion = 0x1;
  static bool IsSupportedVersion(uint64_t ver);
};

DECLARE_REFLECTION_STRUCT(MetalInitParams);
