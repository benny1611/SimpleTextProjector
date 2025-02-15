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


void TextRenderer2D::renderCenteredText(std::string* text, float boxX, float boxY, float width, float height) {

    // measure text

    int numberOfCharacters = utf8::distance(text->begin(), text->end());
    const int maxSupportedLines = 256;
    std::string::iterator iterator = text->begin();
    float textLength = 0;
    int lineNumber = 0;
    int lineWidths[maxSupportedLines];

    for (int i = 0; i < numberOfCharacters; i++) {
        int charCode = utf8::next(iterator, text->end());

        char debug = (char)charCode;

        if (charCode != 10) {
            std::map<int, TextRenderer2D::Character>::iterator characterIt = characterCache.find(charCode);

            if (characterIt == characterCache.end()) {
                // cache miss --> generate texture
                generateAndAddCharacter(charCode);
                characterIt = characterCache.find(charCode);
            }

            Character ch = (*characterIt).second;

            textLength += (ch.Advance >> 6);
        } else {
            if (lineNumber < maxSupportedLines) {
                lineWidths[lineNumber] = textLength;
                lineNumber++;
                textLength = 0;
            }
        }
    }

    char lastChar = text->back();
    if (lastChar != '\n' && lastChar != '\r') {
        lineWidths[lineNumber] = textLength;
        lineNumber++;
    }

    float lineHeight = fontFace->height >> 6;

    glUseProgram(shaderID);
    int colorLocation = glGetUniformLocation(shaderID, "textColor");
    glUniform4f(colorLocation, 1.0f, 1.0f, 1.0f, 1.0f);

    unsigned int projectionLocation = glGetUniformLocation(shaderID, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    
    std::string::iterator it = text->begin();
    int currentLineNumber = 0;
    float x = boxX + (width / 2.0f) - (lineWidths[currentLineNumber] / 2.0f);

    float y = boxY + (height / 2.0f);

    if (lineNumber == 1) {
        y -= lineHeight;
    } else {
        y += (lineHeight * lineNumber);
    }


    for (int i = 0; i < numberOfCharacters; i++) {
        int charCode = utf8::next(it, text->end());

        if (charCode == 10) {
            currentLineNumber++;
            y -= lineHeight * 4;
            if (currentLineNumber < lineNumber) {
                x = boxX + (width / 2.0f) - (lineWidths[currentLineNumber] / 2.0f);
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

        float width = ch.Size.x;
        float height = ch.Size.y;

        float vertices[6][4] = {
            { xPos,         yPos + height,   0.0f, 0.0f },
            { xPos,         yPos,            0.0f, 1.0f },
            { xPos + width, yPos,            1.0f, 1.0f },

            { xPos,         yPos + height,   0.0f, 0.0f },
            { xPos + width, yPos,            1.0f, 1.0f },
            { xPos + width, yPos + height,   1.0f, 0.0f }
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
        fontFace->glyph->advance.x
    };

    characterCache.insert(std::pair<int, Character>(charCode, character));
}