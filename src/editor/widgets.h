#ifndef EDITOR_WIDGETS_H
#define EDITOR_WIDGETS_H

#include "gui.h"
#include "engine/font.h"
#include <sstream>
#include <algorithm>

class Text : public gui::Element
{
    const Font & font;
    size_t cursor, mark;
    bool isSelecting;

    const char * GetFocusText() const { return text.data(); }
    size_t GetFocusTextSize() const { return text.size(); }   
public:
    std::string text;
    bool isEditable;
    NVGcolor color;

    Text(const Font & font) : font(font), cursor(), mark(), isSelecting(), isEditable(), color(nvgRGBA(255,255,255,255)) {}

    size_t GetSelectionLeftIndex() const { return std::min(cursor, mark); }
    size_t GetSelectionRightIndex() const { return std::max(cursor, mark); }
    std::string GetSelectionText() const { return std::string(text.c_str() + GetSelectionLeftIndex(), text.c_str() + GetSelectionRightIndex()); }

    void SelectAll();
    void MoveSelectionCursor(int newCursor, bool holdingShift);
    void RemoveSelection();
    void Insert(const char * string);

    bool IsTabStop() const override { return isEditable; }
    gui::Cursor GetCursor() const override { return isEditable ? gui::Cursor::IBeam : gui::Cursor::Arrow; }

    void OnChar(uint32_t codepoint) override;
    void OnKey(GLFWwindow * window, int key, int action, int mods) override;
    gui::DraggerPtr OnClick(const gui::MouseEvent & e) override;
    void OnTab() override { SelectAll(); }
    NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override;

    std::function<void(const std::string &)> onEdit;
};

class Splitter : public gui::Element
{
public:
    enum Side { Left, Top, Right, Bottom };

    Splitter(gui::ElementPtr panelA, gui::ElementPtr panelB, Side sideB, int pixelsB);
};

class Border : public gui::Element
{
    int size; // Size of border region in pixels
    float offset, width, radius; // Placement of stroked border
    NVGcolor border, background; // Color of border, and internal fill
public:
    Border(int size, float offset, float width, float radius, NVGcolor border, NVGcolor background, gui::ElementPtr inner);

    NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override;
    void OnDrawForeground(const gui::DrawEvent & e) const override;

    static std::shared_ptr<Border> CreateBigBorder(gui::ElementPtr inner);
    static std::shared_ptr<Border> CreateEditBorder(gui::ElementPtr inner);
};

class ListBox : public gui::Element
{
    const Font & font;
    int spacing, selectedIndex;
public:
    ListBox(const Font & font, int spacing) : font(font), spacing(spacing), selectedIndex(-1) {}

    int GetSelectedIndex() const { return selectedIndex; }  

    void SetSelectedIndex(int index);
    void SetItemText(int index, const std::string & text);
    void AddItem(const std::string & text);

    std::function<void()> onSelectionChanged;
};

class Menu;

class ModalBarrier : public gui::Element
{
    Menu & menu;
public:
    ModalBarrier(Menu & menu) : menu(menu) {}

    gui::DraggerPtr OnClick(const gui::MouseEvent & e) override;
};

class MenuButton : public gui::Element
{
    std::function<void()> func;
public:
    MenuButton(const Font & font, const std::string & label, std::function<void()> func);
               
    NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override;
    gui::DraggerPtr OnClick(const gui::MouseEvent & e) override;
};

class Menu : public gui::Element
{
    friend class ModalBarrier;

    const Font & font;
    std::shared_ptr<ModalBarrier> mbar;
    std::vector<std::shared_ptr<Menu>> popups;
    Menu * topLevel;
    size_t level;

    void ActivatePopup(size_t level, std::shared_ptr<Menu> popup);
    void DeactivateBarrier();
public:
    Menu(const Font & font) : font(font), mbar(std::make_shared<ModalBarrier>(*this)), topLevel(nullptr), level(0) {}

    void AddItem(const std::string & label, std::function<void()> func);
    std::shared_ptr<Menu> AddPopup(const std::string & label);
    
    std::shared_ptr<ModalBarrier> GetModalBarrier() { return mbar; }

    NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override;
};

class GuiFactory
{
    const Font & font;
    int spacing, editBorder;
public:
    GuiFactory(const Font & font, int spacing) : font(font), spacing(spacing), editBorder(2) {}

    gui::ElementPtr MakeLabel(const std::string & text) const
    {
        auto elem = std::make_shared<Text>(font);
        elem->text = text;
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
        auto elem = std::make_shared<Text>(font);
        elem->text = text;
        elem->isEditable = true;
        elem->onEdit = onEdit;
        return Border::CreateEditBorder(elem);
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
};

#endif