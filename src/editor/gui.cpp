#include "gui.h"
#include "gl.h"

namespace gui
{
    Element::Element() : cursor(Cursor::Arrow), back({nullptr,0}), text({{1,1,1,1},nullptr,{},false}), rect({}) {}

    void Element::SetRect(const Rect & rect)
    {
        this->rect = rect;
        for(auto & child : children)
        {
            child.element->SetRect(child.placement.resolve(rect));
        }
    }
}