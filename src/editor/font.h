#ifndef EDITOR_FONT_H
#define EDITOR_FONT_H

#include "gl.h"

#include <cstdint>
#include <string>
#include <map>

class Font
{
    struct Glyph
    {
        float s0,t0,s1,t1;
        int x0,y0,x1,y1,advance;
        std::map<uint32_t,int> kerning;
    };
    std::map<uint32_t,Glyph> glyphs;
    int height, baseline, texWidth, texHeight;
    GLuint texture;
public:
    Font(const char * filename, int pixelHeight, bool dropShadow = false, uint32_t maxCodepoint = 127);

    int GetLineHeight() const { return height; }        // Number of pixels from top to bottom of a line
    int GetBaselineOffset() const { return baseline; }  // Number of pixels from top of a line to the "baseline" of the font
    int GetStringWidth(const std::string & s) const;
    size_t GetUnitIndex(const std::string & s, int x) const;

    void DrawString(int x, int y, float r, float g, float b, const std::string & s) const;
    void DrawFontTexture(int x, int y) const;
};

#endif