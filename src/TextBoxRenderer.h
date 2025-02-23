#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#include <glm/glm.hpp>
#include "Poco/Logger.h"

using Poco::Logger;

class TextBoxRenderer {
public:
    TextBoxRenderer(float screenWidth, float screenHeight, float boxX, float boxY, float width, float height, float desiredFontSize, float decreaseStep, float lineSpacing, float colorR, float colorG, float colorB, float colorA, std::string fontPath, bool wordWrap, Logger* logger, FT_Library& freeTypeLibrary);
    
    TextBoxRenderer(float screenWidth, float screenHeight, float boxX, float boxY, float width, float height, Logger* logger, FT_Library& freeTypeLibrary);
    //~TextBoxRenderer();

    void renderCenteredText(bool debug = false);
    void setText(std::string text);
    void setColor(float colorR, float colorG, float colorB, float colorA);
    void setBoxPosition(float boxX, float boxY);
    void setBoxSize(float width, float height);
    void setFontSize(float desiredFontSize, float decreaseStep = 5.0);
    void setLineSpacing(float lineSpacing);
    void setWordWrap(bool wordWrap);
    void setFont(std::string fontPath);
    void setScreenSize(float screenWidth, float screenHeight);
    std::string getText();
    std::string getFontPath();
private:
    float _boxX;
    float _boxY;
    float _width;
    float _height;
    float _desiredFontSize;
    float _decreaseStep;
    float _lineSpacing;
    float _colorR;
    float _colorG; 
    float _colorB;
    float _colorA;
    bool _wordWrap;
    std::string _text;
    std::string _fontPath;

    struct Character {
        unsigned int TextureID; // ID handle of the glyph texture
        glm::ivec2   Size;      // Size of glyph
        glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
        unsigned int Advance;   // Horizontal offset to advance to next glyph
        int height;
    };

    struct Lines {
        int lineWidths[256];
        int lineHeights[256];
        int lineAscends[256];
        int numberOfLines;
        int totalTextHeight;
    };

    Logger* consoleLogger;
    FT_Library _freeTypeLibrary;
    FT_Face fontFace;
    glm::mat4 projectionMatrix;
    unsigned int shaderID;
    unsigned int VBO;

    // cache
    std::map<int, Character> characterCache;
    std::string cachedInput;
    std::string cachedModifiedText;
    Lines cachedLines;
    bool cachedIsTextFittingInBox;

    const char* vertexShaderSource = R"(
    #version 120
    attribute vec2 position; // Screen space position (x, y)
    attribute vec2 texCoord;
    varying vec2 TexCoord;

    uniform mat4 projection;  // Projection matrix for conversion

    void main() {
        vec4 screenPos = vec4(position, 0.0, 1.0);  // Screen space to clip space
        gl_Position = projection * screenPos;       // Apply projection to convert to clip space
        TexCoord = texCoord;
    }
)";

    const char* fragmentShaderSource = R"(
    #version 120
    uniform sampler2D text;  // The texture containing the text glyphs
    uniform vec4 textColor;  // The RGBA color to apply to the text
    varying vec2 TexCoord;   // The texture coordinate

    void main() {
       vec4 sampled = texture2D(text, TexCoord);  // GLSL
       gl_FragColor = vec4(textColor.rgb, sampled.r * textColor.a);
    }
)";
    void generateAndAddCharacter(int charCode);

    enum ShaderType {vertex, fragment, program};

    void checkCompileErrors(unsigned int shader, ShaderType type);

    bool adjustTextForBox(std::string& input, Lines& lines);

    static void addNewLineToString(std::string& str, int position, bool breakAtSpace);

    void drawDebugLines(float boxX, float boxY, float width, float height);

    unsigned char* loadFile(const std::string& filename, size_t& fileSize);

    void loadFontFace(std::string fontPath);

    void clearCache();
};

