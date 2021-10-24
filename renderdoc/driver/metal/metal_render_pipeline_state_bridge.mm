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

#include "metal_render_pipeline_state.h"
#include "metal_types_bridge.h"

// Wrapper for MTLRenderPipelineState
@implementation ObjCWrappedMTLRenderPipelineState

// ObjCWrappedMTLRenderPipelineState specific
- (id<MTLRenderPipelineState>)real
{
  MTL::RenderPipelineState *real = Unwrap<MTL::RenderPipelineState *>(self.wrappedCPP);
  return id<MTLRenderPipelineState>(real);
}

- (void)dealloc
{
  self.wrappedCPP->Dealloc();
  [super dealloc];
}

// MTLRenderPipelineState
- (nullable NSString *)label
{
  return self.real.label;
}

- (id<MTLDevice>)device
{
  return id<MTLDevice>(self.wrappedCPP->GetObjCWrappedMTLDevice());
}

- (NSUInteger)maxTotalThreadsPerThreadgroup
{
  if(@available(macOS 11.0, *))
  {
    return self.real.maxTotalThreadsPerThreadgroup;
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
    return self.real.threadgroupSizeMatchesTileSize;
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
    return self.real.imageblockSampleLength;
  }
  else
  {
    // Fallback on earlier versions
    return 0;
  }
}

- (BOOL)supportIndirectCommandBuffers
{
  return self.real.supportIndirectCommandBuffers;
}

- (NSUInteger)imageblockMemoryLengthForDimensions:(MTLSize)imageblockDimensions
    API_AVAILABLE(macos(11.0), macCatalyst(14.0), ios(11.0), tvos(14.5))
{
  return self.wrappedCPP->imageblockMemoryLengthForDimensions((MTL::Size &)imageblockDimensions);
}

- (nullable id<MTLFunctionHandle>)functionHandleWithFunction:(id<MTLFunction>)function
                                                       stage:(MTLRenderStages)stage
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real functionHandleWithFunction:function stage:stage];
}

- (nullable id<MTLVisibleFunctionTable>)newVisibleFunctionTableWithDescriptor:
                                            (MTLVisibleFunctionTableDescriptor *__nonnull)descriptor
                                                                        stage:(MTLRenderStages)stage
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newVisibleFunctionTableWithDescriptor:descriptor stage:stage];
}

- (nullable id<MTLIntersectionFunctionTable>)
newIntersectionFunctionTableWithDescriptor:(MTLIntersectionFunctionTableDescriptor *_Nonnull)descriptor
                                     stage:(MTLRenderStages)stage
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newIntersectionFunctionTableWithDescriptor:descriptor stage:stage];
}

- (nullable id<MTLRenderPipelineState>)
newRenderPipelineStateWithAdditionalBinaryFunctions:
    (nonnull MTLRenderPipelineFunctionsDescriptor *)additionalBinaryFunctions
                                              error:(__autoreleasing NSError **)error
    API_AVAILABLE(macos(12.0), ios(15.0))
{
  NSLog(@"Not hooked %@", NSStringFromSelector(_cmd));
  return [self.real newRenderPipelineStateWithAdditionalBinaryFunctions:additionalBinaryFunctions
                                                                  error:error];
}

@end
