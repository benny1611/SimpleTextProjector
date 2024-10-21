## How I've built FFmpeg on Windows 10 64

Pre-requisites: 
- Visual Studio 2022
- Git Bash

1. Open x64 Native Tools Command Prompt for VS 2022 (you can just search for it in the search bar)
2. Run the following command: ```sh```
2. cd into the FFmpeg folder
3. Clone the libvpx library: ```git clone https://chromium.googlesource.com/webm/libvpx```
4. ```cd libvpx```
5. Create a folder called build if it doesn't exist and cd into it
6. Inside the build folder, run the following command: ```../configure --enable-vp9 --enable-libs --enable-static --enable-better-hw-compatibility --target=x86_64-win64-vs17```
7. Run ```make```
7. cd to the main FFmpeg folder
8. Open the configure file in an editor (eg. Notepad++)
9. Search for VPX_IMG_FMT_HIGHBITDEPTH and replace
VPX_IMG_FMT_HIGHBITDEPTH" "-lvpx $libm_extralibs $pthreads_extralibs"

with

VPX_IMG_FMT_HIGHBITDEPTH" "-lvpxmd -lmsvcrtd $libm_extralibs $pthreads_extralibs"
10. In the console run the following command: ```./configure --target-os=win64 --arch=x86_64 --toolchain=msvc --enable-shared --enable-libvpx --extra-cflags="-I./libvpx" --extra-ldflags="/LIBPATH:./libvpx/build/x64/Release"```
11. Run ```make```

Now you have compiled FFmpeg with he libvpx library