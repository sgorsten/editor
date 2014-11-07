#ifndef EDITOR_GL_H
#define EDITOR_GL_H

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



#endif