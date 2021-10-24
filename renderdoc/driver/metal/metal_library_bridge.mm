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
#include "metal_types_bridge.h"

// Wrapper for MTLLibrary
@implementation ObjCWrappedMTLLibrary

// ObjCWrappedMTLLibrary specific
- (id<MTLLibrary>)real
{
  MTL::Library *real = Unwrap<MTL::Library *>(self.wrappedCPP);
  return id<MTLLibrary>(real);
}

// MTLLibrary
- (NSString *)label
{
  return self.real.label;
}

- (void)setLabel:value
{
  self.real.label = value;
}

- (id<MTLDevice>)device
{
  return id<MTLDevice>(self.wrappedCPP->GetObjCWrappedMTLDevice());
}

- (NSArray<NSString *> *)functionNames
{
  return self.real.functionNames;
}

- (MTLLibraryType)type API_AVAILABLE(macos(11.0))
{
  return self.real.type;
}

- (NSString *)installName
{
  if(@available(macOS 11.0, *))
  {
    return self.real.installName;
  }
  else
  {
    // Fallback on earlier versions
    return NULL;
  }
}

- (nullable id<MTLFunction>)newFunctionWithName:(NSString *)functionName
{
  return id<MTLFunction>(self.wrappedCPP->newFunctionWithName((NS::String *)functionName));
}

- (nullable id<MTLFunction>)newFunctionWithName:(NSString *)name
                                 constantValues:(MTLFunctionConstantValues *)constantValues
                                          error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newFunctionWithName:name constantValues:constantValues error:error];
}

- (void)newFunctionWithName:(NSString *)name
             constantValues:(MTLFunctionConstantValues *)constantValues
          completionHandler:(void (^)(id<MTLFunction> __nullable function,
                                      NSError *__nullable error))completionHandler
    API_AVAILABLE(macos(10.12), ios(10.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newFunctionWithName:name
                         constantValues:constantValues
                      completionHandler:completionHandler];
}

- (void)newFunctionWithDescriptor:(nonnull MTLFunctionDescriptor *)descriptor
                completionHandler:(void (^)(id<MTLFunction> __nullable function,
                                            NSError *__nullable error))completionHandler
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newFunctionWithDescriptor:descriptor completionHandler:completionHandler];
}

- (nullable id<MTLFunction>)newFunctionWithDescriptor:(nonnull MTLFunctionDescriptor *)descriptor
                                                error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newFunctionWithDescriptor:descriptor error:error];
}

- (void)newIntersectionFunctionWithDescriptor:(nonnull MTLIntersectionFunctionDescriptor *)descriptor
                            completionHandler:(void (^)(id<MTLFunction> __nullable function,
                                                        NSError *__nullable error))completionHandler
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newIntersectionFunctionWithDescriptor:descriptor
                                        completionHandler:completionHandler];
}

- (nullable id<MTLFunction>)newIntersectionFunctionWithDescriptor:
                                (nonnull MTLIntersectionFunctionDescriptor *)descriptor
                                                            error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(11.0), ios(14.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newIntersectionFunctionWithDescriptor:descriptor error:error];
}

@end
