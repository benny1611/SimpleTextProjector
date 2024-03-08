#if defined(_WIN32)           
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif

#include "TCPServer.h"
#include "raylib.h"
#include<iostream>
#include "tinythread.h"


#if defined(_WIN32)           // raylib uses these names as function parameters
#undef near
#undef far
#endif


using Mutex = tthread::mutex;

void toggleFullScreenWindow(int monitor);

int defaultWidth = 800;
int defaultHeight = 450;

int currentScreenHeight;
int currentScreenWidth;
int currentMonitor = -1;

char* text = (char*)"\xC3\x8E\xC8\x9B\x69\x20\x6D\x75\x6C\xC8\x9B\x75\x6D\x65\x73\x63\x20\x63\xC4\x83\x20\x61\x69\x20\x61\x6C\x65\x73\x20\x72\x61\x79\x6C\x69\x62\x2E\x0A";
std::string fontPath = "fonts/Raleway.ttf";

int main(void) {

    Mutex textMutex;

    // Setting up the TCP Server
    TCPServer server = TCPServer(8080, &textMutex, &text, &fontPath);
    server.start();

    int currentMonitor = 0;
    defaultWidth = GetMonitorWidth(currentMonitor);
    defaultHeight = GetMonitorHeight(currentMonitor);
    InitWindow(defaultWidth, defaultHeight, "raylib [core] example - basic window");
    SetTargetFPS(60);
    
    // keep fullscreen on focus
    SetWindowState(FLAG_WINDOW_TOPMOST);

    toggleFullScreenWindow(0);
    int monitors = GetMonitorCount();

    std::string fontPathCopy = fontPath;

    RAYLIB_H::Font font = LoadFontEx(fontPath.c_str(), 128, 0, 512);
    font.baseSize = 72;

    while (!WindowShouldClose()) {
        textMutex.lock();
        if (fontPath != fontPathCopy) {
            fontPathCopy = fontPath;
            font = LoadFontEx(fontPath.c_str(), 128, 0, 512);
            font.baseSize = 72;
        }
        BeginDrawing();
        ClearBackground(BLACK);
        Vector2 vec2;
        vec2.x = 190.f;
        vec2.y = 200.0f;

        DrawTextEx(font, text, vec2, 72.0f, 0, LIME);
        
        EndDrawing();
        textMutex.unlock();
    }

    UnloadFont(font);

    RAYLIB_H::CloseWindow();

    server.stop();

    return 0;
}

void toggleFullScreenWindow(int monitor) {
    if (currentMonitor != monitor) {
        currentMonitor = monitor;
        SetWindowMonitor(monitor);
        currentScreenWidth = GetMonitorWidth(monitor);
        currentScreenHeight = GetMonitorHeight(monitor);
        SetWindowSize(currentScreenWidth, currentScreenHeight);
        ToggleFullscreen();
    }
}