#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include <fstream>
#include FT_FREETYPE_H
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
#include "TextRenderer2D.h"


using Poco::ErrorHandler;
using Poco::AutoPtr;
using Poco::Util::PropertyFileConfiguration;
using Poco::Logger;
using Poco::PatternFormatter;
using Poco::FormattingChannel;
using Poco::ConsoleChannel;
using Poco::Message;

float defaultWidth = 800;
float defaultHeight = 450;

int currentScreenHeight;
int currentScreenWidth;
int currentMonitor = -1;

std::set<WebSocket> clients;
Mutex textMutex;
Mutex clientSetMutex;
Mutex streamingServerMutex;
Poco::TaskManager* taskManager;
ScreenStreamerTask* screenStreamerTask;

std::string* text = new std::string("Welcome to SimpleTextProjector");

unsigned char textColorR = 0xFF;
unsigned char textColorG = 0xFF;
unsigned char textColorB = 0xFF;
unsigned char textColorA = 0xFF;
std::string fontPath = "fonts/Raleway.ttf";
float fontSize = 72.0f;
float baseSize = fontSize;
bool isServerRunning = false;
int codePointsCount = 512;
int loadFontSize = 256;
GLFWmonitor** monitors;
TextRenderer2D* renderer;

void monitor_callback(GLFWmonitor* monitor, int event);
unsigned char* loadFile(const std::string& filename, size_t& fileSize);

int main(int argc, char** argv) {
    CustomErrorHandler ceh;
    ErrorHandler::set(&ceh);

    Poco::TaskManager tm = Poco::TaskManager("STPMainTaskManager");
    taskManager = &tm;

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
        taskManager->start(httpTask);
    }
    else {
        // start HTTPS Server
        HTTPSServer = new HTTPSCommandServer();
        HTTPSServerTask* httpsTask = new HTTPSServerTask(HTTPSServer, argc, argv);
        taskManager->start(httpsTask);
    }

    FT_Library freeTypeLibrary;
    int freeTypeError = FT_Init_FreeType(&freeTypeLibrary);
    if (freeTypeError) {
        consoleLogger.error("FreeType init failed with error code: " + freeTypeError);
        exit(1);
    }

    size_t fontFileSize;
    const unsigned char* fontFileBuffer = loadFile(fontPath, fontFileSize);
    if (fontFileBuffer == nullptr) {
        consoleLogger.error("Could not load font file " + fontPath + " into memory. Error code: " + std::to_string(freeTypeError));
        exit(1);
    }

    FT_Face fontFace;
    freeTypeError = FT_New_Memory_Face(freeTypeLibrary, fontFileBuffer, fontFileSize, 0, &fontFace);

    if (freeTypeError) {
        consoleLogger.error("Error loading the font from memory. Error code: " + freeTypeError);
        exit(1);
    }

    FT_Set_Pixel_Sizes(fontFace, 0, fontSize);

    int err = glfwInit();
    if (err != GLFW_TRUE) {
        consoleLogger.error("GLFW init failed!");
        exit(1);
    }

    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    defaultWidth = (float) mode->width;
    defaultHeight = (float) mode->height;

    int monitorCount;
    monitors = glfwGetMonitors(&monitorCount);

    glfwSetMonitorCallback(monitor_callback);

    GLFWwindow* window = glfwCreateWindow(defaultWidth, defaultHeight, "SimpleTextProjector", primary, NULL);

    if (window == NULL) {
        glfwTerminate();
        consoleLogger.error("Could not create the GLFW window");
        exit(1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderer = new TextRenderer2D(defaultWidth, defaultHeight, fontFace, &consoleLogger);


    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        std::cout << "";

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        textMutex.lock();
        renderer->renderCenteredText(text, 0, 0, defaultWidth, defaultHeight, fontSize, 5.0f, true);
        textMutex.unlock();

        /* Swap buffers */
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    
    if (useHTTP) {
        HTTPServer->stop();
    }
    else {
        HTTPSServer->stop();
    }

    streamingServerMutex.lock();

    if (isServerRunning) {
        streamingServerMutex.unlock();
        screenStreamerTask->cancel();
    }
    else {
        streamingServerMutex.unlock();
    }

    return 0;

}

void monitor_callback(GLFWmonitor* monitor, int event)
{
    if (event == GLFW_CONNECTED)
    {
        // The monitor was connected
    }
    else if (event == GLFW_DISCONNECTED)
    {
        // The monitor was disconnected
    }
}

// Function to load the contents of a file into memory as unsigned char*
unsigned char* loadFile(const std::string& filename, size_t& fileSize) {
    // Open the file in binary mode
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    // Check if the file was successfully opened
    if (!file.is_open()) {
        return nullptr;
    }

    // Get the size of the file
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate memory to hold the file contents
    unsigned char* buffer = new unsigned char[fileSize];

    // Read the file contents into the buffer
    if (file.read(reinterpret_cast<char*>(buffer), fileSize)) {
        return buffer;
    }
    else {
        delete[] buffer;
        return nullptr;
    }
}
