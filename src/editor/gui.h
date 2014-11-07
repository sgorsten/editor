#ifndef EDITOR_GUI_H
#define EDITOR_GUI_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

class Font;

namespace gl { class Texture; }

namespace gui
{
    enum class Cursor { Arrow, IBeam, SizeNS, SizeWE };

    struct Color
    {
        float r,g,b,a;
    };

    struct Rect
    {
        int x0,y0,x1,y1;
        int GetWidth() const { return x1-x0; }
        int GetHeight() const { return y1-y0; }
        float GetAspect() const { return (float)GetWidth()/GetHeight(); }
    };

    struct UCoord
    {
        float a,b;
        int resolve(int lo, int hi) const { return lo + static_cast<int>(round(a*(hi-lo) + b)); }
    };

    struct URect
    {
        UCoord x0,y0,x1,y1;
        Rect resolve(Rect r) const { return {x0.resolve(r.x0,r.x1), y0.resolve(r.y0,r.y1), x1.resolve(r.x0,r.x1), y1.resolve(r.y0,r.y1)}; }
    };

    struct Child;

    struct Element
    {
        struct BackgroundComponent
        {
            const gl::Texture * image;
            int border;
        };

        struct TextComponent
        {
            Color color;
            const Font * font;
            std::string text;
            bool isEditable;
        };

        Cursor                                          cursor;
        BackgroundComponent                             back;
        TextComponent                                   text;
        Rect                                            rect;
        std::vector<Child>                              children;

        std::function<void()>                           onClick;
        std::function<void(int dx, int dy)>             onDrag;
        std::function<void(const std::string & text)>   onEdit;
        std::function<void(const Rect & rect)>          onDraw;        
        
                                                        Element();

        void                                            SetRect(const Rect & rect);
    };

    typedef std::shared_ptr<Element> ElementPtr;

    struct Child
    {
        URect placement;
        ElementPtr element;
    };
}

#endif