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

#include "apple_cocoa.h"

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>
#include <mach/mach_time.h>
#include <pthread.h>

typedef struct COCOA_Context
{
  id nsglPixelFormat;
  id nsglObject;
} COCOA_Context;

typedef struct COCOA_Window
{
  struct COCOA_Window *next;
  COCOA_Context *glContext;
  id nsWindow;
  id delegate;
  id view;
  id layer;
  int shouldClose;
  char mouseButtons[3];
  char keys[COCOA_KEY_LAST + 1];

  COCOACharacterCallback characterCallback;
  COCOAKeyCallback keyCallback;
  COCOAMouseButtonCallback mouseButtonCallback;
  COCOAScrollCallback scrollCallback;
} COCOA_Window;

typedef struct COCOA_GlobalState
{
  short int keycodes[256];
  pthread_key_t contextTLSkey;
  int contextTLSAllocated;
  COCOA_Window *windowListHead;
  CGEventSourceRef eventSource;
  id nsAppDelegate;
  id keyUpMonitor;
  uint64_t timerFrequency;
} COCOA_GlobalState;

static COCOA_GlobalState s_COCOA = {0};

static void SetCurrentGLContext(COCOA_Window *window, COCOA_Context *context)
{
  @autoreleasepool
  {
    if(window)
      window->glContext = context;
    if(context)
      [context->nsglObject makeCurrentContext];
    else
      [NSOpenGLContext clearCurrentContext];

    assert(s_COCOA.contextTLSAllocated);
    pthread_setspecific(s_COCOA.contextTLSkey, context);
  }
}

static COCOA_Context *GetCurrentGLContext()
{
  assert(s_COCOA.contextTLSAllocated);
  return (COCOA_Context *)pthread_getspecific(s_COCOA.contextTLSkey);
}

static COCOA_Context *NewGLContext(COCOA_Window *window)
{
  COCOA_Context *glContext = (COCOA_Context *)calloc(1, sizeof(COCOA_Context));
  NSOpenGLPixelFormatAttribute attributes[] = {NSOpenGLPFAClosestPolicy, NSOpenGLPFAOpenGLProfile,
                                               NSOpenGLProfileVersion4_1Core,
                                               NSOpenGLPFADoubleBuffer, 0};

  glContext->nsglPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
  assert(glContext->nsglPixelFormat);

  glContext->nsglObject =
      [[NSOpenGLContext alloc] initWithFormat:glContext->nsglPixelFormat shareContext:nil];
  assert(glContext->nsglObject);

  [window->view setWantsBestResolutionOpenGLSurface:false];
  [glContext->nsglObject setView:window->view];

  return glContext;
}

static void DeleteGLContext(COCOA_Context *context)
{
  if(!context)
    return;
  if(GetCurrentGLContext() == context)
    SetCurrentGLContext(nil, nil);

  @autoreleasepool
  {
    [context->nsglPixelFormat release];
    context->nsglPixelFormat = nil;

    [context->nsglObject release];
    context->nsglObject = nil;
  }
}

