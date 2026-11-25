/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
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

#include "metal_residency_set.h"
#include "metal_types_bridge.h"

// Bridge for MTLResidencySet
API_AVAILABLE(macos(15.0), ios(18.0))
@implementation ObjCBridgeMTLResidencySet

// ObjCBridgeMTLResidencySet specific
- (id<MTLResidencySet>)real
{
  return id<MTLResidencySet>(Unwrap(GetWrapped(self)));
}

// Silence compiler warning
// error: method possibly missing a [super dealloc] call [-Werror,-Wobjc-missing-super-calls]
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)dealloc
{
  DeallocateObjCBridge(GetWrapped(self));
}
#pragma clang diagnostic pop

// Use the real MTLResidencySet to find methods from messages
- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
  id fwd = self.real;
  return [fwd methodSignatureForSelector:aSelector];
}

// Forward any unknown messages to the real MTLResidencySet
- (void)forwardInvocation:(NSInvocation *)invocation
{
  SEL aSelector = [invocation selector];

  if([self.real respondsToSelector:aSelector])
    [invocation invokeWithTarget:self.real];
  else
    [super forwardInvocation:invocation];
}

// MTLResidencySet: based on the protocol defined in
// Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX27.0.sdk/System/Library/Frameworks/Metal.framework/Headers/MTLResidencySet.h

- (nullable NSString *)label
{
  return self.real.label;
}

- (id<MTLDevice>)device
{
  return id<MTLDevice>(GetWrapped(self)->GetDevice());
}

- (uint64_t)allocatedSize
{
  return self.real.allocatedSize;
}

- (void)requestResidency
{
  METAL_NOT_HOOKED();
  [self.real requestResidency];
}

- (void)endResidency
{
  METAL_NOT_HOOKED();
  [self.real endResidency];
}

- (void)addAllocation:(id<MTLAllocation>)allocation
{
  METAL_NOT_HOOKED();
  [self.real addAllocation:allocation];
}

- (void)addAllocations:(const id<MTLAllocation> _Nonnull[_Nonnull])allocations
                 count:(NSUInteger)count
{
  METAL_NOT_HOOKED();
  [self.real addAllocations:allocations count:count];
}

- (void)removeAllocation:(id<MTLAllocation>)allocation
{
  METAL_NOT_HOOKED();
  [self.real removeAllocation:allocation];
}

- (void)removeAllocations:(const id<MTLAllocation> _Nonnull[_Nonnull])allocations
                    count:(NSUInteger)count
{
  METAL_NOT_HOOKED();
  [self.real removeAllocations:allocations count:count];
}

- (void)removeAllAllocations
{
  METAL_NOT_HOOKED();
  [self.real removeAllAllocations];
}

- (BOOL)containsAllocation:(id<MTLAllocation>)anAllocation
{
  METAL_NOT_HOOKED();
  return [self.real containsAllocation:anAllocation];
}

- (NSArray<id<MTLAllocation>> *)allAllocations
{
  return self.real.allAllocations;
}

- (NSUInteger)allocationCount
{
  return self.real.allocationCount;
}

- (void)commit
{
  METAL_NOT_HOOKED();
  [self.real commit];
}

@end
