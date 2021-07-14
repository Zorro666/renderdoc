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

#include <dlfcn.h>
#include "common/common.h"
#include "core/core.h"
#include "hooks/hooks.h"
#include "metal_dispatch_table.h"

#define METAL_EXPORT_NAME(function) CONCAT(interposed_, function)

class MetalHook : LibraryHook
{
public:
  MetalHook() {}
  void RegisterHooks();

  // default to RTLD_NEXT if we haven't gotten a more specific library handle
  void *handle = RTLD_NEXT;
} metalhook;

id<MTLDevice> METAL_EXPORT_NAME(MTLCreateSystemDefaultDevice)(void)
{
  if(RenderDoc::Inst().IsReplayApp())
  {
    if(!METAL.MTLCreateSystemDefaultDevice)
      METAL.PopulateForReplay();

    return METAL.MTLCreateSystemDefaultDevice();
  }

  auto mtlDevice = METAL.MTLCreateSystemDefaultDevice();

  return mtlDevice;
}

#define DECL_HOOK_EXPORT(function)                                               \
  __attribute__((used)) static struct                                            \
  {                                                                              \
    const void *replacment;                                                      \
    const void *replacee;                                                        \
  } _interpose_def_##function __attribute__((section("__DATA,__interpose"))) = { \
      (const void *)(unsigned long)&METAL_EXPORT_NAME(function),                 \
      (const void *)(unsigned long)&function,                                    \
  };

DECL_HOOK_EXPORT(MTLCreateSystemDefaultDevice);

static void MetalHooked(void *handle)
{
  RDCDEBUG("Metal library hooked");

  // store the handle for any pass-through implementations that need to look up their onward
  // pointers
  metalhook.handle = handle;

  // as a hook callback this is only called while capturing
  RDCASSERT(!RenderDoc::Inst().IsReplayApp());

// fetch non-hooked functions into our dispatch table
#define METAL_FETCH(func) METAL.func = &func;
  METAL_NONHOOKED_SYMBOLS(METAL_FETCH)
#undef METAL_FETCH
}

void MetalHook::RegisterHooks()
{
  RDCLOG("Registering Metal hooks");

  // Library hooks : this should be framework hooks?
  LibraryHooks::RegisterLibraryHook(
      "/System/Library/Frameworks/Metal.framework/Versions/Current/Metal", &MetalHooked);

// Function hooks
#define METAL_REGISTER(func)                                                                      \
  LibraryHooks::RegisterFunctionHook("Metal", FunctionHook(STRINGIZE(func), (void **)&METAL.func, \
                                                           (void *)&METAL_EXPORT_NAME(func)));
  METAL_HOOKED_SYMBOLS(METAL_REGISTER)
#undef METAL_REGISTER
}

MetalDispatchTable METAL = {};

bool MetalDispatchTable::PopulateForReplay()
{
  return false;
}
