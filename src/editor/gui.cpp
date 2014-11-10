#include "gui.h"
#include "engine/gl.h"

namespace gui
{
    Element::Element() : cursor(Cursor::Arrow), style(NONE), rect({}) {}

    void Element::SetRect(const Rect & rect)
    {
        this->rect = rect;
        for(auto & child : children)
        {
            child.element->SetRect(child.placement.resolve(rect));
        }
    }
}