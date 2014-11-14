#include "widgets.h"
#include "engine/utf8.h"

#include <algorithm>

////////////////
// class Fill //
////////////////

NVGcolor Fill::OnDrawBackground(const gui::DrawEvent & e) const
{
    nvgBeginPath(e.vg);
    nvgRect(e.vg, rect.x0, rect.y0, rect.GetWidth(), rect.GetHeight());
	nvgFillColor(e.vg, color);
	nvgFill(e.vg);
    return color;
}

////////////////
// class Text //
////////////////

void Text::SelectAll()
{
    isSelecting = true;
    mark = 0;
    cursor = text.size();
}

void Text::MoveSelectionCursor(int newCursor, bool holdingShift)
{
    if(holdingShift)
    {
        if(!isSelecting)
        {
            mark = cursor;
            isSelecting = true;
        }
    }
    else isSelecting = false;
    cursor = newCursor;
}

void Text::RemoveSelection()
{
    if(!isSelecting || !isEditable) return;
    auto left = GetSelectionLeftIndex(), right = GetSelectionRightIndex();
    text.erase(left, right-left);
    cursor = left;
    isSelecting = false;
    if(onEdit) onEdit(text);
}

void Text::Insert(const char * string)
{
    if(!isEditable) return;
    text.insert(cursor, string);
    cursor += strlen(string);
    if(onEdit) onEdit(text);
}

void Text::OnChar(uint32_t codepoint)
{
    if(isSelecting) RemoveSelection();
    Insert(utf8::units(codepoint).data());
}

void Text::OnKey(GLFWwindow * window, int key, int action, int mods)
{
    // Text elements only respond to key presses, not releases
    if(action == GLFW_RELEASE) return;

    if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_A)
    {
        SelectAll();
        return;
    }

    if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_C)
    {
        if(isSelecting)
        {
            auto ct = GetSelectionText();
            glfwSetClipboardString(window, ct.c_str());
        }
        return;
    }

    // All subsequent commands apply only to editable text elements
    if(!isEditable) return;
    
    if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_X)
    {
        if(isSelecting)
        {
            auto ct = GetSelectionText();
            glfwSetClipboardString(window, ct.c_str());
            RemoveSelection();
        }
        return;
    }

    if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_V)
    {
        if(auto ct = glfwGetClipboardString(window))
        {
            RemoveSelection();
            Insert(ct);
        }
        return;
    }
                    
    switch(key)
    {
    case GLFW_KEY_LEFT:
        if(cursor > 0) MoveSelectionCursor(utf8::prev(GetFocusText() + cursor) - GetFocusText(), (mods & GLFW_MOD_SHIFT) != 0);
        break;
    case GLFW_KEY_RIGHT: 
        if(cursor < GetFocusTextSize()) MoveSelectionCursor(cursor = utf8::next(GetFocusText() + cursor) - GetFocusText(), (mods & GLFW_MOD_SHIFT) != 0);
        break;
    case GLFW_KEY_HOME:
        if(cursor > 0) MoveSelectionCursor(0, (mods & GLFW_MOD_SHIFT) != 0);
        break;
    case GLFW_KEY_END: 
        if(cursor < GetFocusTextSize()) MoveSelectionCursor(GetFocusTextSize(), (mods & GLFW_MOD_SHIFT) != 0);
        break;
    case GLFW_KEY_BACKSPACE: 
        if(isSelecting) RemoveSelection();
        else if(isEditable && cursor > 0)
        {
            int prev = utf8::prev(GetFocusText() + cursor) - GetFocusText();
            text.erase(prev, cursor - prev);
            cursor = prev;
            if(onEdit) onEdit(text);
        }
        break;
    case GLFW_KEY_DELETE:
        if(isSelecting) RemoveSelection();
        else if(isEditable && cursor < GetFocusTextSize())
        {
            auto next = utf8::next(GetFocusText() + cursor) - GetFocusText();
            text.erase(cursor, next - cursor);
            if(onEdit) onEdit(text);
        }
        break;
    }
}

gui::DraggerPtr Text::OnClick(const gui::MouseEvent & e)
{
    class SelectionDragger : public gui::IDragger
    {
        Text & text;
    public:
        SelectionDragger(Text & text) : text(text) {}
        void OnDrag(int2 newMouse) override { text.MoveSelectionCursor(text.font.GetUnitIndex(text.text, newMouse.x - text.rect.x0), true); }
        void OnRelease() override {}
        void OnCancel() override {}
    };

    isSelecting = false;
    MoveSelectionCursor(font.GetUnitIndex(text, e.cursor.x - rect.x0), e.IsShiftHeld());
    return std::make_shared<SelectionDragger>(*this);
}

