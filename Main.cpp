#if defined(_WIN32)           
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif

#include "raylib.h"
#include "tinythread.h"
#include "SharedVariables.h"
#include "HTTPSCommandServer.h"
#include "HTTPCommandServer.h"
#include "Poco/ErrorHandler.h"
#include "Poco/AutoPtr.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include "Poco/Logger.h"
#include "Poco/PatternFormatter.h"
#include "Poco/FormattingChannel.h"
#include "Poco/ConsoleChannel.h"
#include "CustomErrorHandler.h"
#include "Poco/Message.h"
#include "Poco/TaskManager.h"

#if defined(_WIN32)           // raylib uses these names as function parameters
#undef near
#undef far
#endif

using Poco::ErrorHandler;
using Poco::AutoPtr;
using Poco::Util::PropertyFileConfiguration;
using Poco::Logger;
using Poco::PatternFormatter;
using Poco::FormattingChannel;
using Poco::ConsoleChannel;
using Poco::Message;

void toggleFullScreenWindow(int monitor);

int defaultWidth = 800;
int defaultHeight = 450;

int currentScreenHeight;
int currentScreenWidth;
int currentMonitor = -1;

Mutex textMutex;

char* text = (char*)"\xC3\x8E\xC8\x9B\x69\x20\x6D\x75\x6C\xC8\x9B\x75\x6D\x65\x73\x63\x20\x63\xC4\x83\x20\x61\x69\x20\x61\x6C\x65\x73\x20\x72\x61\x79\x6C\x69\x62\x2E\x0A";
std::string fontPath = "fonts/Raleway.ttf";
float fontSize = 72.0f;
float baseSize = fontSize;

int main(int argc, char** argv) {
    CustomErrorHandler ceh;
    ErrorHandler::set(&ceh);

    Poco::TaskManager tm = Poco::TaskManager("STPMainTaskManager");

    // set up two channel chains - one to the
    // console and the other one to a log file.
    FormattingChannel* pFCConsole = new FormattingChannel(new PatternFormatter("[%O] %s: %p: %t"));
    pFCConsole->setChannel(new ConsoleChannel);
    pFCConsole->open();
    Logger& consoleLogger = Logger::create("SimpleTextProjector.Main", pFCConsole, Message::PRIO_INFORMATION);

    AutoPtr<PropertyFileConfiguration> pConf;
    pConf = new PropertyFileConfiguration("SimpleTextProjector.properties");
    bool useHTTP = pConf->getBool("HTTP", true);
    bool useHTTPS = pConf->getBool("HTTPS", false);

    if (useHTTP == useHTTPS) {
        consoleLogger.error("HTTP and HTTPS must be different!!");
        exit(1);
    }
    

    // Setting up the HTTP/S Server
    if (useHTTP) {
        // start HTTP Server
        HTTPCommandServer* HTTPServer = new HTTPCommandServer();
        HTTPServerTask* httpTask = new HTTPServerTask(HTTPServer, argc, argv);
        tm.start(httpTask);
    }
    else {
        // start HTTPS Server
        HTTPSCommandServer* HTTPSServer = new HTTPSCommandServer();
        HTTPSServerTask* httpsTask = new HTTPSServerTask(HTTPSServer, argc, argv);
        tm.start(httpsTask);
    }

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
    font.baseSize = 100;

    while (!WindowShouldClose()) {
        std::cout << "";
        textMutex.lock();
        if (fontPath != fontPathCopy) {
            fontPathCopy = fontPath;
            UnloadFont(font);
            font = LoadFontEx(fontPath.c_str(), 128, 0, 512);
            font.baseSize = 100;
        }
        BeginDrawing();
        ClearBackground(BLACK);
        Vector2 vec2;
        vec2.x = 190.f;
        vec2.y = 200.0f;

        DrawTextEx(font, text, vec2, fontSize, 0, LIME);
        
        EndDrawing();
        textMutex.unlock();
    }

    UnloadFont(font);

    RAYLIB_H::CloseWindow();

    //server.stop();

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