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
    gui::DraggerPtr OnClick(int x, int y) { list.SetSelectedIndex(index); return {}; };
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

class NSSplitterBorder : public gui::Element
{
    class Dragger : public gui::IDragger
    {
        gui::Element & panel;
        gui::URect initialPlacement;
        int clickY;
    public:
        Dragger(gui::Element & panel, const int2 & click) : panel(panel), initialPlacement(panel.children[0].placement), clickY(click.y) {}
        void OnDrag(int2 newMouse) override
        {
            auto borderRect = initialPlacement.resolve(panel.rect);
            int dy = newMouse.y - clickY;
            dy = std::max(dy, 100 - borderRect.y0);
            dy = std::min(dy, panel.rect.GetHeight() - 100 - borderRect.y1);
            auto placement = initialPlacement;
            placement.y0.b += dy;
            placement.y1.b += dy;
            panel.children[0].placement = placement;
            panel.children[1].placement.y1 = placement.y0;
            panel.children[2].placement.y0 = placement.y1;
            panel.SetRect(panel.rect);
        }
        void OnRelease() override {}
        void OnCancel() override {}
    };

    gui::Element & panel;
public:
    NSSplitterBorder(gui::Element & panel) : panel(panel) { cursor = gui::Cursor::SizeNS; style = gui::BACKGROUND; }
    gui::DraggerPtr OnClick(int x, int y) override { return std::make_shared<Dragger>(panel, int2(x,y)); }
};

gui::ElementPtr GuiFactory::MakeNSSizer(gui::ElementPtr top, gui::ElementPtr bottom, int split)
{
    auto panel = std::make_shared<gui::Element>();
    auto sizer = std::make_shared<NSSplitterBorder>(*panel);
    int height = 4;
    if(split > 0) panel->children.push_back({{{0,0},{0,split},{1,0},{0,split+height}}, sizer});
    if(split < 0) panel->children.push_back({{{0,0},{1,split-height},{1,0},{1,split}}, sizer});
    if(split == 0) panel->children.push_back({{{0,0},{0.5,-height*0.5f},{1,0},{0.5,height*0.5f}}, sizer});
    panel->children.push_back({{{0,0},{0,0},{1,0},panel->children[0].placement.y0},top});
    panel->children.push_back({{{0,0},panel->children[0].placement.y1,{1,0},{1,0}},bottom});
    return panel;
}

class WESplitterBorder : public gui::Element
{
    class Dragger : public gui::IDragger
    {
        gui::Element & panel;
        gui::URect initialPlacement;
        int clickX;
    public:
        Dragger(gui::Element & panel, const int2 & click) : panel(panel), initialPlacement(panel.children[0].placement), clickX(click.x) {}
        void OnDrag(int2 newMouse) override
        {
            auto borderRect = initialPlacement.resolve(panel.rect);
            int dx = newMouse.x - clickX;
            dx = std::max(dx, 100 - borderRect.x0);
            dx = std::min(dx, panel.rect.GetWidth() - 100 - borderRect.x1);
            auto placement = initialPlacement;
            placement.x0.b += dx;
            placement.x1.b += dx;
            panel.children[0].placement = placement;
            panel.children[1].placement.x1 = placement.x0;
            panel.children[2].placement.x0 = placement.x1;
            panel.SetRect(panel.rect);
        }
        void OnRelease() override {}
        void OnCancel() override {}
    };

    gui::Element & panel;
public:
    WESplitterBorder(gui::Element & panel) : panel(panel) { cursor = gui::Cursor::SizeWE; style = gui::BACKGROUND; }
    gui::DraggerPtr OnClick(int x, int y) override { return std::make_shared<Dragger>(panel, int2(x,y)); }
};

gui::ElementPtr GuiFactory::MakeWESizer(gui::ElementPtr left, gui::ElementPtr right, int split)
{
    auto panel = std::make_shared<gui::Element>();
    auto sizer = std::make_shared<WESplitterBorder>(*panel);
    int width = 4;
    if(split > 0) panel->children.push_back({{{0,split},{0,0},{0,split+width},{1,0}}, sizer});
    if(split < 0) panel->children.push_back({{{1,split-width},{0,0},{1,split},{1,0}}, sizer});
    if(split == 0) panel->children.push_back({{{0.5,-width*0.5f},{0,0},{0.5,width*0.5f},{1,0}}, sizer});
    panel->children.push_back({{{0,0},{0,0},panel->children[0].placement.x0,{1,0}},left});
    panel->children.push_back({{panel->children[0].placement.x1,{0,0},{1,0},{1,0}},right});
    return panel;
}