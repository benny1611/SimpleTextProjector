#include "SimpleTextProjectorUI.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl2.h"
#include "qrcodegen.hpp"
#include <cstdlib>
#include <string>

using namespace qrcodegen;

SimpleTextProjectorUI::SimpleTextProjectorUI(GLFWwindow* uiWindow, ImFont* titleFont, ImFont* paragraphFont, ImFont* buttonFont, std::string url, bool* shouldCloseUI, bool* checkBoxTicked) : uiWindow(uiWindow), titleFont(titleFont), paragraphFont(paragraphFont), url(url), shouldCloseUI(shouldCloseUI), buttonFont(buttonFont), checkBoxTicked(checkBoxTicked) {
    glfwSetWindowAttrib(uiWindow, GLFW_RESIZABLE, false);
    qrTexture = generateQrTexture(url.c_str());
}

void SimpleTextProjectorUI::draw() {
    if (glfwGetWindowAttrib(uiWindow, GLFW_ICONIFIED) != 0) {
        ImGui_ImplGlfw_Sleep(10);
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    int width, height;
    glfwGetFramebufferSize(uiWindow, &width, &height); // Get current GLFW window size

    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at top-left
    ImGui::SetNextWindowSize(ImVec2(width, height)); // Match GLFW size

    ImGui::Begin("Simple Text Projector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::PushFont(titleFont);

    
    const char* title = "Welcome to Simple Text Projector";
    ImVec2 titleSize = ImGui::CalcTextSize(title);

    float windowWidth = ImGui::GetWindowSize().x;
    float titleX = (windowWidth - titleSize.x) * 0.5f;

    ImGui::SetCursorPosX(titleX);
    ImGui::Text("%s", title); 
    ImGui::PopFont();


    ImGui::PushFont(paragraphFont);

    float windowHeight = ImGui::GetWindowSize().y;
    float spacerHeight = windowHeight / 8.0f;

    // Add vertical space
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacerHeight);

    std::string linkP = "You can access the interface by accessing this link: ";

    // Calculate individual text sizes
    ImVec2 linkPSize = ImGui::CalcTextSize(linkP.c_str());
    ImVec2 urlSize = ImGui::CalcTextSize(url.c_str());

    float totalTextWidth = linkPSize.x + urlSize.x;

    // Center both texts together
    float linkPX = (windowWidth - totalTextWidth) * 0.5f;
    ImGui::SetCursorPosX(linkPX);

    // Print the normal text
    ImGui::Text("%s", linkP.c_str());
    ImGui::SameLine();

    ImVec4 linkColor = ImVec4(0.1f, 0.5f, 1.0f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, linkColor);
    ImGui::Text("%s", url.c_str());
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    if (ImGui::IsItemClicked()) {
#ifdef _WIN32
        system(("start " + url).c_str());
#elif __APPLE__
        system(("open " + url).c_str());
#else
        system(("xdg-open " + url).c_str());
#endif
    }


    ImGui::NewLine();

    std::string qrCodeP = "Or scan this QR code:";

    ImVec2 qrCodePSize = ImGui::CalcTextSize(qrCodeP.c_str());

    float qrCodePX = (windowWidth - qrCodePSize.x) * 0.5f;

    ImGui::SetCursorPosX(qrCodePX);
    ImGui::Text("%s", qrCodeP.c_str());

    ImVec2 availableSpace = ImGui::GetContentRegionAvail();

    float qrXOffset = (availableSpace.x - 256) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + qrXOffset);

    ImGui::Image(qrTexture, ImVec2(256,256));


    ImGui::NewLine();
    std::string checkBoxText = "Don't show this window on startup anymore ";
    ImVec2 checkBoxTextSize = ImGui::CalcTextSize(checkBoxText.c_str());
    float checkBoxTextX = (windowWidth - checkBoxTextSize.x) * 0.5;
    ImGui::SetCursorPosX(checkBoxTextX);
    ImGui::Text(checkBoxText.c_str());
    ImGui::SameLine();
    ImGui::Checkbox("##", checkBoxTicked);

    ImGui::PopFont();

    ImGui::PushFont(buttonFont);

    float buttonHeight = 30.0f;
    float spacing = 10.0f;
    float availableHeight = ImGui::GetContentRegionAvail().y;

    // Move cursor to bottom area for button placement
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availableHeight - buttonHeight - spacing);

    // Center the button horizontally
    float buttonWidth = 100.0f; // Adjust as needed
    float buttonX = (windowWidth - buttonWidth) * 0.5f;
    ImGui::SetCursorPosX(buttonX);

    if (ImGui::Button("Close", ImVec2(buttonWidth, buttonHeight))) {
        *shouldCloseUI = true;
    }

    ImGui::PopFont();
    ImGui::End();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(uiWindow, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}


GLuint SimpleTextProjectorUI::generateQrTexture(const char* text, int scale) {
    GLuint qrTexture = 0;
    QrCode qr = QrCode::encodeText(text, QrCode::Ecc::MEDIUM);
    int size = qr.getSize();
    int imgSize = size * scale;

    // Create RGBA image (White background, Black QR)
    std::vector<uint8_t> image(imgSize * imgSize * 4, 255); // White background

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            bool black = qr.getModule(x, y);
            for (int dy = 0; dy < scale; dy++) {
                for (int dx = 0; dx < scale; dx++) {
                    int px = (y * scale + dy) * imgSize + (x * scale + dx);
                    int index = px * 4;
                    if (black) {
                        image[index] = image[index + 1] = image[index + 2] = 0; // Black
                    }
                    image[index + 3] = 255; // Alpha
                }
            }
        }
    }

    // Upload to OpenGL texture
    if (qrTexture) {
        glDeleteTextures(1, &qrTexture);
    }
    glGenTextures(1, &qrTexture);
    glBindTexture(GL_TEXTURE_2D, qrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgSize, imgSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    return qrTexture;
}