NVGcolor Text::OnDrawBackground(const gui::DrawEvent & e) const
{
    // Draw selection if we are selecting
    if(e.hasFocus && isSelecting)
    {
        const int x = rect.x0 + font.GetStringWidth(text.substr(0,GetSelectionLeftIndex()));
        const int w = font.GetStringWidth(text.substr(GetSelectionLeftIndex(),GetSelectionRightIndex()-GetSelectionLeftIndex()));

        nvgBeginPath(e.vg);
        nvgRect(e.vg, x, rect.y0, w, font.GetLineHeight());
        nvgFillColor(e.vg, nvgRGBA(0,255,255,128));
        nvgFill(e.vg);
    }          

    // Draw the text itself
    font.DrawString(rect.x0, rect.y0, color, text);

    // Draw cursor if we have focus
    if(e.hasFocus)
    {
        int x = rect.x0 + font.GetStringWidth(text.substr(0,cursor));

        nvgBeginPath(e.vg);
        nvgRect(e.vg, x, rect.y0, 1, font.GetLineHeight());
        nvgFillColor(e.vg, nvgRGBA(255,255,255,192));
        nvgFill(e.vg);
    }

    if(font.GetStringWidth(text) > rect.GetWidth())
    {
        // Fade text to the right
        auto transparentBackground = e.parent;
        transparentBackground.a = 0;
        auto bg = nvgLinearGradient(e.vg, rect.x1-6, rect.y0, rect.x1, rect.y0, transparentBackground, e.parent);
        nvgBeginPath(e.vg);
        nvgRect(e.vg, rect.x1-6, rect.y0, 6, font.GetLineHeight());
        nvgFillPaint(e.vg, bg);
        nvgFill(e.vg);
    }

    return e.parent;
}

///////////////////
// class ListBox //
///////////////////

class ListBoxItem : public Text
{
    ListBox & list;
    int index;
public:
    ListBoxItem(const Font & font, ListBox & list, int index) : Text(font), list(list), index(index) {}
    gui::DraggerPtr OnClick(const gui::MouseEvent & e) override { list.SetSelectedIndex(index); return Text::OnClick(e); };
};

