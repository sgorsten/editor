#ifndef EDITOR_WIDGETS_H
#define EDITOR_WIDGETS_H

#include "gui.h"

class GuiFactory
{
    const Font & font;
    int spacing, editBorder;
public:
    GuiFactory(const Font & font, int spacing) : font(font), spacing(spacing), editBorder(2) {}

    gui::ElementPtr AddBorder(int pixels, gui::Style style, gui::ElementPtr inner) const
    {
        auto panel = std::make_shared<gui::Element>();
        panel->style = style;
        panel->children.push_back({{{0,pixels},{0,pixels},{1,-pixels},{1,-pixels}},inner});
        return panel;
    }

    gui::ElementPtr MakeLabel(const std::string & text) const 
    {
        auto elem = std::make_shared<gui::Element>();
        elem->text = {{1,1,1,1},&font,text,false};
        return elem;
    }
    gui::ElementPtr MakePropertyMap(std::vector<std::pair<std::string, gui::ElementPtr>> properties) const
    {
        int y0 = 0;
        auto panel = std::make_shared<gui::Element>();
        for(auto & pair : properties)
        {
            int y1 = y0 + editBorder;
            int y2 = y1 + font.GetLineHeight();
            int y3 = y2 + editBorder;
            panel->children.push_back({{{0,0},{0,y1},{0.5,-spacing*0.5f},{0,y2}}, MakeLabel(pair.first)});
            panel->children.push_back({{{0.5,spacing*0.5f},{0,y0},{1,0},{0,y3}}, pair.second});
            y0 = y3 + spacing;
        }
        return panel;
    }

    gui::ElementPtr MakeEdit(const std::string & text, std::function<void(const std::string & text)> onEdit={}) const 
    { 
        auto elem = std::make_shared<gui::Element>();
        elem->cursor = gui::Cursor::IBeam;
        elem->text = {{1,1,1,1}, &font, text, true};
        elem->onEdit = onEdit;
        return AddBorder(editBorder, gui::EDIT, elem);
    }
    gui::ElementPtr MakeStringEdit(std::string & value) const
    {
        std::ostringstream ss;
        ss << value; 
        return MakeEdit(ss.str(), [&value](const std::string & text) { value = text; });
    }
    gui::ElementPtr MakeFloatEdit(float & value) const
    {
        std::ostringstream ss;
        ss << value; 
        return MakeEdit(ss.str(), [&value](const std::string & text) { std::istringstream(text) >> value; });
    }
    gui::ElementPtr MakeVectorEdit(float3 & value) const
    {
        auto panel = std::make_shared<gui::Element>();
        panel->children.push_back({{{0.0f/3, 0},{0,0},{1.0f/3,-spacing*2.0f/3},{1,0}},MakeFloatEdit(value.x)});
        panel->children.push_back({{{1.0f/3,+spacing*1.0f/3},{0,0},{2.0f/3,-spacing*1.0f/3},{1,0}},MakeFloatEdit(value.y)});
        panel->children.push_back({{{2.0f/3,+spacing*2.0f/3},{0,0},{3.0f/3, 0},{1,0}},MakeFloatEdit(value.z)});
        return panel;
    }

    gui::ElementPtr MakeNSSizer(gui::ElementPtr top, gui::ElementPtr bottom, int split)
    {
        auto panel = std::make_shared<gui::Element>();
        auto weak = std::weak_ptr<gui::Element>(panel);

        auto sizer = std::make_shared<gui::Element>();
        sizer->cursor = gui::Cursor::SizeNS;
        sizer->style = gui::BACKGROUND;
        sizer->onDrag = [weak](int dx, int dy)
        {
            if(auto panel = weak.lock())
            {
                panel->children[0].placement.y0.b += dy;
                panel->children[0].placement.y1.b += dy;
                panel->children[1].placement.y1 = panel->children[0].placement.y0;
                panel->children[2].placement.y0 = panel->children[0].placement.y1;
                panel->SetRect(panel->rect);
            }
        };

        int height = 4;
        if(split > 0) panel->children.push_back({{{0,0},{0,split},{1,0},{0,split+height}}, sizer});
        if(split < 0) panel->children.push_back({{{0,0},{1,split-height},{1,0},{1,split}}, sizer});
        if(split == 0) panel->children.push_back({{{0,0},{0.5,-height*0.5f},{1,0},{0.5,height*0.5f}}, sizer});
        panel->children.push_back({{{0,0},{0,0},{1,0},panel->children[0].placement.y0},top});
        panel->children.push_back({{{0,0},panel->children[0].placement.y1,{1,0},{1,0}},bottom});
        return panel;
    }

    gui::ElementPtr MakeWESizer(gui::ElementPtr left, gui::ElementPtr right, int split)
    {
        auto panel = std::make_shared<gui::Element>();
        auto weak = std::weak_ptr<gui::Element>(panel);

        auto sizer = std::make_shared<gui::Element>();
        sizer->cursor = gui::Cursor::SizeWE;
        sizer->style = gui::BACKGROUND;
        sizer->onDrag = [weak](int dx, int dy)
        {
            if(auto panel = weak.lock())
            {
                panel->children[0].placement.x0.b += dx;
                panel->children[0].placement.x1.b += dx;
                panel->children[1].placement.x1 = panel->children[0].placement.x0;
                panel->children[2].placement.x0 = panel->children[0].placement.x1;
                panel->SetRect(panel->rect);
            }
        };

        int width = 4;
        if(split > 0) panel->children.push_back({{{0,split},{0,0},{0,split+width},{1,0}}, sizer});
        if(split < 0) panel->children.push_back({{{1,split-width},{0,0},{1,split},{1,0}}, sizer});
        if(split == 0) panel->children.push_back({{{0.5,-width*0.5f},{0,0},{0.5,width*0.5f},{1,0}}, sizer});
        panel->children.push_back({{{0,0},{0,0},panel->children[0].placement.x0,{1,0}},left});
        panel->children.push_back({{panel->children[0].placement.x1,{0,0},{1,0},{1,0}},right});
        return panel;
    }
};

class ListControl
{
    gui::ElementPtr panel;
    int selectedIndex;
public:
    ListControl() : panel(std::make_shared<gui::Element>()), selectedIndex(-1) {}

    gui::ElementPtr GetPanel() const { return panel; }
    int GetSelectedIndex() const { return selectedIndex; }

    void SetSelectedIndex(int index)
    {
        for(size_t i=0; i<panel->children.size(); ++i)
        {
            if(i != index) panel->children[i].element->text.color = {0.7f,0.7f,0.7f,1};
            else panel->children[i].element->text.color = {1,1,1,1};
        }
        if(index != selectedIndex)
        {
            selectedIndex = index;
            if(onSelectionChanged) onSelectionChanged();
        }
    }    

    void AddItem(const GuiFactory & factory, const std::string & text)
    {
        auto label = factory.MakeLabel(text);

        float y0 = panel->children.empty() ? 0 : panel->children.back().placement.y1.b+2;
        float y1 = y0 + label->text.font->GetLineHeight();
        int index = panel->children.size();
        panel->children.push_back({{{0,0},{0,y0},{1,0},{0,y1}},label});
        label->onClick = [this,index](int, int) { SetSelectedIndex(index); };
    }

    std::function<void()> onSelectionChanged;
};

#endif