static void CreateKeyTables(void)
{
  memset(s_COCOA.keycodes, -1, sizeof(s_COCOA.keycodes));

  s_COCOA.keycodes[0x1D] = COCOA_KEY_0;
  s_COCOA.keycodes[0x12] = COCOA_KEY_1;
  s_COCOA.keycodes[0x13] = COCOA_KEY_2;
  s_COCOA.keycodes[0x14] = COCOA_KEY_3;
  s_COCOA.keycodes[0x15] = COCOA_KEY_4;
  s_COCOA.keycodes[0x17] = COCOA_KEY_5;
  s_COCOA.keycodes[0x16] = COCOA_KEY_6;
  s_COCOA.keycodes[0x1A] = COCOA_KEY_7;
  s_COCOA.keycodes[0x1C] = COCOA_KEY_8;
  s_COCOA.keycodes[0x19] = COCOA_KEY_9;

  s_COCOA.keycodes[0x00] = COCOA_KEY_A;
  s_COCOA.keycodes[0x0B] = COCOA_KEY_B;
  s_COCOA.keycodes[0x08] = COCOA_KEY_C;
  s_COCOA.keycodes[0x02] = COCOA_KEY_D;
  s_COCOA.keycodes[0x0E] = COCOA_KEY_E;
  s_COCOA.keycodes[0x03] = COCOA_KEY_F;
  s_COCOA.keycodes[0x05] = COCOA_KEY_G;
  s_COCOA.keycodes[0x04] = COCOA_KEY_H;
  s_COCOA.keycodes[0x22] = COCOA_KEY_I;
  s_COCOA.keycodes[0x26] = COCOA_KEY_J;
  s_COCOA.keycodes[0x28] = COCOA_KEY_K;
  s_COCOA.keycodes[0x25] = COCOA_KEY_L;
  s_COCOA.keycodes[0x2E] = COCOA_KEY_M;
  s_COCOA.keycodes[0x2D] = COCOA_KEY_N;
  s_COCOA.keycodes[0x1F] = COCOA_KEY_O;
  s_COCOA.keycodes[0x23] = COCOA_KEY_P;
  s_COCOA.keycodes[0x0C] = COCOA_KEY_Q;
  s_COCOA.keycodes[0x0F] = COCOA_KEY_R;
  s_COCOA.keycodes[0x01] = COCOA_KEY_S;
  s_COCOA.keycodes[0x11] = COCOA_KEY_T;
  s_COCOA.keycodes[0x20] = COCOA_KEY_U;
  s_COCOA.keycodes[0x09] = COCOA_KEY_V;
  s_COCOA.keycodes[0x0D] = COCOA_KEY_W;
  s_COCOA.keycodes[0x07] = COCOA_KEY_X;
  s_COCOA.keycodes[0x10] = COCOA_KEY_Y;
  s_COCOA.keycodes[0x06] = COCOA_KEY_Z;

  s_COCOA.keycodes[0x33] = COCOA_KEY_BACKSPACE;
  s_COCOA.keycodes[0x75] = COCOA_KEY_DELETE;
  s_COCOA.keycodes[0x7D] = COCOA_KEY_DOWN;
  s_COCOA.keycodes[0x77] = COCOA_KEY_END;
  s_COCOA.keycodes[0x24] = COCOA_KEY_ENTER;
  s_COCOA.keycodes[0x73] = COCOA_KEY_HOME;
  s_COCOA.keycodes[0x7B] = COCOA_KEY_LEFT;
  s_COCOA.keycodes[0x79] = COCOA_KEY_PAGE_DOWN;
  s_COCOA.keycodes[0x74] = COCOA_KEY_PAGE_UP;
  s_COCOA.keycodes[0x7C] = COCOA_KEY_RIGHT;
  s_COCOA.keycodes[0x30] = COCOA_KEY_TAB;
  s_COCOA.keycodes[0x7E] = COCOA_KEY_UP;

  s_COCOA.keycodes[0x27] = COCOA_KEY_APOSTROPHE;
  s_COCOA.keycodes[0x2A] = COCOA_KEY_BACKSLASH;
  s_COCOA.keycodes[0x2B] = COCOA_KEY_COMMA;
  s_COCOA.keycodes[0x18] = COCOA_KEY_EQUAL;
  s_COCOA.keycodes[0x32] = COCOA_KEY_GRAVE_ACCENT;
  s_COCOA.keycodes[0x21] = COCOA_KEY_LEFT_BRACKET;
  s_COCOA.keycodes[0x1B] = COCOA_KEY_MINUS;
  s_COCOA.keycodes[0x2F] = COCOA_KEY_PERIOD;
  s_COCOA.keycodes[0x1E] = COCOA_KEY_RIGHT_BRACKET;
  s_COCOA.keycodes[0x29] = COCOA_KEY_SEMICOLON;
  s_COCOA.keycodes[0x2C] = COCOA_KEY_SLASH;

  s_COCOA.keycodes[0x33] = COCOA_KEY_BACKSPACE;
  s_COCOA.keycodes[0x75] = COCOA_KEY_DELETE;
  s_COCOA.keycodes[0x7D] = COCOA_KEY_DOWN;
  s_COCOA.keycodes[0x77] = COCOA_KEY_END;
  s_COCOA.keycodes[0x24] = COCOA_KEY_ENTER;
  s_COCOA.keycodes[0x35] = COCOA_KEY_ESCAPE;
  s_COCOA.keycodes[0x73] = COCOA_KEY_HOME;
  s_COCOA.keycodes[0x72] = COCOA_KEY_INSERT;
  s_COCOA.keycodes[0x7B] = COCOA_KEY_LEFT;
  s_COCOA.keycodes[0x3B] = COCOA_KEY_LEFT_CONTROL;
  s_COCOA.keycodes[0x38] = COCOA_KEY_LEFT_SHIFT;
  s_COCOA.keycodes[0x79] = COCOA_KEY_PAGE_DOWN;
  s_COCOA.keycodes[0x74] = COCOA_KEY_PAGE_UP;
  s_COCOA.keycodes[0x7C] = COCOA_KEY_RIGHT;
  s_COCOA.keycodes[0x3E] = COCOA_KEY_RIGHT_CONTROL;
  s_COCOA.keycodes[0x3C] = COCOA_KEY_RIGHT_SHIFT;
  s_COCOA.keycodes[0x31] = COCOA_KEY_SPACE;
  s_COCOA.keycodes[0x30] = COCOA_KEY_TAB;
  s_COCOA.keycodes[0x7E] = COCOA_KEY_UP;
}

