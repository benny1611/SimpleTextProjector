#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
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
#include "Poco/JSON/Stringifier.h"
#include "Poco/JSON/Array.h"
#include "CustomErrorHandler.h"
#include "Poco/Message.h"
#include "Poco/TaskManager.h"
#include "TextBoxRenderer.h"

using Poco::ErrorHandler;
using Poco::AutoPtr;
using Poco::Util::PropertyFileConfiguration;
using Poco::Logger;
using Poco::PatternFormatter;
using Poco::FormattingChannel;
using Poco::ConsoleChannel;
using Poco::Message;

// Shared variables
Mutex textMutex;
Mutex clientSetMutex;
Mutex streamingServerMutex;
Poco::TaskManager* taskManager;
ScreenStreamerTask* screenStreamerTask;
std::set<WebSocket> clients;
bool isServerRunning = false;
std::map<int, TextBoxRenderer*> renderers;
float backgroundColorR = 0.0f;
float backgroundColorG = 0.0f;
float backgroundColorB = 0.0f;
float backgroundColorA = 0.0f;
MonitorInfo monitorInfo;


// Other variables for main
float defaultWidth = 800;
float defaultHeight = 450;

GLFWmonitor** monitors;
GLFWmonitor* currentWindowMonitor;
GLFWwindow* window;

void monitor_callback(GLFWmonitor* monitor, int event);
void setMonitorJSON();

int main(int argc, char** argv) {
    FT_Library freeTypeLibrary;
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

    int err = glfwInit();
    if (err != GLFW_TRUE) {
        consoleLogger.error("GLFW init failed!");
        exit(1);
    }

    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* primaryMode = glfwGetVideoMode(primary);
    defaultWidth = (float) primaryMode->width;
    defaultHeight = (float) primaryMode->height;

    monitorInfo.monitorMutex.lock();
    monitors = glfwGetMonitors(&monitorInfo.monitorCount);
    for (int i = 0; i < monitorInfo.monitorCount; i++) {
        if (monitors[i] == primary) {
            monitorInfo.monitorIndex = i;
            break;
        }
    }
    monitorInfo.hasChanged = false;
    setMonitorJSON();
    monitorInfo.monitorMutex.unlock();

    glfwSetMonitorCallback(monitor_callback);

    window = glfwCreateWindow(defaultWidth, defaultHeight, "SimpleTextProjector", primary, NULL);

    if (window == NULL) {
        glfwTerminate();
        consoleLogger.error("Could not create the GLFW window");
        exit(1);
    }

    currentWindowMonitor = glfwGetWindowMonitor(window);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool drawDebugLines = pConf->getBool("DrawDebugLines", false);
    float fontSizeDecreaseStep = pConf->getDouble("FontSizeDecreaseStep", 5.0);


    // Initialize freetype
    int freeTypeError = FT_Init_FreeType(&freeTypeLibrary);
    if (freeTypeError) {
        consoleLogger.error("FreeType init failed with error code: " + freeTypeError);
        exit(1);
    }
    TextBoxRenderer* renderer = new TextBoxRenderer(defaultWidth, defaultHeight, defaultWidth / 4, defaultHeight / 4, defaultWidth / 2, defaultHeight / 2, &consoleLogger, freeTypeLibrary);
    //renderer->setText("Welcome to SimpleTextProjector");
    std::pair<int, TextBoxRenderer*> rendererPair(1, renderer);
    renderers.insert(rendererPair);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        monitorInfo.monitorMutex.lock();
        if (monitorInfo.hasChanged) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitorInfo.monitorIndex]);
            glfwSetWindowMonitor(window, monitors[monitorInfo.monitorIndex], 0, 0, mode->width, mode->height, mode->refreshRate);
            monitorInfo.hasChanged = false;
            renderer->setScreenSize(mode->width, mode->height);
            currentWindowMonitor = glfwGetWindowMonitor(window);
        }
        monitorInfo.monitorMutex.unlock();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        textMutex.lock();
        glClearColor(backgroundColorR, backgroundColorG, backgroundColorB, backgroundColorA);
        renderer->renderCenteredText(drawDebugLines);
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

    FT_Done_FreeType(freeTypeLibrary);

    return 0;

}

void monitor_callback(GLFWmonitor* monitor, int event) {
    monitorInfo.monitorMutex.lock();
    if (event == GLFW_CONNECTED) {
        // connected a new monitor, update monitor count
        monitors = glfwGetMonitors(&monitorInfo.monitorCount);
    } else if (event == GLFW_DISCONNECTED) {
        // The monitor was disconnected
        monitors = glfwGetMonitors(&monitorInfo.monitorCount);
        if (monitor == currentWindowMonitor) {

            // the monitor on which we had our window got disconnected, show on the primary
            GLFWmonitor* primary = glfwGetPrimaryMonitor();
            int primaryIndex = 0;

            for (int i = 0; i < monitorInfo.monitorCount; i++) {
                if (primary == monitors[i]) {
                    primaryIndex = i;
                    break;
                }
            }
            monitorInfo.monitorIndex = primaryIndex;
            monitorInfo.hasChanged = true;
        }
    }
    setMonitorJSON();
    monitorInfo.monitorMutex.unlock();


    Poco::JSON::Object::Ptr newMonitorJSON = new Poco::JSON::Object;
    if (event == GLFW_CONNECTED) {
        newMonitorJSON->set("event", "MONITOR_CONNECTED");
    } else {
        newMonitorJSON->set("event", "MONITOR_DISCONNECTED");
    }
    
    newMonitorJSON->set("monitor_count", monitorInfo.monitorCount);

    std::ostringstream oss;
    Poco::JSON::Stringifier::stringify(*newMonitorJSON, oss);
    std::string newMonitorJSONAsString = oss.str();

    std::set<WebSocket>::iterator clientIterator;
    for (clientIterator = clients.begin(); clientIterator != clients.end(); clientIterator++) {
        ((WebSocket)*clientIterator).sendFrame(newMonitorJSONAsString.c_str(), newMonitorJSONAsString.length());
    }
}

void setMonitorJSON() {
    Poco::JSON::Array monitorJsonArray;
    for (int i = 0; i < monitorInfo.monitorCount; i++) {
        Poco::JSON::Object::Ptr monitorJSON = new Poco::JSON::Object;
        const char* name = glfwGetMonitorName(monitors[i]);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        std::string completeName = std::string(name) + " " + std::to_string(mode->width) + " x " + std::to_string(mode->height) + " " + std::to_string(mode->refreshRate) + "hz";

        monitorJSON->set(std::to_string(i), completeName);
        monitorJsonArray.add(monitorJSON);
    }

    std::ostringstream oss;
    Poco::JSON::Stringifier::stringify(monitorJsonArray, oss);
    monitorInfo.monitorJSONAsString = oss.str();
}
