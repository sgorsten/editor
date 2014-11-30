#include "gl.h"
#include <map>
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

static void AttachShader(GLuint program, GLenum type, const char * source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
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

    glAttachShader(program, shader);
    glDeleteShader(shader);
}

gl::Program::Program(const std::string & vertShader, const std::string & fragShader) : Program()
{
    object = glCreateProgram();
    AttachShader(object, GL_VERTEX_SHADER, vertShader.c_str());
    AttachShader(object, GL_FRAGMENT_SHADER, fragShader.c_str());
    glLinkProgram(object);

    GLint status, length;
    glGetProgramiv(object, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> buffer(length);
        glGetProgramInfoLog(object, length, nullptr, buffer.data());
        throw std::runtime_error(std::string("glLinkProgram(...) failed with log:\n") + buffer.data());
    }

    // Enumerate active program uniforms
    std::map<GLint, BlockDesc> blocks;
    GLint activeUniforms, activeUniformMaxLength;
    glGetProgramiv(object, GL_ACTIVE_UNIFORMS, &activeUniforms);
    glGetProgramiv(object, GL_ACTIVE_UNIFORM_MAX_LENGTH, &activeUniformMaxLength);
    std::vector<GLchar> nameBuffer(activeUniformMaxLength);
    for(GLuint index = 0; index < activeUniforms; ++index)
    {
        GLint blockIndex;
        UniformDesc uniform;
        glGetActiveUniform(object, index, nameBuffer.size(), nullptr, &uniform.size, &uniform.type, nameBuffer.data());
        glGetActiveUniformsiv(object, 1, &index, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
        glGetActiveUniformsiv(object, 1, &index, GL_UNIFORM_OFFSET, &uniform.offset);
        glGetActiveUniformsiv(object, 1, &index, GL_UNIFORM_ARRAY_STRIDE, &uniform.arrayStride);
        glGetActiveUniformsiv(object, 1, &index, GL_UNIFORM_MATRIX_STRIDE, &uniform.matrixStride);
        if(blockIndex == -1) uniform.offset = index;
        uniform.name = nameBuffer.data();
        blocks[blockIndex].uniforms.push_back(uniform);
    }

    // Enumerate active program uniform blocks
    for(auto & pair : blocks)
    {
        GLint nameLength;
        pair.second.index = pair.first;
        if(pair.first == -1) pair.second.dataSize = 0;
        else
        {
            glGetActiveUniformBlockiv(object, pair.first, GL_UNIFORM_BLOCK_DATA_SIZE, &pair.second.dataSize);
            glGetActiveUniformBlockiv(object, pair.first, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLength);
            nameBuffer.resize(nameLength);
            glGetActiveUniformBlockName(object, pair.first, nameBuffer.size(), nullptr, nameBuffer.data());
            pair.second.name = nameBuffer.data();
        }
        this->blocks.push_back(std::move(pair.second));
    }
}

gl::Program::~Program()
{
    if(object) glDeleteProgram(object);
}