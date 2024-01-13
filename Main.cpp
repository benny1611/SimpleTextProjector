#include "raylib.h"
#include "Server.h"
#include<iostream>

#if defined(_WIN32)           
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif

#include "Poco/Net/TCPServer.h"
#include "Poco/Thread.h"
#include "Poco/RunnableAdapter.h"

#if defined(_WIN32)           // raylib uses these names as function parameters
#undef near
#undef far
#endif

using Poco::Thread;
using Poco::RunnableAdapter;

void toggleFullScreenWindow(int monitor);
static int* CodepointRemoveDuplicates(int* codepoints, int codepointCount, int* codepointResultCount);

int defaultWidth = 800;
int defaultHeight = 450;

int currentScreenHeight;
int currentScreenWidth;
int currentMonitor = -1;

int main(void) {

    // Setting up the TCP Server
    Server server;
    server.setPort(8080);
    RunnableAdapter<Server> serverStart(server, &Server::start);
    // Starting the TCP Server in a new thread
    Thread serverThread;
    serverThread.start(serverStart);


    const char* text = "\xC3\x8E\xC8\x9B\x69\x20\x6D\x75\x6C\xC8\x9B\x75\x6D\x65\x73\x63\x20\x63\xC4\x83\x20\x61\x69\x20\x61\x6C\x65\x73\x20\x72\x61\x79\x6C\x69\x62\x2E\x0A";
    int currentMonitor = 0;
    defaultWidth = GetMonitorWidth(currentMonitor);
    defaultHeight = GetMonitorHeight(currentMonitor);
    InitWindow(defaultWidth, defaultHeight, "raylib [core] example - basic window");
    SetTargetFPS(60);
    
    // keep fullscreen on focus
    SetWindowState(FLAG_WINDOW_TOPMOST);

    toggleFullScreenWindow(0);
    int monitors = GetMonitorCount();
    std::cout << "Here: " << monitors << std::endl;

    // Get codepoints from text
    int codepointCount = 0;
    int* codepoints = LoadCodepoints(text, &codepointCount);

    // Removed duplicate codepoints to generate smaller font atlas
    int codepointsNoDupsCount = 0;
    int* codepointsNoDups = CodepointRemoveDuplicates(codepoints, codepointCount, &codepointsNoDupsCount);
    UnloadCodepoints(codepoints);

    RAYLIB_H::Font font = LoadFontEx("fonts/Raleway.ttf", 72, codepointsNoDups, codepointsNoDupsCount);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        Vector2 vec2;
        vec2.x = 190.f;
        vec2.y = 200.0f;

        DrawTextEx(font, text, vec2, 72.0f, 0, LIME);
        
        EndDrawing();
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

// Remove codepoint duplicates if requested
// WARNING: This process could be a bit slow if there text to process is very long
static int* CodepointRemoveDuplicates(int* codepoints, int codepointCount, int* codepointsResultCount)
{
    int codepointsNoDupsCount = codepointCount;
    int* codepointsNoDups = (int*)calloc(codepointCount, sizeof(int));
    memcpy(codepointsNoDups, codepoints, codepointCount * sizeof(int));

    // Remove duplicates
    for (int i = 0; i < codepointsNoDupsCount; i++)
    {
        for (int j = i + 1; j < codepointsNoDupsCount; j++)
        {
            if (codepointsNoDups[i] == codepointsNoDups[j])
            {
                for (int k = j; k < codepointsNoDupsCount; k++) codepointsNoDups[k] = codepointsNoDups[k + 1];

                codepointsNoDupsCount--;
                j--;
            }
        }
    }

    // NOTE: The size of codepointsNoDups is the same as original array but
    // only required positions are filled (codepointsNoDupsCount)

    *codepointsResultCount = codepointsNoDupsCount;
    return codepointsNoDups;
}