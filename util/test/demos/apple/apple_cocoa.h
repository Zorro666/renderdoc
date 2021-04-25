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

#ifndef __COCOA_H
#define __COCOA_H

#define COCOA_TRUE 1
#define COCOA_FALSE 0
#define COCOA_RELEASE 0
#define COCOA_PRESS 1

#define COCOA_KEY_UNKNOWN -1

#define COCOA_KEY_SPACE 32
#define COCOA_KEY_APOSTROPHE 39
#define COCOA_KEY_COMMA 44
#define COCOA_KEY_MINUS 45
#define COCOA_KEY_PERIOD 46
#define COCOA_KEY_SLASH 47

#define COCOA_KEY_0 48
#define COCOA_KEY_1 49
#define COCOA_KEY_2 50
#define COCOA_KEY_3 51
#define COCOA_KEY_4 52
#define COCOA_KEY_5 53
#define COCOA_KEY_6 54
#define COCOA_KEY_7 55
#define COCOA_KEY_8 56
#define COCOA_KEY_9 57

#define COCOA_KEY_SEMICOLON 59
#define COCOA_KEY_EQUAL 61

#define COCOA_KEY_A 65
#define COCOA_KEY_B 66
#define COCOA_KEY_C 67
#define COCOA_KEY_D 68
#define COCOA_KEY_E 69
#define COCOA_KEY_F 70
#define COCOA_KEY_G 71
#define COCOA_KEY_H 72
#define COCOA_KEY_I 73
#define COCOA_KEY_J 74
#define COCOA_KEY_K 75
#define COCOA_KEY_L 76
#define COCOA_KEY_M 77
#define COCOA_KEY_N 78
#define COCOA_KEY_O 79
#define COCOA_KEY_P 80
#define COCOA_KEY_Q 81
#define COCOA_KEY_R 82
#define COCOA_KEY_S 83
#define COCOA_KEY_T 84
#define COCOA_KEY_U 85
#define COCOA_KEY_V 86
#define COCOA_KEY_W 87
#define COCOA_KEY_X 88
#define COCOA_KEY_Y 89
#define COCOA_KEY_Z 90

#define COCOA_KEY_LEFT_BRACKET 91
#define COCOA_KEY_BACKSLASH 92
#define COCOA_KEY_RIGHT_BRACKET 93
#define COCOA_KEY_GRAVE_ACCENT 96

#define COCOA_KEY_ESCAPE 300
#define COCOA_KEY_ENTER 301
#define COCOA_KEY_TAB 302
#define COCOA_KEY_BACKSPACE 303
#define COCOA_KEY_INSERT 304
#define COCOA_KEY_DELETE 305
#define COCOA_KEY_RIGHT 306
#define COCOA_KEY_LEFT 307
#define COCOA_KEY_DOWN 308
#define COCOA_KEY_UP 309
#define COCOA_KEY_PAGE_UP 310
#define COCOA_KEY_PAGE_DOWN 311
#define COCOA_KEY_HOME 312
#define COCOA_KEY_END 313

#define COCOA_KEY_LEFT_SHIFT 400
#define COCOA_KEY_LEFT_CONTROL 401
#define COCOA_KEY_RIGHT_SHIFT 402
#define COCOA_KEY_RIGHT_CONTROL 403

#define COCOA_KEY_FIRST COCOA_KEY_SPACE
#define COCOA_KEY_LAST COCOA_KEY_RIGHT_CONTROL

#define COCOA_MOUSE_BUTTON_LEFT 0
#define COCOA_MOUSE_BUTTON_RIGHT 1
#define COCOA_MOUSE_BUTTON_MIDDLE 2

typedef struct COCOAwindow COCOAwindow;
typedef struct COCOAcontext COCOAcontext;
typedef void (*COCOAMouseButtonCallback)(COCOAwindow *, int, int, int);
typedef void (*COCOACharacterCallback)(COCOAwindow *, unsigned int);
typedef void (*COCOAKeyCallback)(COCOAwindow *, int, int);
typedef void (*COCOAScrollCallback)(COCOAwindow *, double, double);

int COCOA_Initialize(void);
void COCOA_Shutdown(void);
void COCOA_Poll(void);
double COCOA_GetTime(void);

void COCOA_GetFrameBufferSize(COCOAwindow *window, int *width, int *height);
void COCOA_SwapBuffers(COCOAwindow *window);

COCOAwindow *COCOA_NewWindow(int width, int height, const char *title);
void COCOA_DeleteWindow(COCOAwindow *window);
int COCOA_WindowShouldClose(COCOAwindow *window);
void COCOA_GetWindowSize(COCOAwindow *window, int *width, int *height);

COCOAcontext *COCOA_NewGLContext(COCOAwindow *window);
void COCOA_DeleteGLContext(COCOAcontext *context);
COCOAcontext *COCOA_GetGLContext(COCOAwindow *window);
void COCOA_SetGLContext(COCOAwindow *window, COCOAcontext *context);

void COCOA_GetMousePosition(COCOAwindow *handle, double *xpos, double *ypos);
int COCOA_GetMouseButtonState(COCOAwindow *window, int button);
void COCOA_SetMouseButtonCallback(COCOAwindow *window, COCOAMouseButtonCallback callback);
void COCOA_SetScrollCallback(COCOAwindow *window, COCOAScrollCallback callback);

int COCOA_GetKeyState(COCOAwindow *window, int key);
void COCOA_SetCharacterCallback(COCOAwindow *window, COCOACharacterCallback callback);
void COCOA_SetKeyCallback(COCOAwindow *window, COCOAKeyCallback callback);

void *COCOA_SwitchLayerToMetal(COCOAwindow *window);

#endif    // #ifndef __COCOA_H
