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

char* text = new char[100];
std::string fontPath = "fonts/Raleway.ttf";
float fontSize = 72.0f;
float baseSize = fontSize;

static void DrawTextBoxedSelectable(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint);    // Draw text using font inside rectangle limits with support for text selection
static void DrawTextCenteredInARectangle(Font font, Rectangle rec, float spacing, float desiredFontSize, bool wordWrap, Color textColor);

int main(int argc, char** argv) {
    char* initialText = (char*)"\xC3\x8E\xC8\x9B\x69\x20\x6D\x75\x6C\xC8\x9B\x75\x6D\x65\x73\x63\x20\x63\xC4\x83\x20\x61\x69\x20\x61\x6C\x65\x73\x20\x72\x61\x79\x6C\x69\x62\x2E\x0A";
    int initTextLength = strlen(initialText);
    memcpy_s(text, initTextLength, initialText, initTextLength);
    text[initTextLength] = '\0';

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
        //DrawTextBoxedSelectable(font, text, rec, fontSize, 0, true, LIME, 0, 0, BLACK, BLACK);
        DrawTextCenteredInARectangle(font, rec, 0, fontSize, true, LIME);
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

static void DrawTextCenteredInARectangle(Font font, Rectangle rec, float spacing, float desiredFontSize, bool wordWrap, Color textColor) {
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop
    float scaleFactor = desiredFontSize / (float)font.baseSize;     // Character rectangle scaling factor

    // copy text in a separate variable
    char* resultText = new char[length];
    memcpy_s(resultText, length, text, length);
    //strncpy_s(resultText, length, text, length);

    bool doneAdjustingTextWidth = false;
    bool doneAdjustingTextHeight = false;
    bool doneDrawing = false;

    float textHeight = -1;
    std::list<int> newLineIndexList;
    std::list<float> linesWidthList;

    while (!doneAdjustingTextWidth || !doneAdjustingTextHeight || !doneDrawing) {
        if (!doneAdjustingTextWidth) {
            float sentenceWidth = 0.0f;
            int lastSpaceChIdx = -1;

            for (int i = 0; i < length; i++) {
                // Get next codepoint from byte string and glyph index in font
                int codepointByteCount = 0;
                int codepoint = GetCodepoint(&resultText[i], &codepointByteCount);
                int index = GetGlyphIndex(font, codepoint);

                // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
                // but we need to draw all of the bad bytes using the '?' symbol moving one byte
                if (codepoint == 0x3f) {
                    codepointByteCount = 1;
                }
                i += (codepointByteCount - 1);

                float glyphWidth = 0;
                if (codepoint != '\n') {
                    if (font.glyphs[index].advanceX == 0) {
                        glyphWidth = font.recs[index].width * scaleFactor;
                    }
                    else {
                        glyphWidth = font.glyphs[index].advanceX * scaleFactor;
                    }

                    if (i + 1 < length) {
                        glyphWidth = glyphWidth + spacing;
                    }
                } else {
                    sentenceWidth += glyphWidth;
                    linesWidthList.push_back(sentenceWidth);
                    sentenceWidth = 0.0f;
                }

                sentenceWidth += glyphWidth;  // avoid leading spaces
                
                if (codepoint == ' ') {
                    lastSpaceChIdx = i;  // save the position of the last space character
                }

                if (!wordWrap) {
                    lastSpaceChIdx = i; // override the position of the last space character if wordWrap is off
                }

                if (sentenceWidth > rec.width) { // add a newline either now or where the last space was
                    bool noSpaceBefore = lastSpaceChIdx == -1;
                    if (noSpaceBefore) { // turn off the wordWrap for this word if the word is too long
                        lastSpaceChIdx = i;
                    }

                    int indexToCopyFrom = lastSpaceChIdx;

                    //int lengthOfResultText = strlen(resultText);
                    char* tmp = new char[length + 1];
                    memcpy_s(tmp, indexToCopyFrom, resultText, indexToCopyFrom);

                    tmp[indexToCopyFrom] = '\n';
                    indexToCopyFrom++;
                    if (!wordWrap || noSpaceBefore) { // we add a \n here 
                        memcpy(tmp + indexToCopyFrom, resultText + indexToCopyFrom - 1, length - indexToCopyFrom + 1);
                        length++;
                    } else { // we substitute the space with a \n here
                        memcpy(tmp + indexToCopyFrom, resultText + indexToCopyFrom, length - indexToCopyFrom);
                    }
                    
                    delete[] resultText;
                    resultText = tmp;
                    linesWidthList.clear();
                    break;
                }

                if (i >= length - 1) {
                    doneAdjustingTextWidth = true;
                    linesWidthList.push_back(sentenceWidth);
                }
            }
        }

        if (doneAdjustingTextWidth && !doneAdjustingTextHeight) {
            for (int i = 0; i < length; i++) {
                int codepointByteCount = 0;
                int codepoint = GetCodepoint(&resultText[i], &codepointByteCount);
                if (codepoint == '\n') {
                    newLineIndexList.push_back(i);
                }
            }
            float initialTextHeight = (font.baseSize + (font.baseSize/2)) * newLineIndexList.size();
            if (initialTextHeight > rec.height) {
                desiredFontSize -= 5;
                if (desiredFontSize <= 0) {
                    // cannot fit text to rectangle
                    // TODO: Show nothing
                    return;
                }
                scaleFactor = desiredFontSize / (float)font.baseSize;
                delete[] resultText;
                resultText = new char[length];
                memcpy_s(resultText, length, text, length);
                doneAdjustingTextHeight = false;
                doneAdjustingTextWidth = false;
                newLineIndexList.clear();
                linesWidthList.clear();
            } else {
                doneAdjustingTextHeight = true;
                textHeight = initialTextHeight;
            }
        }

        if (doneAdjustingTextWidth && doneAdjustingTextHeight) {
            int lineWidthIndex = 0;
            int positionIndex = 0;
            int numberOfLinesThatCanBeDraw = 0;
            for (std::list<float>::iterator it = linesWidthList.begin(); it != linesWidthList.end(); ++it) {
                if (*it != 0) {
                    numberOfLinesThatCanBeDraw++;
                }
            }
            float offsetY = ((float)(font.baseSize * (numberOfLinesThatCanBeDraw - 1)))/2.0f;
            std::list<int>::iterator it = newLineIndexList.begin();
            while (positionIndex <= newLineIndexList.size()) {

                int startPosition;
                int endPosition;
                if (it == newLineIndexList.begin()) {
                    startPosition = 0;
                    endPosition = *it;
                } else if(it == newLineIndexList.end()) {
                    startPosition = *newLineIndexList.rbegin();
                    endPosition = length;
                } else {
                    std::list<int>::iterator it2 = newLineIndexList.begin();
                    std::advance(it2, positionIndex - 1);
                    startPosition = *it2;
                    if (it == newLineIndexList.end()) {
                        endPosition = *newLineIndexList.rbegin();
                    } else {
                        endPosition = *it;
                    }
                }
                
                std::list<float>::iterator lineWidthIt = linesWidthList.begin();
                std::advance(lineWidthIt, lineWidthIndex);
                lineWidthIndex++;

                float sentenceWidth = *lineWidthIt;
                float offsetX = 0.0f;
                if (sentenceWidth == 0) {
                    positionIndex++;
                    continue;
                }

                for (int i = startPosition; i < endPosition; i++) {
                    int codepointByteCount = 0;
                    int codepoint = GetCodepoint(&resultText[i], &codepointByteCount);
                    if ((codepoint != ' ') && (codepoint != '\t') && codepoint != '\n') {
                        Vector2 vec2;
                        vec2.x = rec.x + offsetX + (rec.width / 2) - (sentenceWidth / 2);
                        vec2.y = rec.y - offsetY + (rec.height / 2) - (font.baseSize/2);
                        DrawTextCodepoint(font, codepoint, vec2, desiredFontSize, textColor);
                    }
                    float glyphWidth = 0;
                    int index = GetGlyphIndex(font, codepoint);
                    if (codepoint != '\n') {
                        if (font.glyphs[i].advanceX == 0) {
                            glyphWidth = font.recs[index].width * scaleFactor;
                        } else {
                            glyphWidth = font.glyphs[index].advanceX * scaleFactor;
                        }

                        if (i + 1 < length) {
                            glyphWidth = glyphWidth + spacing;
                        }
                    }
                    offsetX += glyphWidth;
                    if (codepoint == 0x3f) {
                        codepointByteCount = 1;
                    }
                    i += (codepointByteCount - 1);
                }
                offsetY -= font.baseSize;
                positionIndex++;
                if (it != newLineIndexList.end()) {
                    std::advance(it, 1);
                }
            }

            doneDrawing = true;
        }
    }
}
