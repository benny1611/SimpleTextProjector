#include "SimpleTextProjectorUI.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl2.h"
#include <cstdlib>
#include <string>

SimpleTextProjectorUI::SimpleTextProjectorUI(GLFWwindow* uiWindow, ImFont* titleFont, ImFont* paragraphFont, std::string url) : uiWindow(uiWindow), titleFont(titleFont), paragraphFont(paragraphFont), url(url) {
    glfwSetWindowAttrib(uiWindow, GLFW_RESIZABLE, false);
}

void SimpleTextProjectorUI::draw() {
    if (glfwGetWindowAttrib(uiWindow, GLFW_ICONIFIED) != 0)
    {
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