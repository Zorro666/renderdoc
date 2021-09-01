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

#import <Foundation/NSStream.h>
#include "metal_stringise.h"

const char *MTL::GetUTF8CString(NSString *nsString)
{
  if(nsString == NULL)
    return NULL;
  return [nsString UTF8String];
}

NSString *MTL::NewNSStringFromUTF8(const char *cStr)
{
  if(cStr == NULL)
    return [NSString new];

  return [NSString stringWithCString:cStr encoding:NSUTF8StringEncoding];
}

const char *MTL::Get_localizedDescription(NSError *error)
{
  NSString *nsString = [error localizedDescription];
  return GetUTF8CString(nsString);
}

const char *MTL::Get_localizedRecoverySuggestion(NSError *error)
{
  NSString *nsString = [error localizedRecoverySuggestion];
  return GetUTF8CString(nsString);
}

const char *MTL::Get_localizedFailureReason(NSError *error)
{
  NSString *nsString = [error localizedFailureReason];
  return GetUTF8CString(nsString);
}
