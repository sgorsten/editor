#include "gl.h"

#pragma comment(lib, "glfw3dll.lib")

#include <stb_image.h>

void gl::Texture::Load(const char * filename)
{
    int x, y, n;
    if(auto pixels = stbi_load(filename, &x, &y, &n, 0))
    {
        GLenum format[] = {0, GL_LUMINANCE, 0, GL_RGB, GL_RGBA};

        if(!tex) glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, format[n], GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stbi_image_free(pixels);

        width = x;
        height = y;
    }
}