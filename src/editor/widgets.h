#ifndef EDITOR_WIDGETS_H
#define EDITOR_WIDGETS_H

#include "gui.h"
#include "engine/font.h"
#include "engine/asset.h"
#include <sstream>

class GuiFactory
{
    const Font & font;
    int spacing, editBorder;
public:
    GuiFactory(const Font & font, int spacing) : font(font), spacing(spacing), editBorder(2) {}

    gui::ElementPtr MakeLabel(const std::string & text) const
    {
        auto elem = std::make_shared<gui::Text>(font);
        elem->text = text;
        elem->minimumSize = {font.GetStringWidth(text), font.GetLineHeight()};
        return elem;
    }

    gui::ElementPtr MakePropertyMap(std::vector<std::pair<std::string, gui::ElementPtr>> properties) const
    {
        float y0 = 0;
        auto labelPanel = std::make_shared<gui::Element>(), valuePanel = std::make_shared<gui::Element>();
        valuePanel->minimumSize.x = 128;
        for(auto & pair : properties)
        {
            float y1 = y0 + editBorder, y2 = y1 + font.GetLineHeight(), y3 = y2 + editBorder;
            labelPanel->children.push_back({{{0,0},{0,y1},{1,0},{0,y2}}, MakeLabel(pair.first)});
            valuePanel->children.push_back({{{0,0},{0,y0},{1,0},{0,y3}}, pair.second});
            y0 = y3 + spacing;
        }
        return std::make_shared<gui::Splitter>(valuePanel, labelPanel, gui::Splitter::Left, labelPanel->GetMinimumSize().x);       
    }

    gui::ElementPtr MakeEdit(const std::string & text, std::function<void(const std::string & text)> onEdit={}) const
    { 
        auto elem = std::make_shared<gui::Text>(font);
        elem->text = text;
        elem->isEditable = true;
        elem->onEdit = onEdit;
        return gui::Border::CreateEditBorder(elem);
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
    gui::ElementPtr MakeVectorEdit(float4 & value) const
    {
        auto panel = std::make_shared<gui::Element>();
        panel->children.push_back({{{0.0f/4, 0},{0,0},{1.0f/4,-spacing*3.0f/4},{1,0}},MakeFloatEdit(value.x)});
        panel->children.push_back({{{1.0f/4,+spacing*1.0f/4},{0,0},{2.0f/4,-spacing*2.0f/4},{1,0}},MakeFloatEdit(value.y)});
        panel->children.push_back({{{2.0f/4,+spacing*2.0f/4},{0,0},{3.0f/4,-spacing*1.0f/4},{1,0}},MakeFloatEdit(value.z)});
        panel->children.push_back({{{3.0f/4,+spacing*3.0f/4},{0,0},{4.0f/4, 0},{1,0}},MakeFloatEdit(value.w)});
        return panel;
    }
    template<class T> gui::ElementPtr MakeAssetHandleEdit(AssetLibrary & assets, AssetLibrary::Handle<T> & value) const
    {
        return MakeEdit(value ? value.GetId() : "{None}", [&assets, &value](const std::string & text) 
        {
            value = assets.GetAsset<T>(text);
        });
    }
};

#endif