#include "gui.h"
#include "engine/gl.h"
#include <algorithm>

namespace gui
{
    Element::Element() : rect({}), isVisible(true), isTransparent(false) {}

    int2 Element::GetMinimumSize() const
    {
        int2 size = minimumSize;
        for(auto & child : children)
        {
            auto childSize = child.element->GetMinimumSize();

            if(0 < child.placement.x0.a) size.x = std::max(size.x, (int32_t)ceilf(-child.placement.x0.b / child.placement.x0.a));
            if(child.placement.x0.a < child.placement.x1.a) size.x = std::max(size.x, (int32_t)ceilf((childSize.x - (child.placement.x1.b - child.placement.x0.b)) / (child.placement.x1.a - child.placement.x0.a)));
            if(child.placement.x1.a < 1) size.x = std::max(size.x, (int32_t)ceilf(child.placement.x1.b / (1-child.placement.x1.a)));

            if(0 < child.placement.y0.a) size.y = std::max(size.y, (int32_t)ceilf(-child.placement.y0.b / child.placement.y0.a));
            if(child.placement.y0.a < child.placement.y1.a) size.y = std::max(size.y, (int32_t)ceilf((childSize.y - (child.placement.y1.b - child.placement.y0.b)) / (child.placement.y1.a - child.placement.y0.a)));
            if(child.placement.y1.a < 1) size.y = std::max(size.y, (int32_t)ceilf(child.placement.y1.b / (1-child.placement.y1.a)));
        }
        return size;
    }

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