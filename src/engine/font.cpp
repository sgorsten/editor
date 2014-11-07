#include "font.h"
#include "utf8.h"

#include <vector>
#include <algorithm>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

Font::Font(const char * filename, int pixelHeight, bool dropShadow, uint32_t maxCodepoint) : height(), baseline()
{
    // Load file into a memory buffer
    auto file = fopen(filename, "rb");
    if(!file) throw std::runtime_error(std::string("File not found: ") + filename);
    fseek(file, 0, SEEK_END);
    std::vector<uint8_t> buffer(ftell(file));
    fseek(file, 0, SEEK_SET);
    fread(buffer.data(), 1, buffer.size(), file);
    fclose(file);

    // Read the font file
    stbtt_fontinfo font;
    if(!stbtt_InitFont(&font, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0))) throw std::runtime_error(filename + std::string(" is not a valid truetype font!"));
    float scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(pixelHeight));

    // Obtain bitmaps for printable codepoints, and sort them by descending height, then descending width
    struct GlyphBitmap { int codepoint,width,height,xoff,yoff,advance,x,y; uint8_t * pixels; };
    std::vector<GlyphBitmap> bitmaps;
    for(uint32_t codepoint=0; codepoint<=maxCodepoint; ++codepoint)
    {
        if(codepoint < 128 && !isprint(codepoint)) continue;
        GlyphBitmap bitmap = {codepoint};
        bitmap.pixels = stbtt_GetCodepointBitmap(&font, 0, scale, codepoint, &bitmap.width, &bitmap.height, &bitmap.xoff, &bitmap.yoff);
        stbtt_GetCodepointHMetrics(&font, codepoint, &bitmap.advance, 0);
        baseline = std::max(baseline, -bitmap.yoff);
        height = std::max(height, bitmap.yoff + bitmap.height);
        bitmaps.push_back(bitmap);
    }
    std::sort(begin(bitmaps), end(bitmaps), [](const GlyphBitmap & a, const GlyphBitmap & b) { return std::tie(a.height,a.width) > std::tie(b.height,b.width); });
    for(auto & bitmap : bitmaps) bitmap.yoff += baseline; // Define y-offset relative to the top of the font
    height += baseline;

    // Determine placement for each bitmap
    const int margin = dropShadow ? 1 : 0;
    texWidth = 1;
    texHeight = 1;
    while(true)
    {
        int x = 0, y = 0, nextY = 0;
        for(auto & bitmap : bitmaps)
        {
            if(bitmap.width > texWidth)
            {
                nextY = texHeight + 1;
                break;
            }
            if(x + bitmap.width + margin > texWidth)
            {
                x = 0;
                y = nextY;
            }
            bitmap.x = x;
            bitmap.y = y;
            x += bitmap.width + margin;
            nextY = std::max(nextY, y + bitmap.height + margin);
        }
        if(nextY <= texHeight) break;
        if(texWidth == texHeight) texWidth *= 2;
        else texHeight *= 2;
    }
    if(dropShadow)
    {
        ++height;
        ++baseline;
    }

    // Finally, produce the actual image
    struct LumAlpha { uint8_t lum,alpha; };
    std::vector<LumAlpha> image(texWidth * texHeight * 2, {0,0});
    for(auto & bitmap : bitmaps)
    {
        if(dropShadow)
        {
            for(int y=0; y<bitmap.height; ++y)
            {
                for(int x=0; x<bitmap.width; ++x)
                {
                    image[(bitmap.y+y+1)*texWidth + bitmap.x+x+1].alpha = bitmap.pixels[y*bitmap.width+x];
                }
            }
        }
        for(int y=0; y<bitmap.height; ++y)
        {
            for(int x=0; x<bitmap.width; ++x)
            {
                uint8_t whiteAlpha = bitmap.pixels[y*bitmap.width+x];
                uint8_t blackAlpha = image[(bitmap.y+y)*texWidth + bitmap.x+x].alpha;
                uint8_t alpha = static_cast<uint8_t>(blackAlpha + whiteAlpha - blackAlpha*whiteAlpha/255);
                image[(bitmap.y+y)*texWidth + bitmap.x+x] = {whiteAlpha,alpha}; // Use premultiplied alpha
            }
        }
        stbtt_FreeBitmap(bitmap.pixels, 0);

        auto & glyph = glyphs[bitmap.codepoint];
        glyph.s0 = static_cast<float>(bitmap.x) / texWidth;
        glyph.t0 = static_cast<float>(bitmap.y) / texHeight;
        glyph.s1 = static_cast<float>(bitmap.x + bitmap.width + margin) / texWidth;
        glyph.t1 = static_cast<float>(bitmap.y + bitmap.height + margin) / texHeight;
        glyph.x0 = bitmap.xoff;
        glyph.y0 = bitmap.yoff;
        glyph.x1 = bitmap.xoff + bitmap.width + margin;
        glyph.y1 = bitmap.yoff + bitmap.height + margin;
        glyph.advance = static_cast<int>(bitmap.advance * scale);
    }

    // Upload the image to a GL texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, image.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Retrieve kerning values for pairs of used characters
    for(auto & pair1 : glyphs)
    {
        for(auto & pair2 : glyphs)
        {
            if(int kern = static_cast<int>(stbtt_GetCodepointKernAdvance(&font, pair1.first, pair2.first) * scale))
            {
                pair1.second.kerning[pair2.first] = kern;
            }            
        }
    }
}