void ListBox::SetSelectedIndex(int index)
{
    for(size_t i=0; i<children.size(); ++i)
    {
        auto child = reinterpret_cast<ListBoxItem *>(children[i].element.get()); // TODO: Fix
        if(i != index) child->color = nvgRGBA(179,179,179,255);
        else child->color = nvgRGBA(255,255,255,255);
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
    auto child = reinterpret_cast<ListBoxItem *>(children[index].element.get()); // TODO: Fix
    child->text = text;
}

void ListBox::AddItem(const std::string & text)
{
    auto item = std::make_shared<ListBoxItem>(font, *this, children.size());
    item->text = text;

    float y0 = children.empty() ? 0 : children.back().placement.y1.b + spacing;
    float y1 = y0 + font.GetLineHeight();
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
    SplitterBorder(gui::Element & panel, const DimensionDesc & dim) : panel(panel), dim(dim) {}
    gui::Cursor GetCursor() const override { return dim.cursor; }
    gui::DraggerPtr OnClick(const gui::MouseEvent & e) override { return std::make_shared<Dragger>(panel, dim, e.cursor); }

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
std::shared_ptr<Border> Border::CreateEditBorder(gui::ElementPtr inner) { return std::make_shared<Border>(2, 0.5f, 1.0f, 2.5f, nvgRGBA(0,0,0,128), nvgRGBA(88,88,88,255), inner); }

//////////
// Menu //
//////////

class Menu::Barrier : public gui::Element
{
    std::vector<gui::ElementPtr> popups;
public:
    Barrier() { HidePopups(); }

    void HidePopups()
    {
        while(!popups.empty())
        {
            popups.back()->isVisible = false;
            popups.pop_back();
        }
        isTransparent = true;
    }

    void ShowPopup(size_t level, gui::ElementPtr popup)
    {
        while(popups.size() > level)
        {
            popups.back()->isVisible = false;
            popups.pop_back();
        }
        popups.push_back(popup);
        popup->isVisible = true;
        isTransparent = false;
    }

    gui::DraggerPtr OnClick(const gui::MouseEvent & e) { HidePopups(); return {}; }
};

class MenuButton : public gui::Element
{
    std::function<void()> func;
    bool isPopup;
public:
    MenuButton(const Font & font, const std::string & label, int offset, std::function<void()> func, bool isPopup) : func(func), isPopup(isPopup)
    {
        auto text = std::make_shared<Text>(font);
        text->isTransparent = true;
        text->text = label;
        children.push_back({{{0,offset},{0,2},{1,-offset},{1,-2}}, text});
    }

    gui::DraggerPtr OnClick(const gui::MouseEvent & e) { func(); return {}; }

    NVGcolor MenuButton::OnDrawBackground(const gui::DrawEvent & e) const
    {
        auto bg = e.parent;

        if(e.isMouseOver)
        {
	        nvgBeginPath(e.vg);
	        nvgRect(e.vg, rect.x0, rect.y0, rect.GetWidth(), rect.GetHeight());
            nvgFillColor(e.vg, nvgRGBA(115,98,50,255));
            nvgFill(e.vg);

	        nvgBeginPath(e.vg);
	        nvgRect(e.vg, rect.x0+0.5f, rect.y0+0.5f, rect.GetWidth()-1, rect.GetHeight()-1);
	        nvgStrokeColor(e.vg, nvgRGBA(253,244,191,255));
            nvgStrokeWidth(e.vg, 1);
	        nvgStroke(e.vg);
            bg = nvgRGBA(115,98,50,255);
        }

        if(isPopup)
        {
            nvgBeginPath(e.vg);
            nvgMoveTo(e.vg, rect.x1 - 8, rect.GetCenterY() - 3);
            nvgLineTo(e.vg, rect.x1 - 3, rect.GetCenterY() + 0);
            nvgLineTo(e.vg, rect.x1 - 8, rect.GetCenterY() + 3);
            nvgLineTo(e.vg, rect.x1 - 8, rect.GetCenterY() - 3);
            nvgFillColor(e.vg, nvgRGBA(255,255,255,255));
            nvgFill(e.vg);            
        }

        return bg;
    }
};

std::weak_ptr<gui::Element> Menu::MakePopup(size_t level, const Font & font, const std::vector<MenuItem> & items, float x, float y)
{
    auto popup = std::make_shared<gui::Element>();

    auto bar = barrier;
    gui::URect placement = {{0,0}, {0,0}, {1,0}, {0,0}};
    for(auto & item : items)
    {
        placement.y0 = placement.y1;
        placement.y1.b += font.GetLineHeight() + 4;

        auto func = item.onClick;
        if(!item.children.empty())
        {
            auto childPopup = MakePopup(level + 1, font, item.children, x+98, y+placement.y0.b);
            func = [bar,childPopup,level]() { if(auto pop = childPopup.lock()) bar->ShowPopup(level, pop); };
        }
        else
        {
            auto f = item.onClick;
            func = [bar,f]() { bar->HidePopups(); f(); };
        }
        popup->AddChild(placement, std::make_shared<MenuButton>(font, item.label, 10, func, !item.children.empty()));
    }

    popup = std::make_shared<Border>(1, 0.5f, 1.0f, 0, nvgRGBA(0,0,0,255), nvgRGBA(64,64,64,255), popup);

    AddChild({{0,x}, {0,y}, {0,x+100}, {0,y+placement.y1.b+2}}, popup);
    popup->isVisible = false;
    return popup;
}

Menu::Menu(gui::ElementPtr inner, const Font & font, const std::vector<MenuItem> & items)
{
    float menuSize = 24;
    AddChild({{0,0}, {0,0}, {1,0}, {0,menuSize}}, std::make_shared<Fill>(nvgRGBA(64,64,64,255)));
    AddChild({{0,0}, {0,menuSize}, {1,0}, {1,0}}, inner);

    barrier = std::make_shared<Barrier>();
    AddChild({{0,0}, {0,0}, {1,0}, {1,0}}, barrier);

    auto bar = barrier;
    gui::URect placement = {{0,0}, {0,0}, {0,0}, {0,menuSize}};
    for(auto & item : items)
    {
        placement.x0 = placement.x1;
        placement.x1.b += font.GetStringWidth(item.label) + 20;
        auto func = item.onClick;
        if(!item.children.empty())
        {
            auto popup = MakePopup(1, font, item.children, placement.x0.b, placement.y1.b);
            func = [bar,popup]() { if(auto pop = popup.lock()) bar->ShowPopup(0, pop); };
        }
        else
        {
            auto f = item.onClick;
            func = [bar,f]() { bar->HidePopups(); f(); };
        }

        AddChild(placement, std::make_shared<MenuButton>(font, item.label, 10, func, false));
    }

    std::reverse(children.begin()+3, children.end());
}