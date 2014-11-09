#include "gl.h"
#include <stb_image.h>
#pragma comment(lib, "glfw3dll.lib")

template<class T> T & BltSwap(T * lhs, T & rhs)
{
    char temp[sizeof(T)];
    memcpy(temp, lhs, sizeof(T));
    memcpy(lhs, &rhs, sizeof(T));
    memcpy(&rhs, temp, sizeof(T));
    return *lhs;
}

using namespace gl;

Mesh::Mesh() : vertexArray(), arrayBuffer(), elementBuffer(), vertexCount(), indexCount(), mode(GL_TRIANGLES), indexType() {}
Mesh::Mesh(Mesh && r) : Mesh() { BltSwap(this,r); }
Mesh & Mesh::operator = (Mesh && r) { return BltSwap(this,r); }
Mesh::~Mesh()
{
    if(elementBuffer) glDeleteBuffers(1,&elementBuffer);
    if(arrayBuffer) glDeleteBuffers(1,&arrayBuffer);
    if(vertexArray) glDeleteVertexArrays(1,&vertexArray);
}

void Mesh::Draw() const
{
    glBindVertexArray(vertexArray);
    if(elementBuffer) glDrawElements(mode, indexCount, indexType, nullptr);
    else glDrawArrays(mode, 0, vertexCount);
}

void Mesh::SetVertexData(const void * vertices, size_t vertexSize, size_t vertexCount)
{
    if(!arrayBuffer) glGenBuffers(1,&arrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexSize*vertexCount, vertices, GL_STATIC_DRAW);
    this->vertexCount = vertexCount;
}

void Mesh::SetAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
    if(!vertexArray) glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);
    if(!arrayBuffer) glGenBuffers(1,&arrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    glEnableVertexAttribArray(index);
}

void Mesh::SetIndexData(const void * indices, GLenum type, size_t indexCount, GLenum mode)
{
    if(!vertexArray) glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);
    if(!elementBuffer) glGenBuffers(1,&elementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (type == GL_UNSIGNED_INT ? 4 : type == GL_UNSIGNED_SHORT ? 2 : type == GL_UNSIGNED_BYTE ? 1 : 0)*indexCount, indices, GL_STATIC_DRAW);
    this->indexCount = indexCount;
    this->indexType = type;
    this->mode = mode;
}

void Texture::Load(const char * filename)
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