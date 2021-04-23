## Mac Issues (in priority order)

&#9744; A couple of key functions in GL rely on compute shaders which thus aren't implemented on macOS. Namely: min/max texture range, histogram, and mesh picking. The simplest workaround is just to emulate on the CPU and take the performance hit on that, but it's possible the min/max/histogram shaders could still run on the GPU with pixel shaders doing a smaller down sample pyramid.

&#9744; Update Mac build instructions for latest MacOS, Xcode 12 and Xcode project generation options.

&#9744; The build needs significantly more testing and bug fixing. At the moment I've really only tried it on a handful of demo programs, so more real-world testing is desperately needed. If you have any non-trivial projects please share them so I can use them for testing.

&#9744; Implement code signing for librenderdoc.dylib to allow capturing of signed applications ie. MoltenVK samples installed without being compiled locally.

&#9744; Implement universal binary (arm64 & x86_64) support for the nightly builds for qrenderdoc

&#9744; Some modern mac programs are likely to require OpenCL interop for compute, which would need at least minimal hooking to serialise data at sync points (similar to the DX interop).

&#9744; The display is not correctly resizing Mesh & Texture Viewer windows until a manual repaint is done ie. clicking on an event ID 

&#9745; Currently the Xcode generator in cmake doesn't produce a working build. It seems to run into issues with the GL/Vulkan driver projects and doesn't build them correctly. If there is a simple/minimal impact fix to get this working then that would be great, but since it compiles fine on the command line this isn't critical.

&#9745; Some dialog boxes in the Qt UI have a large question mark - I don't know if that's normal for the OS or if that's a missing resource/set up on my side to provide RenderDoc's icon. A: Looks like the Qt default QMessageBox::question on Mac

&#9745; This also just means testing the build for any weird UI bugs or strange behaviour on mac. E.g. I seem to sometimes get 'sticky' context menus where right clicking will pop one up but then left clicking will continue to pop it up. This might be a holdover of mac not really having right clicking as a very native concept? A: Look to be bugs in Qt Mac implementation.

&#9745; qrenderdoc crashes if you put a unicode character into the capture save name

&#9745; When launching .app folders we need to parse the Info.plist to determine the executable to launch.

&#9745; On GL we always create a 4.1 core context whenever we need one. In theory we should work OK on 3.3 as well - is it necessary to detect what versions are available and downscale, or are 3.3 devices/OSs not relevant any more?

&#9745; Performance in the UI seems very slow for some reason. I don't know if it's because of my hardware though.

&#9745; Lock Mac SDK to 10.15 for compiling and build

&#9745; Is there a minimum macOS version that should be tested against? what about minimum hardware support? Currently I only have one apple device - a mid-2013 macbook air running 10.14.3. This is probably close to the minimum end on hardware I'd guess but it's the latest OS.

&#9745; Implement universal binary (arm64 & x86_64) support for the nightly builds for librenderdoc.dylib not qrenderdoc

&#9745; Key input in the injected application for triggering captures is not supported yet, so the only way to trigger captures is via the UI. I'm not sure what library/input method is best for minimally-intrusive keyboard input in windows created by the application that I don't control.

&#9745; High-DPI screens aren't handled properly in at least the GL overlay, we need to detect the DPI scaling factor and multiply up the detected window bounds and the size of the rendering.

&#9745; High-DPI screens may also have issues with sizing widgets in the UI, and with rendering displays on at least GL and maybe Vulkan.

&#9745; renderdoccmd has some stubs that need to find a way to create a window on apple to display previews.
