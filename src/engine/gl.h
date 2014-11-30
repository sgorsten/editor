#ifndef ENGINE_GL_H
#define ENGINE_GL_H

#include "linalg.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

namespace gl
{
    inline GLenum GetType(uint8_t *) { return GL_UNSIGNED_BYTE; }
    inline GLenum GetType(uint16_t *) { return GL_UNSIGNED_SHORT; }
    inline GLenum GetType(uint32_t *) { return GL_UNSIGNED_INT; }
    inline GLenum GetType(float *) { return GL_FLOAT; }

    class Buffer
    {
        GLuint object;
    public:
        Buffer() : object() {}
        Buffer(Buffer && r) : Buffer() { *this = std::move(r); }
        Buffer(const Buffer & r) = delete;
        ~Buffer() { if(object) glDeleteBuffers(1, &object); }

        void BindBase(GLenum target, GLuint index) const { glBindBufferBase(target, index, object); }

        Buffer & operator = (Buffer && r) { std::swap(object, r.object); return *this; }
        Buffer & operator = (const Buffer & r) = delete;

        void SetData(GLenum target, GLsizeiptr size, GLvoid * data, GLenum usage)
        {
            if(!object) glGenBuffers(1, &object);
            glBindBuffer(target, object);
            glBufferData(target, size, data, usage);
        }
    };

    class Mesh
    {
        GLuint vertexArray, arrayBuffer, elementBuffer;
        GLsizei vertexCount, indexCount;
        GLenum mode, indexType;
    public:
        Mesh();
        Mesh(Mesh && r);
        Mesh(const Mesh & r) = delete;
        Mesh & operator = (Mesh && r);
        Mesh & operator = (const Mesh & r) = delete;
        ~Mesh();

        void Draw() const;

        void SetVertexData(const void * vertices, size_t vertexSize, size_t vertexCount);
        void SetAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
        void SetIndexData(const void * indices, GLenum type, size_t indexCount, GLenum mode);

        template<class V> void SetVertices(const std::vector<V> & vertices) { SetVertexData(vertices.data(), sizeof(V), vertices.size()); }
        template<class V, class T, int N> void SetAttribute(GLuint index, vec<T,N> V::*attribute, bool normalized = false) { SetAttribPointer(index, N, GetType((T*)0), normalized ? GL_TRUE : GL_FALSE, sizeof(V), &(reinterpret_cast<V *>(nullptr)->*attribute)); }
        template<class T, int N> void SetElements(const std::vector<vec<T,N>> & elements) { const GLenum modes[] = {0,GL_POINTS,GL_LINES,GL_TRIANGLES,GL_QUADS}; SetIndexData(elements.data(), GetType((T*)0), elements.size()*N, modes[N]); }
    };

    class Texture
    {
        GLuint tex;
        int width, height;
    public:
        Texture() : tex() {}
        ~Texture() { if(tex) glDeleteTextures(1,&tex); }

        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        void Bind() const { glBindTexture(GL_TEXTURE_2D, tex); }

        void Load(const char * filename);
    };

    struct UniformDesc
    {
        std::string name;
        GLint offset;
        GLint size, arrayStride, matrixStride;
        GLenum type;

        void SetValue(uint8_t * data, const float3 & value) const { if(type == GL_FLOAT_VEC3) reinterpret_cast<float3 &>(data[offset]) = value; }
        void SetValue(uint8_t * data, const float4x4 & value) const { if(type == GL_FLOAT_MAT4) reinterpret_cast<float4x4 &>(data[offset]) = value; }
    };

    struct BlockDesc
    {
        std::string name;
        GLint binding, dataSize;
        std::vector<UniformDesc> uniforms;

        const UniformDesc * GetNamedUniform(const std::string & name) const { for(auto & uniform : uniforms) if(uniform.name == name) return &uniform; return nullptr; }
        template<class T> void SetUniform(uint8_t * data, const std::string & name, const T & value) const { if(auto u = GetNamedUniform(name)) u->SetValue(data, value); }
    };

    class Program
    {
        GLuint object;
        std::vector<BlockDesc> blocks;
    public:
        Program() : object() {}
        Program(const std::string & vertShader, const std::string & fragShader);
        Program(Program && r) : Program() { *this = std::move(r); }
        Program(const Program & r) = delete;
        ~Program();

        Program & operator = (Program && r) { std::swap(object, r.object); blocks.swap(r.blocks); return *this; }
        Program & operator = (const Program & r) = delete;

        const std::vector<BlockDesc> & GetBlocks() const { return blocks; }
        const BlockDesc * GetDefaultBlock(const std::string & name) const { for(auto & block : blocks) if(block.binding == -1) return &block; return nullptr; }
        const BlockDesc * GetNamedBlock(const std::string & name) const { for(auto & block : blocks) if(block.name == name) return &block; return nullptr; }
        void Use() const { glUseProgram(object); }
    };
}

#endif