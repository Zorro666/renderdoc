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

#include "metal_library_bridge.h"
#import <Metal/MTLLibrary.h>
#include "metal_library.h"

id_MTLLibrary WrappedMTLLibrary::CreateObjCWrappedMTLLibrary()
{
  ObjCWrappedMTLLibrary *objCWrappedMTLLibrary = [ObjCWrappedMTLLibrary alloc];
  objCWrappedMTLLibrary.wrappedMTLLibrary = this;
  return objCWrappedMTLLibrary;
}

id_MTLFunction WrappedMTLLibrary::real_newFunctionWithName(NSString *functionName)
{
  id_MTLLibrary realMTLLibrary = Unwrap<id_MTLLibrary>(this);
  id_MTLFunction realMTLFunction = [realMTLLibrary newFunctionWithName:functionName];
  return realMTLFunction;
}

// Wrapper for MTLLibrary
@implementation ObjCWrappedMTLLibrary

// ObjCWrappedMTLLibrary specific
- (id<MTLLibrary>)realMTLLibrary
{
  id_MTLLibrary realMTLLibrary = Unwrap<id_MTLLibrary>(self.wrappedMTLLibrary);
  return realMTLLibrary;
}

// MTLLibrary
- (NSString *)label
{
  return self.realMTLLibrary.label;
}

- (void)setLabel:value
{
  self.realMTLLibrary.label = value;
}

- (id<MTLDevice>)device
{
  return self.wrappedMTLLibrary->GetObjCWrappedMTLDevice();
}

- (NSArray<NSString *> *)functionNames
{
  return self.realMTLLibrary.functionNames;
}

- (MTLLibraryType)type API_AVAILABLE(macos(11.0))
{
  return self.realMTLLibrary.type;
}

- (NSString *)installName
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLLibrary.installName;
  }
  else
  {
    // Fallback on earlier versions
    return NULL;
  }
}

- (nullable id<MTLFunction>)newFunctionWithName:(NSString *)functionName
{
  return self.wrappedMTLLibrary->newFunctionWithName(functionName);
}

- (nullable id<MTLFunction>)newFunctionWithName:(NSString *)name
                                 constantValues:(MTLFunctionConstantValues *)constantValues
                                          error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.12), ios(10.0))
{
  return [self.realMTLLibrary newFunctionWithName:name constantValues:constantValues error:error];
}

- (void)newFunctionWithName:(NSString *)name
             constantValues:(MTLFunctionConstantValues *)constantValues
          completionHandler:(void (^)(id<MTLFunction> __nullable function,
                                      NSError *__nullable error))completionHandler
    API_AVAILABLE(macos(10.12), ios(10.0))
{
  return [self.realMTLLibrary newFunctionWithName:name
                                   constantValues:constantValues
                                completionHandler:completionHandler];
}

- (void)newFunctionWithDescriptor:(nonnull MTLFunctionDescriptor *)descriptor
                completionHandler:(void (^)(id<MTLFunction> __nullable function,
                                            NSError *__nullable error))completionHandler
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLLibrary newFunctionWithDescriptor:descriptor
                                      completionHandler:completionHandler];
}

- (nullable id<MTLFunction>)newFunctionWithDescriptor:(nonnull MTLFunctionDescriptor *)descriptor
                                                error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLLibrary newFunctionWithDescriptor:descriptor error:error];
}

- (void)newIntersectionFunctionWithDescriptor:(nonnull MTLIntersectionFunctionDescriptor *)descriptor
                            completionHandler:(void (^)(id<MTLFunction> __nullable function,
                                                        NSError *__nullable error))completionHandler
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLLibrary newIntersectionFunctionWithDescriptor:descriptor
                                                  completionHandler:completionHandler];
}

- (nullable id<MTLFunction>)newIntersectionFunctionWithDescriptor:
                                (nonnull MTLIntersectionFunctionDescriptor *)descriptor
                                                            error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  return [self.realMTLLibrary newIntersectionFunctionWithDescriptor:descriptor error:error];
}

@end
