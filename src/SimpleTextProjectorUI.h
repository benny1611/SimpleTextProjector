#pragma once
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include <string>

class SimpleTextProjectorUI {
public:
	SimpleTextProjectorUI(GLFWwindow* uiWindow, ImFont* titleFont, ImFont* paragraphFont, ImFont* buttonFont, std::string url, bool* shouldCloseUI, bool* checkBoxTicked);
	void draw();
private:
	GLuint generateQrTexture(const char* text, int scale = 10);
	GLuint qrTexture;
	std::string url;
	GLFWwindow* uiWindow;
	ImFont* titleFont;
	ImFont* paragraphFont;
	ImFont* buttonFont;
	bool* shouldCloseUI;
	bool* checkBoxTicked;
};