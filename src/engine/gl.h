#ifndef ENGINE_GL_H
#define ENGINE_GL_H

#include "linalg.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace gl
{
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
}

inline void glVertex(const float2 & vertex) { glVertex2fv(&vertex.x); }
inline void glVertex(const float3 & vertex) { glVertex3fv(&vertex.x); }
inline void glVertex(const float4 & vertex) { glVertex4fv(&vertex.x); }
inline void glNormal(const float3 & normal) { glNormal3fv(&normal.x); }
inline void glColor(const float3 & color) { glColor3fv(&color.x); }
inline void glColor(const float4 & color) { glColor4fv(&color.x); }
inline void glLoad(const float4x4 & matrix) { glLoadMatrixf(&matrix.x.x); }
inline void glMult(const float4x4 & matrix) { glMultMatrixf(&matrix.x.x); }

#endif