static int TranslateKey(unsigned int key)
{
  if(key >= sizeof(s_COCOA.keycodes) / sizeof(s_COCOA.keycodes[0]))
    return COCOA_KEY_UNKNOWN;

  return s_COCOA.keycodes[key];
}

static void InputMouseClick(COCOA_Window *window, int button, int action)
{
  if(button < 0 || button > 2)
    return;

  window->mouseButtons[button] = (char)action;
  if(window->mouseButtonCallback)
    window->mouseButtonCallback((COCOAwindow *)window, button, action, 0);
}

static void InputKey(COCOA_Window *window, int key, int action)
{
  if(key >= 0 && key <= COCOA_KEY_LAST)
  {
    if(action == COCOA_RELEASE && window->keys[key] == COCOA_RELEASE)
      return;

    window->keys[key] = (char)action;
  }
}

void COCOA_SwapBuffers(COCOAwindow *handle)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  @autoreleasepool
  {
    if(window->glContext)
      [window->glContext->nsglObject flushBuffer];
  }
}

@interface COCOA_WindowDelegate : NSObject
{
  COCOA_Window *window;
}

- (instancetype)initWithCOCOAWindow:(COCOA_Window *)initWindow;

@end

@implementation COCOA_WindowDelegate

- (instancetype)initWithCOCOAWindow:(COCOA_Window *)initWindow
{
  self = [super init];
  assert(self);
  window = initWindow;
  return self;
}

- (void)windowDidResize:(NSNotification *)notification
{
  if(window->glContext)
    [window->glContext->nsglObject update];
}

- (BOOL)windowShouldClose:(id)sender
{
  window->shouldClose = COCOA_TRUE;
  return NO;
}
@end

@interface COCOA_WindowView : NSView<NSTextInputClient>
{
  COCOA_Window *window;
}

- (instancetype)initWithCOCOAWindow:(COCOA_Window *)initWindow;

@end

@implementation COCOA_WindowView

- (instancetype)initWithCOCOAWindow:(COCOA_Window *)initWindow
{
  self = [super init];
  assert(self);
  window = initWindow;
  return self;
}

- (BOOL)wantsUpdateLayer
{
  return YES;
}

- (void)updateLayer
{
  if(window->glContext)
    [window->glContext->nsglObject update];
  else
    [window->layer setNeedsDisplay];
}

- (void)mouseDown:(NSEvent *)event
{
  InputMouseClick(window, COCOA_MOUSE_BUTTON_LEFT, COCOA_PRESS);
}

- (void)mouseDragged:(NSEvent *)event
{
  [self mouseMoved:event];
}

- (void)mouseUp:(NSEvent *)event
{
  InputMouseClick(window, COCOA_MOUSE_BUTTON_LEFT, COCOA_RELEASE);
}

- (void)rightMouseDown:(NSEvent *)event
{
  InputMouseClick(window, COCOA_MOUSE_BUTTON_RIGHT, COCOA_PRESS);
}

