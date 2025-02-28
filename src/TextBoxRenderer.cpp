#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include "TextBoxRenderer.h"
#include "utf8.h"

TextBoxRenderer::TextBoxRenderer(float screenWidth, float screenHeight, float boxX, float boxY, float width, float height, Logger* logger, FT_Library& freeTypeLibrary): 
    TextBoxRenderer(screenWidth, screenHeight, boxX, boxY, width, height, 72.0f, 5.0f, 5.0f, 1, 1, 1, 1, "fonts/Raleway.ttf", true, logger, freeTypeLibrary) {
}

TextBoxRenderer::TextBoxRenderer(float screenWidth, float screenHeight, float boxX, float boxY, float width, float height, float desiredFontSize, float decreaseStep, float lineSpacing, float colorR, float colorG, float colorB, float colorA, std::string fontPath, bool wordWrap, Logger* logger, FT_Library& freeTypeLibrary) {
    this->consoleLogger = logger;
    this->projectionMatrix = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
    this->_boxX = boxX;
    this->_boxY = boxY;
    this->_width = width;
    this->_height = height;
    this->_desiredFontSize = desiredFontSize;
    this->_decreaseStep = decreaseStep;
    this->_lineSpacing = lineSpacing;
    this->_wordWrap = wordWrap;
    this->_freeTypeLibrary = freeTypeLibrary;
    this->_colorR = colorR;
    this->_colorG = colorG;
    this->_colorB = colorB;
    this->_colorA = colorA;
    this->_fontPath = fontPath;

    loadFontFace(fontPath);

    // compile shaders
    unsigned int vertex;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex);
    this->checkCompileErrors(vertex, ShaderType::vertex);

    unsigned int fragment;
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment);
    this->checkCompileErrors(fragment, ShaderType::fragment);

    // shader program
    shaderID = glCreateProgram();
    glAttachShader(shaderID, vertex);
    glAttachShader(shaderID, fragment);
    glLinkProgram(shaderID);
    this->checkCompileErrors(shaderID, ShaderType::program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);

}

void TextBoxRenderer::checkCompileErrors(unsigned int shader, ShaderType type) {
    int success;
    char infoLog[1024];
    switch (type) {
    case ShaderType::vertex:
    case ShaderType::fragment:
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::string errorMessage;
            if (type == ShaderType::vertex) {
                errorMessage = "Error compiling vertex shader ";
            } else {
                errorMessage = "Error compiling fragment shader ";
            }
            consoleLogger->error(errorMessage + std::to_string(shader) + ": " + infoLog);
            exit(1);
        }
        break;
    case ShaderType::program:
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            consoleLogger->error("Error linking program " + std::to_string(shader) + ": " + infoLog);
            exit(1);
        }
        break;
    }
}


