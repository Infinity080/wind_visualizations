#include "Texture.h"

#include <fstream> 
#include <iostream>
#include <iterator>
#include <vector>
#include "SOIL/SOIL.h"

typedef unsigned char byte;

GLuint Core::LoadTexture( const char * filepath )
{
	std::string fp_str(filepath);
	std::string format = fp_str.substr(fp_str.find_last_of(".") + 1);

	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	int w, h;
	unsigned char* image = SOIL_load_image(filepath, &w, &h, 0, SOIL_LOAD_RGBA);
	

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);

	return id;
}

GLuint Core::LoadTiledTexture(const char* tileFilePattern, int tileCountX, int tileCountY, int tileWidth, int tileHeight)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int totalWidth = tileCountX * tileWidth;
    int totalHeight = tileCountY * tileHeight;

    // Rezerwujemy pust¹ teksturê o du¿ym rozmiarze
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, totalWidth, totalHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Iterujemy po kafelkach
    for (int y = 0; y < tileCountY; ++y)
    {
        for (int x = 0; x < tileCountX; ++x)
        {
            char filepath[256];
            sprintf(filepath, tileFilePattern, x, y); // "textures/tiles/tile_%d_%d.png"

            int w, h;
            unsigned char* image = SOIL_load_image(filepath, &w, &h, 0, SOIL_LOAD_RGBA);

            if (image == nullptr)
            {
                std::cerr << "Blad ladowania kafelka: " << filepath << std::endl;
                continue;
            }

            // Wstawiamy kafelek na odpowiednie miejsce w du¿ej teksturze
            glTexSubImage2D(GL_TEXTURE_2D, 0, x * tileWidth, y * tileHeight, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);

            SOIL_free_image_data(image);
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    return textureID;
}


void Core::SetActiveTexture(GLuint textureID, const char * shaderVariableName, GLuint programID, int textureUnit)
{
	glUniform1i(glGetUniformLocation(programID, shaderVariableName), textureUnit);
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureID);
}
