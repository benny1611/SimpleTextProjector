#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "SharedVariables.h"
#include "HTTPSCommandServer.h"
#include "HTTPCommandServer.h"
#include "Poco/ErrorHandler.h"

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
#include "Poco/Exception.h"
#include "Poco/Net/NetworkInterface.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl2.h"

#include "SimpleTextProjectorUI.h"

using Poco::ErrorHandler;
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
AutoPtr<PropertyFileConfiguration> pConf;


// Other variables for main
float defaultWidth = 800;
float defaultHeight = 450;
bool shouldCloseUI = false;

GLFWmonitor** monitors;
GLFWmonitor* currentWindowMonitor;
GLFWwindow* window;

void monitor_callback(GLFWmonitor* monitor, int event);
void setMonitorJSON();
void createUIWindow(GLFWwindow*& uiWindow, GLFWmonitor* primaryMonitor, SimpleTextProjectorUI*& ui, std::string url, bool& shouldCloseUI, bool& isCheckBoxTicked);
void uiWindowCloseCallback(GLFWwindow* uiWindow);
static void glfw_error_callback(int error, const char* description);

int RealMain(int argc, char** argv);

int main(int argc, char** argv) {
    try {
        return RealMain(argc, argv); // or your actual code
    } catch (const std::exception& ex) {
        std::cerr << "Uncaught std::exception: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception caught!" << std::endl;
    }
    return 1;
}


