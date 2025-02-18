#include "TextBoxFactory.h"
#include <fstream>

TextBoxFactory::TextBoxFactory(float screenWidth, float screenHeight, Logger* consoleLogger) {
	this->_screenWidth = screenWidth;
	this->_screenHeight = screenHeight;

	int freeTypeError = FT_Init_FreeType(&_freeTypeLibrary);
	if (freeTypeError) {
		consoleLogger->error("FreeType init failed with error code: " + freeTypeError);
		exit(1);
	}
}

void TextBoxFactory::setScreenWidth(float screenWidth) {
	this->_screenWidth = screenWidth;
}

void TextBoxFactory::setScreenHeight(float screenHeight) {
	this->_screenHeight = screenHeight;
}

TextBoxRenderer* TextBoxFactory::createTextBox(std::string fontPath, float desiredFontSize, float boxX, float boxY, float width, float height, float fontSizeDecreaseStep, bool wordWrap) {
    size_t fontFileSize;
    const unsigned char* fontFileBuffer = loadFile(fontPath, fontFileSize);
    if (fontFileBuffer == nullptr) {
        _consoleLogger->error("Could not load font file " + fontPath + " into memory");
        exit(1);
    }

    FT_Face fontFace;
    int freeTypeError = FT_New_Memory_Face(_freeTypeLibrary, fontFileBuffer, fontFileSize, 0, &fontFace);

    if (freeTypeError) {
        _consoleLogger->error("Error loading the font from memory. Error code: " + freeTypeError);
        exit(1);
    }

    FT_Set_Pixel_Sizes(fontFace, 0, desiredFontSize);

    return new TextBoxRenderer(_screenWidth, _screenHeight, fontFace, boxX, boxY, width, height, desiredFontSize, fontSizeDecreaseStep, _consoleLogger);
}

// Function to load the contents of a file into memory as unsigned char*
unsigned char* TextBoxFactory::loadFile(const std::string& filename, size_t& fileSize) {
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