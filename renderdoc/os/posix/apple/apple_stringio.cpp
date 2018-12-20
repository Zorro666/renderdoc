/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2018 Baldur Karlsson
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
#include <mach-o/dyld.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "api/app/renderdoc_app.h"
#include "os/os_specific.h"

extern bool IsKeyPressed(int key);
/*
#include <Carbon/Carbon.h>

bool isPressed( unsigned short inKeyCode )
{
    unsigned char keyMap[16];
    GetKeys((BigEndianUInt32*) &keyMap);
    return (0 != ((keyMap[ inKeyCode >> 3] >> (inKeyCode & 7)) & 1));
}
*/
/*
bool CGEventSourceKeyState(CGEventSourceStateID stateID, CGKeyCode key);
stateID = kCGEventSourceStateCombinedSessionState
/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/Events.h
CGKeyCode's
kVK_F12                       = 0x6F,


*/

namespace Keyboard
{
void Init()
{
}

bool PlatformHasKeyInput()
{
  return true;
}

bool IsKeySupported(int key)
{
  switch(key)
  {
    case eRENDERDOC_Key_F11: return true;
    case eRENDERDOC_Key_F12: return true;
    default: return false;
  }
}

void AddInputWindow(void *wnd)
{
}

void RemoveInputWindow(void *wnd)
{
}

/*
/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/Events.h
CGKeyCode's
kVK_F11                       = 0x67,
kVK_F12                       = 0x6F,
*/

bool GetKeyState(int key)
{
  int vk = 0;
  switch(key)
  {
    case eRENDERDOC_Key_F11: vk = 0x67; break;
    case eRENDERDOC_Key_F12: vk = 0x6F; break;
    default: break;
  }
  if(vk == 0)
    return false;
  return IsKeyPressed(vk);
}
}

namespace FileIO
{
string GetTempRootPath()
{
  return "/tmp";
}

string GetAppFolderFilename(const string &filename)
{
  passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;

  string ret = string(homedir) + "/.renderdoc/";

  mkdir(ret.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  return ret + filename;
}

void GetExecutableFilename(string &selfName)
{
  char path[512] = {0};

  uint32_t pathSize = (uint32_t)sizeof(path);
  if(_NSGetExecutablePath(path, &pathSize) == 0)
  {
    selfName = string(path);
  }
  else
  {
    pathSize++;
    char *allocPath = new char[pathSize];
    memset(allocPath, 0, pathSize);
    if(_NSGetExecutablePath(path, &pathSize) == 0)
    {
      selfName = string(path);
    }
    else
    {
      selfName = "/unknown/unknown";
      RDCERR("Can't get executable name");
      delete[] allocPath;
      return;    // don't try and readlink this
    }
    delete[] allocPath;
  }

  memset(path, 0, sizeof(path));
  readlink(selfName.c_str(), path, 511);

  if(path[0] != 0)
    selfName = string(path);
}

int LibraryLocator = 42;

void GetLibraryFilename(string &selfName)
{
  Dl_info info;
  if(dladdr(&LibraryLocator, &info))
  {
    selfName = info.dli_fname;
  }
  else
  {
    RDCERR("dladdr failed to get library path");
  }
  selfName = "";
}
};

namespace StringFormat
{
string Wide2UTF8(const std::wstring &s)
{
  RDCFATAL("Converting wide strings to UTF-8 is not supported on Apple!");
  return "";
}

void Shutdown()
{
}
};

namespace OSUtility
{
void WriteOutput(int channel, const char *str)
{
  if(channel == OSUtility::Output_StdOut)
    fprintf(stdout, "%s", str);
  else if(channel == OSUtility::Output_StdErr)
    fprintf(stderr, "%s", str);
}

uint64_t GetMachineIdent()
{
  uint64_t ret = MachineIdent_macOS;

#if defined(_M_ARM) || defined(__arm__)
  ret |= MachineIdent_Arch_ARM;
#else
  ret |= MachineIdent_Arch_x86;
#endif

#if ENABLED(RDOC_X64)
  ret |= MachineIdent_64bit;
#else
  ret |= MachineIdent_32bit;
#endif

  return ret;
}
};
