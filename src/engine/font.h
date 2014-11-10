#ifndef ENGINE_FONT_H
#define ENGINE_FONT_H

#include "gl.h"
#include "nanovg.h"

#include <cstdint>
#include <string>
#include <map>

struct NVGcontext;

class Font
{
    NVGcontext * vg;
    std::string name;
    int pixelHeight;
    bool dropShadow;
public:
    Font(NVGcontext * vg, const char * filename, int pixelHeight, bool dropShadow = false, uint32_t maxCodepoint = 127);

    int GetLineHeight() const; // Number of pixels from top to bottom of a line
    int GetBaselineOffset() const; // Number of pixels from top of a line to the "baseline" of the font
    int GetStringWidth(const std::string & s) const;
    size_t GetUnitIndex(const std::string & s, int x) const;

    void DrawString(int x, int y, NVGcolor color, const std::string & s) const;
};

#endif