int Font::GetStringWidth(const std::string & string) const
{
    int width = 0;
    const Glyph * prev = 0;
    for(auto str = string.c_str(); *str; str = utf8::next(str))
    {
        int codepoint = utf8::code(str);
        if(prev)
        {
            auto it = prev->kerning.find(codepoint);
            if(it != end(prev->kerning)) width += it->second;
        }

        auto it = glyphs.find(codepoint);
        if(it == end(glyphs)) it = glyphs.find('?');
        if(it == end(glyphs)) continue;
        width += it->second.advance;
        prev = &it->second;
    }
    return width;
}

size_t Font::GetUnitIndex(const std::string & string, int x) const
{
    const Glyph * prev = 0;
    for(auto str = string.c_str(); *str; str = utf8::next(str))
    {
        int codepoint = utf8::code(str);
        if(prev)
        {
            auto it = prev->kerning.find(codepoint);
            if(it != end(prev->kerning)) x -= it->second;
        }

        auto it = glyphs.find(codepoint);
        if(it == end(glyphs)) it = glyphs.find('?');
        if(it == end(glyphs)) continue;
        x -= it->second.advance;
        if(x <= 0) return str - string.c_str();
        prev = &it->second;
    }
    return string.size();
}

void Font::DrawString(int x, int y, float r, float g, float b, const std::string & string) const
{
    const Glyph * prev = 0;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glColor4f(r, g, b, 1);
    for(auto str = string.c_str(); *str; str = utf8::next(str))
    {
        int codepoint = utf8::code(str);
        if(prev)
        {
            auto it = prev->kerning.find(codepoint);
            if(it != end(prev->kerning)) x += it->second;
        }

        auto it = glyphs.find(codepoint);
        if(it == end(glyphs))
        {
            it = glyphs.find('?');
        }
        if(it == end(glyphs)) continue;
        auto & g = it->second;

        glTexCoord2f(g.s0, g.t0); glVertex2i(x+g.x0, y+g.y0);
        glTexCoord2f(g.s1, g.t0); glVertex2i(x+g.x1, y+g.y0);
        glTexCoord2f(g.s1, g.t1); glVertex2i(x+g.x1, y+g.y1);
        glTexCoord2f(g.s0, g.t1); glVertex2i(x+g.x0, y+g.y1);
        x += g.advance;
        prev = &g;
    }
    glEnd();
    glPopAttrib();
}

void Font::DrawFontTexture(int x, int y) const
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2i(x, y);
    glTexCoord2f(1, 0); glVertex2i(x + texWidth, y);
    glTexCoord2f(1, 1); glVertex2i(x + texWidth, y + texHeight);
    glTexCoord2f(0, 1); glVertex2i(x, y + texHeight);
    glEnd();
    glPopAttrib();
}