- (void)rightMouseDragged:(NSEvent *)event
{
  [self mouseMoved:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
  InputMouseClick(window, COCOA_MOUSE_BUTTON_RIGHT, COCOA_RELEASE);
}

- (void)otherMouseDown:(NSEvent *)event
{
  InputMouseClick(window, (int)[event buttonNumber], COCOA_PRESS);
}

- (void)otherMouseDragged:(NSEvent *)event
{
  [self mouseMoved:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
  InputMouseClick(window, (int)[event buttonNumber], COCOA_RELEASE);
}

- (void)keyDown:(NSEvent *)event
{
  const int key = TranslateKey([event keyCode]);
  InputKey(window, key, COCOA_PRESS);
  [self interpretKeyEvents:@[ event ]];
  if(window->keyCallback)
    window->keyCallback((COCOAwindow *)window, key, COCOA_PRESS);
}

- (void)flagsChanged:(NSEvent *)event
{
  int action;
  const unsigned int modifierFlags =
      [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
  const int key = TranslateKey([event keyCode]);
  NSUInteger keyFlag = 0;
  switch(key)
  {
    case COCOA_KEY_LEFT_SHIFT:
    case COCOA_KEY_RIGHT_SHIFT: keyFlag = NSEventModifierFlagShift; break;
    case COCOA_KEY_LEFT_CONTROL:
    case COCOA_KEY_RIGHT_CONTROL: keyFlag = NSEventModifierFlagControl; break;
  }

  if(keyFlag & modifierFlags)
  {
    if(window->keys[key] == COCOA_PRESS)
      action = COCOA_RELEASE;
    else
      action = COCOA_PRESS;
  }
  else
    action = COCOA_RELEASE;

  InputKey(window, key, action);
}

- (void)keyUp:(NSEvent *)event
{
  const int key = TranslateKey([event keyCode]);
  InputKey(window, key, COCOA_RELEASE);
  if(window->keyCallback)
    window->keyCallback((COCOAwindow *)window, key, COCOA_RELEASE);
}

- (void)scrollWheel:(NSEvent *)event
{
  double deltaX = [event scrollingDeltaX];
  double deltaY = [event scrollingDeltaY];

  if([event hasPreciseScrollingDeltas])
  {
    deltaX *= 0.1;
    deltaY *= 0.1;
  }

  if(fabs(deltaX) > 0.0 || fabs(deltaY) > 0.0)
  {
    if(window->scrollCallback)
      window->scrollCallback((COCOAwindow *)window, deltaX, deltaY);
  }
}

static const NSRange s_NSRange_Empty = {NSNotFound, 0};

- (BOOL)hasMarkedText
{
  return NO;
}

- (NSRange)markedRange
{
  return s_NSRange_Empty;
}

- (NSRange)selectedRange
{
  return s_NSRange_Empty;
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange
{
}

- (void)unmarkText
{
}

- (NSArray *)validAttributesForMarkedText
{
  return [NSArray array];
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range
                                                actualRange:(NSRangePointer)actualRange
{
  return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
  return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
  const NSRect frame = [window->view frame];
  return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
  if(!window->characterCallback)
    return;

  NSString *characters;
  NSEvent *event = [NSApp currentEvent];

  if([string isKindOfClass:[NSAttributedString class]])
    characters = [string string];
  else
    characters = (NSString *)string;

  NSRange range = NSMakeRange(0, [characters length]);
  while(range.length)
  {
    uint32_t codepoint = 0;

    if([characters getBytes:&codepoint
                  maxLength:sizeof(codepoint)
                 usedLength:nil
                   encoding:NSUTF32StringEncoding
                    options:0
                      range:range
             remainingRange:&range])
    {
      if(codepoint >= 0xf700 && codepoint <= 0xf7ff)
        continue;

      if(window->characterCallback)
        window->characterCallback((COCOAwindow *)window, codepoint);
    }
  }
}

@end

void COCOA_GetWindowSize(COCOAwindow *handle, int *width, int *height)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  @autoreleasepool
  {
    const NSRect contentRect = [window->view frame];

    if(width)
      *width = contentRect.size.width;
    if(height)
      *height = contentRect.size.height;
  }
}

void COCOA_GetFrameBufferSize(COCOAwindow *handle, int *width, int *height)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);

  @autoreleasepool
  {
    const NSRect contentRect = [window->view frame];
    const NSRect fbRect = [window->view convertRectToBacking:contentRect];

    if(width)
      *width = (int)fbRect.size.width;
    if(height)
      *height = (int)fbRect.size.height;
  }
}

void COCOA_Poll(void)
{
  @autoreleasepool
  {
    for(;;)
    {
      NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:[NSDate distantPast]
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES];
      if(event == nil)
        break;

      [NSApp sendEvent:event];
    }
  }
}

void COCOA_GetMousePosition(COCOAwindow *handle, double *xpos, double *ypos)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  @autoreleasepool
  {
    const NSRect contentRect = [window->view frame];
    const NSPoint pos = [window->nsWindow mouseLocationOutsideOfEventStream];

    if(xpos)
      *xpos = pos.x;
    if(ypos)
      *ypos = contentRect.size.height - pos.y;
  }
}