int RealMain(int argc, char** argv) {
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

    std::string propertyFilePath = "SimpleTextProjector.properties";

    pConf = new PropertyFileConfiguration(propertyFilePath);
    bool useHTTP = pConf->getBool("HTTP", true);
    bool useHTTPS = pConf->getBool("HTTPS", false);

    if (useHTTP == useHTTPS) {
        consoleLogger.error("HTTP and HTTPS must be different!!");
        exit(1);
    }

    std::string url = "";
    try {
        url = pConf->getString("hostname");
    } catch (Poco::NotFoundException e) {
        // Ignore
    }

    if (url.empty()) {
        Poco::Net::NetworkInterface::Map interfaces = Poco::Net::NetworkInterface::map();
        for (const auto& entry : interfaces) {
            const Poco::Net::NetworkInterface& netInterface = entry.second;
            if (netInterface.supportsIPv4() && !netInterface.isLoopback() && netInterface.isRunning()) {
                if (useHTTP) {
                    url = "http://" + netInterface.address().toString();
                }
                else {
                    url = "https://" + netInterface.address().toString();
                }
            }
        }
    }

    std::string portKey;
    if (useHTTP) {
        portKey = "HTTPCommandServer.port";
    }
    else {
        portKey = "HTTPSCommandServer.port";
    }
    int port = -1;
    try {
        port = pConf->getInt(portKey);
    }
    catch (Poco::NotFoundException) {
        // ignore
    }
    if (port > 0) {
        if ((port != 80 && useHTTP) || (port != 443 && useHTTPS) ) {
            url += ":" + std::to_string(port);
        }
    }

    HTTPCommandServer* HTTPServer = NULL;
    HTTPSCommandServer* HTTPSServer = NULL;
    // Setting up the HTTP/S Server
    if (useHTTP) {
        // start HTTP Server
        HTTPServer = new HTTPCommandServer(url);
        HTTPServerTask* httpTask = new HTTPServerTask(HTTPServer, argc, argv);
        taskManager->start(httpTask);
    }
    else {
        // start HTTPS Server
        HTTPSServer = new HTTPSCommandServer(url);
        HTTPSServerTask* httpsTask = new HTTPSServerTask(HTTPSServer, argc, argv);
        taskManager->start(httpsTask);
    }

    glfwSetErrorCallback(glfw_error_callback);

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
    monitorInfo.monitorHeight = primaryMode->height;
    monitorInfo.monitorWidth = primaryMode->width;
    monitorInfo.refreshRate = primaryMode->refreshRate;
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


    bool drawDebugLines = pConf->getBool("DrawDebugLines", false);
    float fontSizeDecreaseStep = pConf->getDouble("FontSizeDecreaseStep", 5.0);


    // Initialize freetype
    int freeTypeError = FT_Init_FreeType(&freeTypeLibrary);
    if (freeTypeError) {
        consoleLogger.error("FreeType init failed with error code: " + freeTypeError);
        exit(1);
    }
    //TextBoxRenderer* renderer = new TextBoxRenderer(defaultWidth, defaultHeight, defaultWidth / 4, defaultHeight / 4, defaultWidth / 2, defaultHeight / 2, &consoleLogger, freeTypeLibrary);
    TextBoxRenderer* renderer = new TextBoxRenderer(defaultWidth, defaultHeight, 0, 0, defaultWidth / 2, defaultHeight / 2, &consoleLogger, freeTypeLibrary);
    //renderer->setText("Welcome to SimpleTextProjector");
    std::pair rendererPair(0, renderer);
    renderers.insert(rendererPair);

    bool showGreetingWindow = pConf->getBool("ShowGreetingWindow", true);

    // UI window
    bool isCheckBoxTicked = false;
    GLFWwindow* uiWindow = nullptr;
    SimpleTextProjectorUI* ui = nullptr;

    if (showGreetingWindow) {
        createUIWindow(uiWindow, primary, ui, url, shouldCloseUI, isCheckBoxTicked);
    }

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        monitorInfo.monitorMutex.lock();
        if (monitorInfo.hasChanged) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitorInfo.monitorIndex]);
            glfwSetWindowMonitor(window, monitors[monitorInfo.monitorIndex], 0, 0, mode->width, mode->height, mode->refreshRate);
            monitorInfo.hasChanged = false;
            monitorInfo.monitorHeight = mode->height;
            monitorInfo.monitorWidth = mode->width;
            monitorInfo.refreshRate = mode->refreshRate;
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

        if (showGreetingWindow) {
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            ui->draw();

            glfwMakeContextCurrent(uiWindow);
            glfwSwapBuffers(uiWindow);
            if (shouldCloseUI) {
                showGreetingWindow = false;
                delete ui;
                glfwDestroyWindow(uiWindow);
                if (isCheckBoxTicked) {
                    pConf->setBool("ShowGreetingWindow", false);
                    pConf->save(propertyFilePath);
                }
            }
        }
    }

    if (showGreetingWindow) {
        delete ui;
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);

    if (showGreetingWindow) {
        glfwDestroyWindow(uiWindow);
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
            const GLFWvidmode* primaryMode = glfwGetVideoMode(primary);
            monitorInfo.monitorHeight = primaryMode->height;
            monitorInfo.monitorWidth = primaryMode->width;
            monitorInfo.refreshRate = primaryMode->refreshRate;
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

    newMonitorJSON->set("width", monitorInfo.monitorWidth);
    newMonitorJSON->set("height", monitorInfo.monitorHeight);
    newMonitorJSON->set("refresh_rate", monitorInfo.refreshRate);

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

    Poco::JSON::Object::Ptr monitorsJSONMain = new Poco::JSON::Object;
    monitorsJSONMain->set("monitors", monitorJsonArray);

    std::ostringstream oss;
    Poco::JSON::Stringifier::stringify(monitorsJSONMain, oss);
    monitorInfo.monitorJSONAsString = oss.str();
}

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void createUIWindow(GLFWwindow*& uiWindow, GLFWmonitor* primaryMonitor, SimpleTextProjectorUI*& ui, std::string url, bool& shouldCloseUI, bool& isCheckBoxTicked) {
    uiWindow = glfwCreateWindow(1280, 720, "Simple Text Projector", nullptr, nullptr);

    glfwSetWindowCloseCallback(uiWindow, uiWindowCloseCallback);

    glfwMakeContextCurrent(uiWindow);

    GLFWmonitor* uiMonitor = primaryMonitor;

    for (int i = 0; i < monitorInfo.monitorCount; i++) {
        if (primaryMonitor != monitors[i]) {
            uiMonitor = monitors[i];
            break;
        }
    }

    int uiWindowX, uiWindowY;
    glfwGetMonitorPos(uiMonitor, &uiWindowX, &uiWindowY);
    const GLFWvidmode* uiMonitorMode = glfwGetVideoMode(uiMonitor);
    int uiWindowWidth, uiWindowHeight;
    glfwGetWindowSize(uiWindow, &uiWindowWidth, &uiWindowHeight);

    uiWindowX = uiWindowX + (uiMonitorMode->width / 2) - (uiWindowWidth / 2);
    uiWindowY = uiWindowY + (uiMonitorMode->height / 2) - (uiWindowHeight / 2);

    glfwSetWindowPos(uiWindow, uiWindowX, uiWindowY);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(uiWindow, true);
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    io.Fonts->AddFontDefault();
    ImFont* titleFont = io.Fonts->AddFontFromFileTTF("./fonts/Raleway.ttf", 32.0f);
    ImFont* paragraphFont = io.Fonts->AddFontFromFileTTF("./fonts/Raleway.ttf", 24.0f);
    ImFont* buttonFont = io.Fonts->AddFontFromFileTTF("./fonts/Raleway.ttf", 16.0f);
    io.Fonts->Build();

    ImGui_ImplOpenGL2_CreateFontsTexture();

    ui = new SimpleTextProjectorUI(uiWindow, titleFont, paragraphFont, buttonFont, url, &shouldCloseUI, &isCheckBoxTicked);
}

void uiWindowCloseCallback(GLFWwindow* uiWindow) {
    shouldCloseUI = true;

}