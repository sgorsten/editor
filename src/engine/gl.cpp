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

GLuint gl::CompileShader(GLenum type, const char * source)
{
    const char * sources[] = {source};

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, sources, nullptr);
    glCompileShader(shader);

    GLint status, length;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> buffer(length);
        glGetShaderInfoLog(shader, length, nullptr, buffer.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("glCompileShader(...) failed with log:\n") + buffer.data());
    }

    return shader;
}

GLuint gl::LinkProgram(GLuint vertShader, GLuint fragShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint status, length;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> buffer(length);
        glGetProgramInfoLog(program, length, nullptr, buffer.data());
        glDeleteProgram(program);
        throw std::runtime_error(std::string("glLinkProgram(...) failed with log:\n") + buffer.data());
    }

    return program;
}