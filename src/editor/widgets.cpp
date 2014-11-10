#include "widgets.h"

#include <algorithm>

///////////////////
// class ListBox //
///////////////////

class ListBoxItem : public gui::Element
{
    ListBox & list;
    int index;
public:
    ListBoxItem(ListBox & list, int index) : list(list), index(index) {}
    gui::DraggerPtr OnClick(const int2 &) override { list.SetSelectedIndex(index); return {}; };
};

void ListBox::SetSelectedIndex(int index)
{
    for(size_t i=0; i<children.size(); ++i)
    {
        if(i != index) children[i].element->text.color = {0.7f,0.7f,0.7f,1};
        else children[i].element->text.color = {1,1,1,1};
    }
    if(index != selectedIndex)
    {
        selectedIndex = index;
        if(onSelectionChanged) onSelectionChanged();
    }
}    

void ListBox::SetItemText(int index, const std::string & text)
{
    if(index < 0 || index >= children.size()) return;
    children[index].element->text.text = text;
}

void ListBox::AddItem(const GuiFactory & factory, const std::string & text)
{
    auto label = factory.MakeLabel(text);

    auto item = std::make_shared<ListBoxItem>(*this, children.size());
    item->text = label->text;

    float y0 = children.empty() ? 0 : children.back().placement.y1.b+2;
    float y1 = y0 + item->text.font->GetLineHeight();
    children.push_back({{{0,0},{0,y0},{1,0},{0,y1}},item});
}

///////////////
// Splitters //
///////////////

struct DimensionDesc { int int2::*coord, gui::Rect::*r0, gui::Rect::*r1; gui::UCoord gui::URect::*u0, gui::URect::*u1; gui::Cursor cursor; };
class SplitterBorder : public gui::Element
{
    class Dragger : public gui::IDragger
    {
        gui::Element & panel;
        gui::URect initialPlacement;
        DimensionDesc dim;
        int click;

        void SetBorderPlacement(const gui::URect & placement)
        {
            panel.children[0].placement = placement;
            panel.children[1].placement.*dim.u1 = placement.*dim.u0;
            panel.children[2].placement.*dim.u0 = placement.*dim.u1;
            panel.SetRect(panel.rect);
        }
    public:
        Dragger(gui::Element & panel, const DimensionDesc & dim, const int2 & click) : panel(panel), initialPlacement(panel.children[0].placement), dim(dim), click(click.*dim.coord) {}
        void OnDrag(int2 newMouse) override
        {
            auto borderRect = initialPlacement.resolve(panel.rect);
            int delta = newMouse.*dim.coord - click;
            delta = std::max(delta, panel.rect.*dim.r0 + 100 - borderRect.*dim.r0);
            delta = std::min(delta, panel.rect.*dim.r1 - 100 - borderRect.*dim.r1);
            auto placement = initialPlacement;
            (placement.*dim.u0).b += delta;
            (placement.*dim.u1).b += delta;
            SetBorderPlacement(placement);
        }
        void OnRelease() override {}
        void OnCancel() override { SetBorderPlacement(initialPlacement); }
    };

    gui::Element & panel;
    DimensionDesc dim;
public:
    SplitterBorder(gui::Element & panel, const DimensionDesc & dim) : panel(panel), dim(dim) { cursor = dim.cursor; style = gui::BACKGROUND; }
    gui::DraggerPtr OnClick(const int2 & mouse) override { return std::make_shared<Dragger>(panel, dim, mouse); }

    NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override
    {
        nvgBeginPath(e.vg);
        nvgRect(e.vg, rect.x0, rect.y0, rect.GetWidth(), rect.GetHeight());
	    nvgFillColor(e.vg, nvgRGBA(64,64,64,255));
	    nvgFill(e.vg);
        return nvgRGBA(64,64,64,255);
    }
};

Splitter::Splitter(gui::ElementPtr panelA, gui::ElementPtr panelB, Side sideB, int pixelsB)
{
    const DimensionDesc dimX {&int2::x, &gui::Rect::x0, &gui::Rect::x1, &gui::URect::x0, &gui::URect::x1, gui::Cursor::SizeWE};
    const DimensionDesc dimY {&int2::y, &gui::Rect::y0, &gui::Rect::y1, &gui::URect::y0, &gui::URect::y1, gui::Cursor::SizeNS};
    auto dim = sideB == Left || sideB == Right ? dimX : dimY;
    auto border = std::make_shared<SplitterBorder>(*this, dim);

    gui::URect placement = {{0,0},{0,0},{1,0},{1,0}}, placeA = placement, placeB = placement;
    switch(sideB)
    {
    case Left: placement.x0 = {0,pixelsB}; placement.x1 = {0,pixelsB+4}; panelA.swap(panelB); break;
    case Top: placement.y0 = {0,pixelsB}; placement.y1 = {0,pixelsB+4}; panelA.swap(panelB); break;
    case Right: placement.x0 = {1,-pixelsB-4}; placement.x1 = {1,-pixelsB}; break;
    case Bottom: placement.y0 = {1,-pixelsB-4}; placement.y1 = {1,-pixelsB}; break;
    }    

    placeA.*dim.u1 = placement.*dim.u0;
    placeB.*dim.u0 = placement.*dim.u1;
    children.push_back({placement, border});
    children.push_back({placeA, panelA});
    children.push_back({placeB, panelB});
}

////////////
// Border //
////////////

Border::Border(int size, float offset, float width, float radius, NVGcolor border, NVGcolor background, gui::ElementPtr inner) :
    size(size), offset(offset), width(width), radius(radius), border(border), background(background)
{
    children.push_back({{{0,size},{0,size},{1,-size},{1,-size}},inner});
}

NVGcolor Border::OnDrawBackground(const gui::DrawEvent & e) const
{
    nvgBeginPath(e.vg);
    nvgRect(e.vg, rect.x0, rect.y0, rect.GetWidth(), rect.GetHeight());
	nvgFillColor(e.vg, background);
	nvgFill(e.vg);
    return background;
}

void Border::OnDrawForeground(const gui::DrawEvent & e) const
{
    // Restore corners of background
    nvgBeginPath(e.vg);
    nvgRect(e.vg, rect.x0, rect.y0, rect.GetWidth(), rect.GetHeight());
    nvgRoundedRect(e.vg, rect.x0+offset, rect.y0+offset, rect.GetWidth()-offset*2, rect.GetHeight()-offset*2, radius);
    nvgPathWinding(e.vg, NVG_HOLE);
	nvgFillColor(e.vg, e.parent);
	nvgFill(e.vg);

    // Stroke an outline for the box
	nvgBeginPath(e.vg);
	nvgRoundedRect(e.vg, rect.x0+offset, rect.y0+offset, rect.GetWidth()-offset*2, rect.GetHeight()-offset*2, radius);
	nvgStrokeColor(e.vg, border);
    nvgStrokeWidth(e.vg, width);
	nvgStroke(e.vg);
}

std::shared_ptr<Border> Border::CreateBigBorder(gui::ElementPtr inner) { return std::make_shared<Border>(4, 1.5f, 2.0f, 4.5f, nvgRGBA(0,0,0,192), nvgRGBA(64,64,64,255), inner); }
std::shared_ptr<Border> Border::CreateEditBorder(gui::ElementPtr inner) { return std::make_shared<Border>(1, 0.5f, 1.0f, 2.5f, nvgRGBA(0,0,0,128), nvgRGBA(88,88,88,255), inner); }