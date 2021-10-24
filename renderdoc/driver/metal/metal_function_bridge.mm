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

#include "metal_function_bridge.h"
#import <Metal/MTLLibrary.h>
#include "metal_function.h"

WrappedMTLFunction *GetWrappedFromObjC(id_MTLFunction function)
{
  RDCASSERT([function isKindOfClass:[ObjCWrappedMTLFunction class]]);

  ObjCWrappedMTLFunction *objCWrappedMTLFunction = (ObjCWrappedMTLFunction *)function;
  return [objCWrappedMTLFunction wrappedMTLFunction];
}

id_MTLFunction WrappedMTLFunction::CreateObjCWrappedMTLFunction()
{
  ObjCWrappedMTLFunction *objCWrappedMTLFunction = [ObjCWrappedMTLFunction alloc];
  objCWrappedMTLFunction.wrappedMTLFunction = this;
  return objCWrappedMTLFunction;
}

// Wrapper for MTLFunction
@implementation ObjCWrappedMTLFunction

// ObjCWrappedMTLLibrary specific
- (id<MTLFunction>)realMTLFunction
{
  id_MTLFunction realMTLFunction = Unwrap<id_MTLFunction>(self.wrappedMTLFunction);
  return realMTLFunction;
}

// MTLFunction
- (NSString *)label
{
  return self.realMTLFunction.label;
}

- (void)setLabel:value
{
  self.realMTLFunction.label = value;
}

- (id<MTLDevice>)device
{
  return self.wrappedMTLFunction->GetObjCWrappedMTLDevice();
}

- (MTLFunctionType)functionType
{
  return self.realMTLFunction.functionType;
}

- (MTLPatchType)patchType
{
  return self.realMTLFunction.patchType;
}

- (NSInteger)patchControlPointCount
{
  return self.realMTLFunction.patchControlPointCount;
}

- (NSArray<MTLVertexAttribute *> *)vertexAttributes
{
  return self.realMTLFunction.vertexAttributes;
}

- (NSArray<MTLAttribute *> *)stageInputAttributes
{
  return self.realMTLFunction.stageInputAttributes;
}

- (NSString *)name
{
  return self.realMTLFunction.name;
}

- (NSDictionary<NSString *, MTLFunctionConstant *> *)functionConstantsDictionary
{
  return self.realMTLFunction.functionConstantsDictionary;
}

- (id<MTLArgumentEncoder>)newArgumentEncoderWithBufferIndex:(NSUInteger)bufferIndex
{
  return [self.realMTLFunction newArgumentEncoderWithBufferIndex:bufferIndex];
}

- (id<MTLArgumentEncoder>)newArgumentEncoderWithBufferIndex:(NSUInteger)bufferIndex
                                                 reflection:(MTLAutoreleasedArgument *__nullable)reflection
{
  return [self.realMTLFunction newArgumentEncoderWithBufferIndex:bufferIndex reflection:reflection];
}

- (MTLFunctionOptions)options
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLFunction.options;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

@end
