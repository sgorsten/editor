#ifndef EDITOR_GUI_H
#define EDITOR_GUI_H

#include "engine/linalg.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

class Font;

namespace gl { class Texture; }

namespace gui
{
    struct IDragger
    {
        virtual void OnDrag(int2 newMouse) = 0;
        virtual void OnRelease() = 0;
        virtual void OnCancel() = 0;
    };
    typedef std::shared_ptr<IDragger> DraggerPtr;

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

    enum Style { NONE, BACKGROUND, BORDER, EDIT };
    struct Element
    {
        struct TextComponent
        {
            Color color;
            const Font * font;
            std::string text;
            bool isEditable;
        };

        Cursor                                              cursor;
        Style                                               style;
        TextComponent                                       text;
        Rect                                                rect;
        std::vector<Child>                                  children;

        virtual DraggerPtr                                  OnClick(const int2 & mouse) { return nullptr; } // If a dragger is returned, it will take focus until user releases mouse or hits "escape"
        virtual void                                        OnKey(int key, int action, int mods) {}

        virtual void                                        OnDrawBackground() const {} // Draw contents before children
        virtual void                                        OnDrawForeground() const {} // Draw contents after children

        std::function<void(const std::string & text)>       onEdit;
        
                                                            Element();

        void                                                SetRect(const Rect & rect);
    };

    typedef std::shared_ptr<Element> ElementPtr;

    struct Child
    {
        URect placement;
        ElementPtr element;
    };
}

#endif