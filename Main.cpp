#if defined(_WIN32)           
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif

#include "raylib.h"
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

std::set<WebSocket> clients;
Mutex textMutex;
Mutex clientSetMutex;

char* text = (char*)"\xC3\x8E\xC8\x9B\x69\x20\x6D\x75\x6C\xC8\x9B\x75\x6D\x65\x73\x63\x20\x63\xC4\x83\x20\x61\x69\x20\x61\x6C\x65\x73\x20\x72\x61\x79\x6C\x69\x62\x2E\x0A";
std::string fontPath = "fonts/Raleway.ttf";
float fontSize = 72.0f;
float baseSize = fontSize;

static void DrawTextBoxedSelectable(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint);    // Draw text using font inside rectangle limits with support for text selection

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
    
    HTTPCommandServer* HTTPServer = NULL;
    HTTPSCommandServer* HTTPSServer = NULL;
    // Setting up the HTTP/S Server
    if (useHTTP) {
        // start HTTP Server
        HTTPServer = new HTTPCommandServer();
        HTTPServerTask* httpTask = new HTTPServerTask(HTTPServer, argc, argv);
        tm.start(httpTask);
    }
    else {
        // start HTTPS Server
        HTTPSServer = new HTTPSCommandServer();
        HTTPSServerTask* httpsTask = new HTTPSServerTask(HTTPSServer, argc, argv);
        tm.start(httpsTask);
    }

    int currentMonitor = GetCurrentMonitor();
    defaultWidth = GetScreenWidth();
    defaultHeight = GetScreenHeight();
    InitWindow(defaultWidth, defaultHeight, "SimpleTextProjector");
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
        Vector2 textMeasurement = MeasureTextEx(font, text, fontSize, 0);
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        Vector2 vec2;
        vec2.x = (screenWidth/2) - (textMeasurement.x/2);
        vec2.y = (screenHeight/2) - (textMeasurement.y/2);
        Vector2 center;
        center.x = (screenWidth / 2);
        center.y = (screenHeight / 2);
        
        //DrawTextEx(font, text, vec2, fontSize, 0, LIME);
        Rectangle rec;
        rec.x = 0;
        rec.y = 0;
        rec.width = screenWidth;
        rec.height = screenHeight;
        DrawTextBoxedSelectable(font, text, rec, fontSize, 0, true, LIME, 0, 0, BLACK, BLACK);
        DrawLine(screenWidth / 2, 0, screenWidth / 2, screenHeight, WHITE);
        DrawLine(0, screenHeight / 2, screenWidth, screenHeight / 2, WHITE);
        
        EndDrawing();
        textMutex.unlock();
    }

    UnloadFont(font);

    RAYLIB_H::CloseWindow();

    if (useHTTP) {
        HTTPServer->stop();
    }
    else {
        HTTPSServer->stop();
    }

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

static void DrawTextBoxedSelectable(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint) {

    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0;          // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw

    float scaleFactor = fontSize / (float)font.baseSize;     // Character rectangle scaling factor

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap ? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;         // Index where to begin drawing (where a line begins)
    int endLine = -1;           // Index where to stop drawing (where a line ends)
    int lastk = -1;             // Holds last value of the character position
    float sentenceWidth = 0.0f;

    for (int i = 0, k = 0; i < length; i++, k++) {

        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;
        i += (codepointByteCount - 1);

        float glyphWidth = 0;
        if (codepoint != '\n') {
            if (font.glyphs[index].advanceX == 0) {
                glyphWidth = font.recs[index].width * scaleFactor;
            } else {
                glyphWidth = font.glyphs[index].advanceX * scaleFactor;
            }

            if (i + 1 < length) {
                glyphWidth = glyphWidth + spacing;
            }
        }

        // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in startLine and endLine, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_STATE) {

            // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
            // Ref: http://jkorpela.fi/chars/spaces.html
            if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) {
                endLine = i;
            }

            if ((textOffsetX + glyphWidth) > rec.width) {
                if (endLine < 1) {
                    endLine = i;
                }

                if (i == endLine) {
                    endLine -= codepointByteCount;
                }
                if ((startLine + codepointByteCount) == endLine) {
                    endLine = (i - codepointByteCount);
                }

                state = !state;
            } else if ((i + 1) == length) {
                endLine = i;
                state = !state;
            } else if (codepoint == '\n') {
                state = !state;
            }

            if (state == DRAW_STATE) {
                // done measuring
                sentenceWidth = textOffsetX;
                textOffsetX = 0;
                i = startLine;
                glyphWidth = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        } else {
            if (codepoint == '\n') {
                if (!wordWrap) {
                    textOffsetY += ((font.baseSize + font.baseSize / 2) * scaleFactor) + (rec.height / 2);
                    textOffsetX = (rec.width / 2) - (sentenceWidth / 2);
                }
            } else {
                if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width)) { // jump to next line
                    textOffsetY += ((font.baseSize + font.baseSize / 2) * scaleFactor) + (rec.height / 2);
                    textOffsetX = (rec.width / 2) - (sentenceWidth / 2);
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + font.baseSize * scaleFactor) > rec.height) {
                    break;
                }

                // Draw selection background
                bool isGlyphSelected = false;
                if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength))) {
                    Rectangle rect;
                    rect.x = rec.x + textOffsetX - 1;
                    rect.y = rec.y + textOffsetY;
                    rect.width = glyphWidth;
                    rect.height = (float)font.baseSize * scaleFactor;
                    DrawRectangleRec(rect, selectBackTint);
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t')) {
                    Vector2 vec2;
                    vec2.x = rec.x + textOffsetX + (rec.width / 2) - (sentenceWidth / 2);
                    vec2.y = rec.y + textOffsetY + (rec.height / 2) - (font.baseSize/2);
                    DrawTextCodepoint(font, codepoint, vec2, fontSize, isGlyphSelected ? selectTint : tint);
                }
            }

            if (wordWrap && (i == endLine)) {
                textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                textOffsetX = 0;
                startLine = endLine;
                endLine = -1;
                glyphWidth = 0;
                selectStart += lastk - k;
                k = lastk;

                state = !state;
            }
        }

        if ((textOffsetX != 0) || (codepoint != ' ')) {
            textOffsetX += glyphWidth;  // avoid leading spaces
        }
    }
}