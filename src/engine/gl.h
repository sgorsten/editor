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
        GLuint vertexArray,arrayBuffer,elementBuffer;
        GLsizei vertexCount,indexCount;
        GLenum mode,indexType;
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

    GLuint CompileShader(GLenum type, const char * source);
    GLuint LinkProgram(GLuint vertShader, GLuint fragShader);
}

#endif