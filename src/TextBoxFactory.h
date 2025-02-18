#pragma once

#include "TextBoxRenderer.h"

class TextBoxFactory {
public:
	TextBoxFactory(float screenWidth, float screenHeight, Logger* consoleLogger);

	void setScreenWidth(float screenWidth);
	void setScreenHeight(float screenHeight);
	TextBoxRenderer* createTextBox(std::string fontPath, float desiredFontSize, float boxX, float boxY, float width, float height, float fontSizeDecreaseStep, bool wordWrap);
private:
	float _screenWidth;
	float _screenHeight;
	FT_Library _freeTypeLibrary;
	Logger* _consoleLogger;

	unsigned char* loadFile(const std::string& filename, size_t& fileSize);
};