void TextBoxRenderer::renderCenteredText(bool debug) {
    if (debug) {
        drawDebugLines(_boxX, _boxY, _width, _height);
    }

    std::string modifiedText;
    Lines lines;
    bool isTextFittingInTheBox;

    if (cachedInput == _text) {
        // cache hit, no need to measure text & all
        modifiedText = cachedModifiedText;
        isTextFittingInTheBox = cachedIsTextFittingInBox;
        lines = cachedLines;
    } else {
        // cache miss, now measure the text and update cache
        modifiedText = _text;
        isTextFittingInTheBox = adjustTextForBox(modifiedText, lines);
        cachedInput = _text;
        cachedModifiedText = modifiedText;
        cachedIsTextFittingInBox = isTextFittingInTheBox;
        cachedLines = lines;
    }

    if (!isTextFittingInTheBox) {
        // text doesn't fit, don't draw anything
        return;
    }

    int numberOfCharacters = utf8::distance(modifiedText.begin(), modifiedText.end());

    glUseProgram(shaderID);
    int colorLocation = glGetUniformLocation(shaderID, "textColor");
    glUniform4f(colorLocation, _colorR, _colorG, _colorB, _colorA);

    unsigned int projectionLocation = glGetUniformLocation(shaderID, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    
    std::string::iterator it = modifiedText.begin();
    int currentLineNumber = 0;
    float x = _boxX + (_width / 2.0f) - (lines.lineWidths[currentLineNumber] / 2.0f);

    float y = _boxY + (_height / 2.0f) + (lines.totalTextHeight / 2.0f) - lines.lineAscends[currentLineNumber];


    for (int i = 0; i < numberOfCharacters; i++) {
        int charCode = utf8::next(it, modifiedText.end());

        if (charCode == 10) {
            currentLineNumber++;
            if (currentLineNumber < lines.numberOfLines) {
                x = _boxX + (_width / 2.0f) - (lines.lineWidths[currentLineNumber] / 2.0f);
                int previousLineDescent = lines.lineHeights[currentLineNumber - 1] - lines.lineAscends[currentLineNumber - 1];
                y = y - previousLineDescent - lines.lineAscends[currentLineNumber] - _lineSpacing;
            }
            continue;
        }

        std::map<int, TextBoxRenderer::Character>::iterator characterIt = characterCache.find(charCode);

        if (characterIt == characterCache.end()) {
            // cache miss --> generate texture
            generateAndAddCharacter(charCode);
            characterIt = characterCache.find(charCode);
        }

        // drawing text
        Character ch = (*characterIt).second;
        
        float xPos = x + ch.Bearing.x;
        float yPos = y - (ch.Size.y - ch.Bearing.y);

        float characterWidth = ch.Size.x;
        float characterHeight = ch.Size.y;

        float vertices[6][4] = {
            { xPos,                  yPos + characterHeight,   0.0f, 0.0f },
            { xPos,                  yPos,                     0.0f, 1.0f },
            { xPos + characterWidth, yPos,                     1.0f, 1.0f },

            { xPos,                  yPos + characterHeight,   0.0f, 0.0f },
            { xPos + characterWidth, yPos,                     1.0f, 1.0f },
            { xPos + characterWidth, yPos + characterHeight,   1.0f, 0.0f }
        };

        unsigned int positionAttributeLocation = glGetAttribLocation(shaderID, "position");
        glVertexAttribPointer(positionAttributeLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(positionAttributeLocation);

        unsigned int textTextureLocation = glGetUniformLocation(shaderID, "text");
        glUniform1i(textTextureLocation, 0);

        unsigned int texCoordinateAttributeLocation = glGetAttribLocation(shaderID, "texCoord");
        glVertexAttribPointer(texCoordinateAttributeLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray(texCoordinateAttributeLocation);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

}

void TextBoxRenderer::generateAndAddCharacter(int charCode) {
    int freeTypeError;
    unsigned int glyphIndex = FT_Get_Char_Index(fontFace, charCode);
    freeTypeError = FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_DEFAULT);
    if (freeTypeError) {
        consoleLogger->error("Error loading glyph");
    }
    freeTypeError = FT_Render_Glyph(fontFace->glyph, FT_RENDER_MODE_NORMAL);
    if (freeTypeError) {
        consoleLogger->error("Error rendering glyph");
    }

    // tell OpenGL to use the spacing you give it, not the normal 32-bit boundaries it expects.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int texture;
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        fontFace->glyph->bitmap.width,
        fontFace->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        fontFace->glyph->bitmap.buffer
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Character character = {
        texture,
        glm::ivec2(fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows),
        glm::ivec2(fontFace->glyph->bitmap_left, fontFace->glyph->bitmap_top),
        fontFace->glyph->advance.x,
        fontFace->glyph->metrics.height
    };

    characterCache.insert(std::pair<int, Character>(charCode, character));
}

void TextBoxRenderer::addNewLineToString(std::string& str, int pos, bool wordWrap) {
    char newLine = '\n';

    char* strAsCString = (char*)str.c_str();
    utf8::unchecked::iterator<char*> it(strAsCString);

    auto insertIterator = str.begin();
    int character = -1;
    for (int i = 0; i < pos && insertIterator != str.end(); i++) {
        character = utf8::unchecked::next(insertIterator);
    }

    bool foundASpaceCharacter = false;
    if (wordWrap) {
        if (character != 32) {
            
            for (int i = pos; i >= 0 && insertIterator != str.begin(); i--) {
                character = utf8::unchecked::prior(insertIterator);
                if (character == 32) { // ASCII code for space
                    // erase the space
                    auto insertIteratorOneNext = insertIterator;
                    utf8::unchecked::next(insertIteratorOneNext); 
                    str.erase(insertIterator, insertIteratorOneNext);

                    foundASpaceCharacter = true;
                    break;
                }
            }
            if (!foundASpaceCharacter) {
                for (int i = 0; i < pos && insertIterator != str.end(); i++) {
                    utf8::unchecked::next(insertIterator);
                }
            }
        }
    }

    std::string newLineString;
    utf8::append(newLine, std::back_inserter(newLineString));
    str.insert(insertIterator - str.begin(), newLineString);
}

bool TextBoxRenderer::adjustTextForBox(std::string& input, Lines& lines) {
    // measure text
    int numberOfCharacters = utf8::distance(input.begin(), input.end());

    std::string modifiedText = input;

    int _numberOfLines = 0;

    bool textFitsInBox = false;

    while (!textFitsInBox) {
        bool textWidthBiggerThanBoxWidth = false;

        numberOfCharacters = utf8::distance(modifiedText.begin(), modifiedText.end());
        std::string::iterator iterator = modifiedText.begin();
        _numberOfLines = 0;
        int textWidth = 0;
        int textHeight = 0;

        int maxAscend = 0;
        int maxDescend = 0;

        for (int i = 0; i < numberOfCharacters; i++) {
            int charCode = utf8::next(iterator, modifiedText.end());

            if (charCode != 10) {
                std::map<int, TextBoxRenderer::Character>::iterator characterIt = characterCache.find(charCode);

                if (characterIt == characterCache.end()) {
                    // cache miss --> generate texture
                    generateAndAddCharacter(charCode);
                    characterIt = characterCache.find(charCode);
                }

                Character ch = (*characterIt).second;

                textWidth += (ch.Advance >> 6);
                if (textWidth > _width) {
                    textWidthBiggerThanBoxWidth = true;
                    addNewLineToString(modifiedText, i, _wordWrap);
                    break;
                }

                int ascend = ch.Bearing.y;
                int descend = (ch.height >> 6) - ascend;

                maxAscend = std::max(maxAscend, ascend);
                maxDescend = std::max(maxDescend, descend);

            } else {
                if (_numberOfLines < 256) {
                    lines.lineWidths[_numberOfLines] = textWidth;
                    if (textWidth > _width) {
                        textWidthBiggerThanBoxWidth = true;
                        addNewLineToString(modifiedText, i, _wordWrap);
                        break;
                    }
                    textHeight = maxAscend + maxDescend;
                    lines.lineHeights[_numberOfLines] = textHeight;
                    lines.lineAscends[_numberOfLines] = maxAscend;
                    _numberOfLines++;
                    textWidth = 0;
                    maxAscend = 0;
                    maxDescend = 0;
                }
            }
        }

        if (textWidthBiggerThanBoxWidth) {
            continue;
        }

        char lastChar = modifiedText.back();
        if (lastChar != '\n' && lastChar != '\r') {
            lines.lineWidths[_numberOfLines] = textWidth;
            textHeight = maxAscend + maxDescend;
            lines.lineHeights[_numberOfLines] = textHeight;
            lines.lineAscends[_numberOfLines] = maxAscend;
            _numberOfLines++;
        }

        int _totalTextHeight = 0;

        for (int i = 0; i < _numberOfLines; i++) {
            _totalTextHeight += lines.lineHeights[i];
        }

        _totalTextHeight += (_numberOfLines - 1) * _lineSpacing;

        if (_totalTextHeight > _height) {
            if (_desiredFontSize > _decreaseStep) {
                _desiredFontSize -= _decreaseStep;
                FT_Set_Pixel_Sizes(fontFace, 0, _desiredFontSize);
                characterCache.clear();
                modifiedText = input;
                continue;
            } else {
                return false;
            }
        }

        lines.totalTextHeight = _totalTextHeight;
        textFitsInBox = true;
    }

    lines.numberOfLines = _numberOfLines;
    input = modifiedText;

    return textFitsInBox;
}

void TextBoxRenderer::drawDebugLines(float boxX, float boxY, float width, float height) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glLineWidth(3.0f);

    glBegin(GL_LINES);
    glVertex2f(boxX, boxY + (height / 2));
    glVertex2f(boxX + width, boxY + (height / 2));
    glEnd();

    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(boxX + (width / 2), boxY);
    glVertex2f(boxX + (width / 2), boxY + height);
    glEnd();


    glBegin(GL_LINES);
    glVertex2f(boxX, boxY);
    glVertex2d(boxX + width, boxY); 
    glEnd();

    glBegin(GL_LINES);
    glVertex2d(boxX + width, boxY);
    glVertex2f(boxX + width, boxY + height);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(boxX + width, boxY + height);
    glVertex2f(boxX, boxY + height);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(boxX, boxY + height);
    glVertex2f(boxX, boxY);
    glEnd();

    glFlush();

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
}


void TextBoxRenderer::loadFontFace(std::string fontPath) {
    size_t fontFileSize;
    const unsigned char* fontFileBuffer = loadFile(fontPath, fontFileSize);
    if (fontFileBuffer == nullptr) {
        consoleLogger->error("Could not load font file " + fontPath + " into memory");
        exit(1);
    }

    FT_Face fontFace;
    int freeTypeError = FT_New_Memory_Face(_freeTypeLibrary, fontFileBuffer, fontFileSize, 0, &fontFace);

    if (freeTypeError) {
        consoleLogger->error("Error loading the font from memory. Error code: " + freeTypeError);
        exit(1);
    }

    FT_Set_Pixel_Sizes(fontFace, 0, _desiredFontSize);

    FT_Done_Face(this->fontFace);

    this->fontFace = fontFace;
}


unsigned char* TextBoxRenderer::loadFile(const std::string& filename, size_t& fileSize) {
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

void TextBoxRenderer::setText(std::string text) {
    this->_text = text;
}

void TextBoxRenderer::setColor(float colorR, float colorG, float colorB, float colorA) {
    this->_colorR = colorR;
    this->_colorG = colorG;
    this->_colorB = colorB;
    this->_colorA = colorA;
}

void TextBoxRenderer::setBoxPosition(float boxX, float boxY) {
    this->_boxX = boxX;
    this->_boxY = boxY;
}

void TextBoxRenderer::setBoxSize(float width, float height) {
    this->_width = width;
    this->_height = height;
}

void TextBoxRenderer::setFontSize(float desiredFontSize, float decreaseStep) {
    this->_desiredFontSize = desiredFontSize;
    this->_decreaseStep = decreaseStep;
    FT_Set_Pixel_Sizes(fontFace, 0, _desiredFontSize);
    clearCache();
}

void TextBoxRenderer::setLineSpacing(float lineSpacing) {
    this->_lineSpacing = lineSpacing;
}

void TextBoxRenderer::setWordWrap(bool wordWrap) {
    this->_wordWrap = wordWrap;
}

void TextBoxRenderer::setFont(std::string fontPath) {
    this->_fontPath = fontPath;
    loadFontFace(fontPath);
    clearCache();
}

void TextBoxRenderer::setScreenSize(float screenWidth, float screenHeight) {
    this->projectionMatrix = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
    clearCache();
}

std::string TextBoxRenderer::getText() {
    return this->_text;
}

std::string TextBoxRenderer::getFontPath() {
    return this->_fontPath;
}

void TextBoxRenderer::clearCache() {
    this->characterCache.clear();
    this->cachedInput.clear();
}