@interface COCOA_ApplicationDelegate : NSObject<NSApplicationDelegate>
@end

@implementation COCOA_ApplicationDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
  COCOA_Window *window;

  for(window = s_COCOA.windowListHead; window; window = window->next)
    window->shouldClose = COCOA_TRUE;

  return NSTerminateCancel;
}

- (void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
  for(COCOA_Window *window = s_COCOA.windowListHead; window; window = window->next)
  {
    if(window->glContext)
      [window->glContext->nsglObject update];
  }
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
  @autoreleasepool
  {
    NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0];
    [NSApp postEvent:event atStart:YES];
    [NSApp stop:nil];
  }
}

@end    // COCOA_ApplicationDelegate

int COCOA_WindowShouldClose(COCOAwindow *handle)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  return window->shouldClose;
}

void COCOA_SetCharacterCallback(COCOAwindow *handle, COCOACharacterCallback callback)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  window->characterCallback = callback;
}

void COCOA_SetKeyCallback(COCOAwindow *handle, COCOAKeyCallback callback)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  window->keyCallback = callback;
}

void COCOA_SetScrollCallback(COCOAwindow *handle, COCOAScrollCallback callback)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  window->scrollCallback = callback;
}

void COCOA_SetMouseButtonCallback(COCOAwindow *handle, COCOAMouseButtonCallback callback)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  window->mouseButtonCallback = callback;
}

double COCOA_GetTime(void)
{
  return (double)mach_absolute_time() / s_COCOA.timerFrequency;
}

int COCOA_GetKeyState(COCOAwindow *handle, int key)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  assert(key >= COCOA_KEY_FIRST && key <= COCOA_KEY_LAST);
  return (int)window->keys[key];
}

int COCOA_GetMouseButtonState(COCOAwindow *handle, int button)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  assert(window);
  assert(button >= 0 && button <= 2);
  return (int)window->mouseButtons[button];
}

int COCOA_Initialize(void)
{
  memset(&s_COCOA, 0, sizeof(s_COCOA));

  @autoreleasepool
  {
    [NSApplication sharedApplication];

    s_COCOA.nsAppDelegate = [[COCOA_ApplicationDelegate alloc] init];
    assert(s_COCOA.nsAppDelegate);

    [NSApp setDelegate:s_COCOA.nsAppDelegate];

    NSEvent * (^block)(NSEvent *) = ^NSEvent *(NSEvent *event)
    {
      if([event modifierFlags] & NSEventModifierFlagCommand)
        [[NSApp keyWindow] sendEvent:event];
      return event;
    };

    s_COCOA.keyUpMonitor =
        [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:block];

    CreateKeyTables();

    s_COCOA.eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if(!s_COCOA.eventSource)
    {
      COCOA_Shutdown();
      return COCOA_FALSE;
    }

    CGEventSourceSetLocalEventsSuppressionInterval(s_COCOA.eventSource, 0.0);

    if(![[NSRunningApplication currentApplication] isFinishedLaunching])
      [NSApp run];

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  }

  mach_timebase_info_data_t info;
  mach_timebase_info(&info);
  s_COCOA.timerFrequency = (info.denom * 1e9) / info.numer;

  assert(!s_COCOA.contextTLSAllocated);
  int result = pthread_key_create(&s_COCOA.contextTLSkey, nil);
  assert(result == 0);
  s_COCOA.contextTLSAllocated = 1;

  return COCOA_TRUE;
}

