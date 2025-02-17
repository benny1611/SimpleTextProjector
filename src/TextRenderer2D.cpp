#include "TextRenderer2D.h"
#include "utf8.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

TextRenderer2D::TextRenderer2D(float screenWidth, float screenHeight, FT_Face& face, Logger* logger) {
    this->consoleLogger = logger;
    this->fontFace = face;
    this->projectionMatrix = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);

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

void TextRenderer2D::checkCompileErrors(unsigned int shader, ShaderType type) {
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


void TextRenderer2D::renderCenteredText(std::string* text, float boxX, float boxY, float width, float height, float desiredFontSize, float decreaseStep, bool debug) {

    if (debug) {
        drawDebugLines(boxX, boxY, width, height);
    }

    std::string modifiedText = *text;
    Lines lines;

    bool isTextFittingInTheBox = adjustTextForBox(modifiedText, boxX, boxY, width, height, desiredFontSize, decreaseStep, lines);

    if (!isTextFittingInTheBox) {
        return;
    }

    int numberOfCharacters = utf8::distance(modifiedText.begin(), modifiedText.end());

    glUseProgram(shaderID);
    int colorLocation = glGetUniformLocation(shaderID, "textColor");
    glUniform4f(colorLocation, 1.0f, 1.0f, 1.0f, 1.0f);

    unsigned int projectionLocation = glGetUniformLocation(shaderID, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    
    std::string::iterator it = modifiedText.begin();
    int currentLineNumber = 0;
    float x = boxX + (width / 2.0f) - (lines.lineWidths[currentLineNumber] / 2.0f);

    float y = boxY + (height / 2.0f) + (lines.totalTextHeight / 2.0f) - lines.lineAscends[currentLineNumber];


    for (int i = 0; i < numberOfCharacters; i++) {
        int charCode = utf8::next(it, modifiedText.end());

        if (charCode == 10) {
            currentLineNumber++;
            if (currentLineNumber < lines.numberOfLines) {
                x = boxX + (width / 2.0f) - (lines.lineWidths[currentLineNumber] / 2.0f);
                int previousLineDescent = lines.lineHeights[currentLineNumber - 1] - lines.lineAscends[currentLineNumber - 1];
                y = y - previousLineDescent - lines.lineAscends[currentLineNumber];
            }
            continue;
        }

        std::map<int, TextRenderer2D::Character>::iterator characterIt = characterCache.find(charCode);

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

void TextRenderer2D::generateAndAddCharacter(int charCode) {
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

void TextRenderer2D::addNewLineToString(std::string& str, int pos, bool breakAtSpace) {
    char newLine = '\n';

    char* strAsCString = (char*)str.c_str();
    utf8::unchecked::iterator<char*> it(strAsCString);

    auto insertIterator = str.begin();
    int character = -1;
    for (int i = 0; i < pos && insertIterator != str.end(); i++) {
        character = utf8::unchecked::next(insertIterator);
    }

    bool foundASpaceCharacter = false;
    if (breakAtSpace) {
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

bool TextRenderer2D::adjustTextForBox(std::string& input, float boxX, float boxY, float width, float height, float desiredFontSize, float decreaseStep, Lines& lines) {
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
                std::map<int, TextRenderer2D::Character>::iterator characterIt = characterCache.find(charCode);

                if (characterIt == characterCache.end()) {
                    // cache miss --> generate texture
                    generateAndAddCharacter(charCode);
                    characterIt = characterCache.find(charCode);
                }

                Character ch = (*characterIt).second;

                textWidth += (ch.Advance >> 6);
                if (textWidth > width) {
                    textWidthBiggerThanBoxWidth = true;
                    addNewLineToString(modifiedText, i);
                    break;
                }

                int ascend = ch.Bearing.y;
                int descend = (ch.height >> 6) - ascend;

                maxAscend = std::max(maxAscend, ascend);
                maxDescend = std::max(maxDescend, descend);

            } else {
                if (_numberOfLines < 256) {
                    lines.lineWidths[_numberOfLines] = textWidth;
                    if (textWidth > width) {
                        textWidthBiggerThanBoxWidth = true;
                        addNewLineToString(modifiedText, i);
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

        if (_totalTextHeight > height) {
            if (desiredFontSize > decreaseStep) {
                desiredFontSize -= decreaseStep;
                FT_Set_Pixel_Sizes(fontFace, 0, desiredFontSize);
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

void TextRenderer2D::drawDebugLines(float boxX, float boxY, float width, float height) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glLineWidth(3.0f);

    glBegin(GL_LINES);
    glVertex2f(boxX, (boxY + height) / 2);
    glVertex2f(boxX + width, (boxY + height) / 2);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f((boxX + width) / 2, boxY);
    glVertex2f((boxX + width) / 2, boxY + height);
    glEnd();


    glBegin(GL_LINES);
    glVertex2f(boxX, boxY);
    glVertex2d(boxX + width, boxY); 
    glVertex2f(boxX + width, boxY + height);
    glVertex2f(boxX, boxY + height);
    glVertex2f(boxX, boxY + height);
    glVertex2f(boxX, boxY);
    glEnd();

    glFlush();

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
}