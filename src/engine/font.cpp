#include "font.h"
#include "utf8.h"

#include <vector>
#include <algorithm>

#include "nanovg.h"

Font::Font(NVGcontext * vg, const char * filename, int pixelHeight, bool dropShadow, uint32_t maxCodepoint) : vg(vg), name(filename), pixelHeight(pixelHeight), dropShadow(dropShadow)
{
	int font = nvgCreateFont(vg, filename, filename);
	if (font == -1) throw std::runtime_error("Could not add font " + name);
}

int Font::GetLineHeight() const
{
    float a, d, h;
    nvgFontSize(vg, pixelHeight);
	nvgFontFace(vg, name.c_str());
    nvgTextMetrics(vg, &a, &d, &h);
    return static_cast<int>(h);
}

int Font::GetBaselineOffset() const
{
    float a, d, h;
    nvgFontSize(vg, pixelHeight);
	nvgFontFace(vg, name.c_str());
    nvgTextMetrics(vg, &a, &d, &h);
    return static_cast<int>(a);
}

int Font::GetStringWidth(const std::string & string) const
{
    nvgFontSize(vg, pixelHeight);
	nvgFontFace(vg, name.c_str());
    return static_cast<int>(nvgTextBounds(vg, 0,0, string.c_str(), nullptr, nullptr));
}

size_t Font::GetUnitIndex(const std::string & string, int x) const
{
    NVGglyphPosition positions[1024];
    nvgFontSize(vg, pixelHeight);
	nvgFontFace(vg, name.c_str());
    int n = nvgTextGlyphPositions(vg, 0, 0, string.c_str(), nullptr, positions, 1024);
    for(int i=0; i<n; ++i) if(x < positions[i].maxx) return static_cast<size_t>(i);
    return n;
}

void Font::DrawString(int x, int y, NVGcolor color, const std::string & string) const
{
    nvgFontSize(vg, pixelHeight);
	nvgFontFace(vg, name.c_str());
	nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    if(dropShadow)
    {
        
	    nvgFillColor(vg, nvgRGBA(0,0,0,color.a));
	    nvgText(vg, x+1, y+1, string.c_str(), nullptr);
    }
	nvgFillColor(vg, color);
	nvgText(vg, x, y, string.c_str(), nullptr);
}