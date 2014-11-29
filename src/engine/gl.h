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

    class Program
    {
        GLuint object;
    public:
        Program() : object() {}
        Program(const std::string & vertShader, const std::string & fragShader);
        Program(Program && r) : Program() { *this = std::move(r); }
        Program(const Program & r) = delete;
        ~Program();

        Program & operator = (Program && r) { std::swap(object, r.object); return *this; }
        Program & operator = (const Program & r) = delete;

        GLuint GetObject() const { return object; }

        // TODO: Deprecate all of this, so that programs are actually constant
        void Uniform(GLint location, const float4x4 & mat) const { glUniformMatrix4fv(location, 1, GL_FALSE, &mat.x.x); }
        void Uniform(GLint location, const float2 & vec) const { glUniform2fv(location, 1, &vec.x); }
        void Uniform(GLint location, const float3 & vec) const { glUniform3fv(location, 1, &vec.x); }
        void Uniform(GLint location, const float4 & vec) const { glUniform4fv(location, 1, &vec.x); }

        template<class T> void Uniform(const char * name, const T & value) const { Uniform(glGetUniformLocation(object, name), value); }
        template<class T> void Uniform(const std::string & name, const T & value) const { Uniform(name.c_str(), value); }
    };
}

#endif