void COCOA_Shutdown(void)
{
  while(s_COCOA.windowListHead)
    COCOA_DeleteWindow((COCOAwindow *)s_COCOA.windowListHead);

  @autoreleasepool
  {
    if(s_COCOA.eventSource)
    {
      CFRelease(s_COCOA.eventSource);
      s_COCOA.eventSource = nil;
    }

    if(s_COCOA.nsAppDelegate)
    {
      [NSApp setDelegate:nil];
      [s_COCOA.nsAppDelegate release];
      s_COCOA.nsAppDelegate = nil;
    }

    if(s_COCOA.keyUpMonitor)
      [NSEvent removeMonitor:s_COCOA.keyUpMonitor];
  }

  if(s_COCOA.contextTLSAllocated)
    pthread_key_delete(s_COCOA.contextTLSkey);

  memset(&s_COCOA, 0, sizeof(s_COCOA));
}

COCOAwindow *COCOA_NewWindow(int width, int height, const char *title)
{
  assert(title);
  assert(width >= 0);
  assert(height >= 0);

  COCOA_Window *window = (COCOA_Window *)calloc(1, sizeof(COCOA_Window));
  window->next = s_COCOA.windowListHead;
  s_COCOA.windowListHead = window;

  window->delegate = [[COCOA_WindowDelegate alloc] initWithCOCOAWindow:window];
  assert(window->delegate);

  NSRect contentRect;

  contentRect = NSMakeRect(0, 0, width, height);

  window->nsWindow = [[NSWindow alloc]
      initWithContentRect:contentRect
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                  backing:NSBackingStoreBuffered
                    defer:NO];

  assert(window->nsWindow);

  [(NSWindow *)window->nsWindow center];

  window->view = [[COCOA_WindowView alloc] initWithCOCOAWindow:window];

  [window->nsWindow setContentView:window->view];
  [window->nsWindow makeFirstResponder:window->view];
  [window->nsWindow setTitle:@(title)];
  [window->nsWindow setDelegate:window->delegate];
  [window->nsWindow setAcceptsMouseMovedEvents:YES];
  [window->nsWindow setRestorable:NO];

  window->glContext = nil;

  [window->nsWindow orderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];
  [window->nsWindow makeKeyAndOrderFront:nil];
  return (COCOAwindow *)window;
}

void COCOA_DeleteWindow(COCOAwindow *handle)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  if(window == nil)
    return;

  window->characterCallback = nil;
  window->keyCallback = nil;
  window->mouseButtonCallback = nil;
  window->scrollCallback = nil;

  DeleteGLContext(window->glContext);
  window->glContext = nil;

  @autoreleasepool
  {
    [window->nsWindow orderOut:nil];
    [window->nsWindow setDelegate:nil];

    [window->delegate release];
    window->delegate = nil;

    [window->view release];
    window->view = nil;

    [window->nsWindow close];
    window->nsWindow = nil;

    COCOA_Poll();
  }

  COCOA_Window **prev = &s_COCOA.windowListHead;
  while(*prev != window)
    prev = &((*prev)->next);

  *prev = window->next;

  free(window);
}

COCOAcontext *COCOA_NewGLContext(COCOAwindow *handle)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  return (COCOAcontext *)NewGLContext(window);
}

void COCOA_DeleteGLContext(COCOAcontext *ctx)
{
  COCOA_Context *context = (COCOA_Context *)ctx;
  DeleteGLContext(context);
}

COCOAcontext *COCOA_GetGLContext(COCOAwindow *handle)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  return (COCOAcontext *)(window->glContext);
}

void COCOA_SetGLContext(COCOAwindow *handle, COCOAcontext *ctx)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  COCOA_Context *context = (COCOA_Context *)ctx;
  SetCurrentGLContext(window, context);
}

void *COCOA_SwitchLayerToMetal(COCOAwindow *handle)
{
  COCOA_Window *window = (COCOA_Window *)handle;
  @autoreleasepool
  {
    window->layer = [CAMetalLayer layer];
    [window->view setLayer:window->layer];
    [window->view setWantsLayer:YES];
  }
  return window->view;
}
