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

#include "metal_render_pipeline_state_bridge.h"
#import <Metal/MTLRenderPipeline.h>
#include "metal_render_pipeline_state.h"

static ObjCWrappedMTLRenderPipelineState *GetObjC(id_MTLRenderPipelineState pipelineState)
{
  RDCASSERT([pipelineState isKindOfClass:[ObjCWrappedMTLRenderPipelineState class]]);
  ObjCWrappedMTLRenderPipelineState *objC = (ObjCWrappedMTLRenderPipelineState *)pipelineState;
  return objC;
}

id_MTLRenderPipelineState GetReal(id_MTLRenderPipelineState pipelineState)
{
  ObjCWrappedMTLRenderPipelineState *objC = GetObjC(pipelineState);
  id_MTLRenderPipelineState realRenderPipelineState = objC.realMTLRenderPipelineState;
  return realRenderPipelineState;
}

WrappedMTLRenderPipelineState *GetWrapped(id_MTLRenderPipelineState pipelineState)
{
  ObjCWrappedMTLRenderPipelineState *objC = GetObjC(pipelineState);
  return [objC wrappedMTLRenderPipelineState];
}

id_MTLRenderPipelineState WrappedMTLRenderPipelineState::CreateObjCWrappedMTLRenderPipelineState()
{
  ObjCWrappedMTLRenderPipelineState *objCWrappedMTLRenderPipelineState =
      [ObjCWrappedMTLRenderPipelineState new];
  objCWrappedMTLRenderPipelineState.wrappedMTLRenderPipelineState = this;
  return objCWrappedMTLRenderPipelineState;
}

NSUInteger WrappedMTLRenderPipelineState::real_imageblockMemoryLengthForDimensions(
    MTLSize imageblockDimensions)
{
  if(@available(macOS 11.0, *))
  {
    id_MTLRenderPipelineState realMTLRenderPipelineState = Unwrap<id_MTLRenderPipelineState>(this);
    return [realMTLRenderPipelineState imageblockMemoryLengthForDimensions:imageblockDimensions];
  }
  else
  {
    return 0;
  }
}

// Wrapper for MTLRenderPipelineState
@implementation ObjCWrappedMTLRenderPipelineState

// ObjCWrappedMTLRenderPipelineState specific
- (id<MTLRenderPipelineState>)realMTLRenderPipelineState
{
  id_MTLRenderPipelineState realMTLRenderPipelineState =
      Unwrap<id_MTLRenderPipelineState>(self.wrappedMTLRenderPipelineState);
  return realMTLRenderPipelineState;
}

// MTLRenderPipelineState
- (nullable NSString *)label
{
  return self.realMTLRenderPipelineState.label;
}

- (id<MTLDevice>)device
{
  return self.wrappedMTLRenderPipelineState->GetObjCWrappedMTLDevice();
}

- (NSUInteger)maxTotalThreadsPerThreadgroup
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLRenderPipelineState.maxTotalThreadsPerThreadgroup;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (BOOL)threadgroupSizeMatchesTileSize
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLRenderPipelineState.threadgroupSizeMatchesTileSize;
  }
  else
  {
    // Fallback on earlier versions
    return NO;
  }
}

- (NSUInteger)imageblockSampleLength
{
  if(@available(macOS 11.0, *))
  {
    return self.realMTLRenderPipelineState.imageblockSampleLength;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (BOOL)supportIndirectCommandBuffers
{
  return self.realMTLRenderPipelineState.supportIndirectCommandBuffers;
}

- (NSUInteger)imageblockMemoryLengthForDimensions:(MTLSize)imageblockDimensions
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  return self.wrappedMTLRenderPipelineState->imageblockMemoryLengthForDimensions(imageblockDimensions);
}

- (nullable id<MTLFunctionHandle>)functionHandleWithFunction:(id<MTLFunction>)function
                                                       stage:(MTLRenderStages)stage
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderPipelineState functionHandleWithFunction:function stage:stage];
}

- (nullable id<MTLVisibleFunctionTable>)newVisibleFunctionTableWithDescriptor:
                                            (MTLVisibleFunctionTableDescriptor *__nonnull)descriptor
                                                                        stage:(MTLRenderStages)stage
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderPipelineState newVisibleFunctionTableWithDescriptor:descriptor
                                                                          stage:stage];
}

- (nullable id<MTLIntersectionFunctionTable>)
newIntersectionFunctionTableWithDescriptor:(MTLIntersectionFunctionTableDescriptor *_Nonnull)descriptor
                                     stage:(MTLRenderStages)stage
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderPipelineState newIntersectionFunctionTableWithDescriptor:descriptor
                                                                               stage:stage];
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithAdditionalBinaryFunctions:
    (nonnull MTLRenderPipelineFunctionsDescriptor *)additionalBinaryFunctions
                                              error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.realMTLRenderPipelineState
      newRenderPipelineStateWithAdditionalBinaryFunctions:additionalBinaryFunctions
                                                    error:error];
}

@end
