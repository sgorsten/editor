#include "gui.h"
#include "engine/gl.h"

namespace gui
{
    Element::Element() : rect({}), isVisible(true), isTransparent(false) {}

    void Element::AddChild(const URect & placement, ElementPtr child)
    {
        children.push_back({placement, child});
    }

    void Element::SetRect(const Rect & rect)
    {
        this->rect = rect;
        for(auto & child : children)
        {
            child.element->SetRect(child.placement.resolve(rect));
        }
    }
}