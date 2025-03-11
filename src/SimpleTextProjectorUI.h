#pragma once
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include <string>

class SimpleTextProjectorUI {
public:
	SimpleTextProjectorUI(GLFWwindow* uiWindow, ImFont* titleFont, ImFont* paragraphFont, std::string url);
	void draw();
private:
	std::string url;
	GLFWwindow* uiWindow;
	ImFont* titleFont;
	ImFont* paragraphFont;
};