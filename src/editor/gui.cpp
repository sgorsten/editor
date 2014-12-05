#include "gui.h"
#include "engine/gl.h"
#include "engine/utf8.h"
#include "engine/font.h"
#include "window.h"

#include <sstream>
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

    ////////////////
    // class Fill //
    ////////////////

    NVGcolor Fill::OnDrawBackground(const DrawEvent & e) const
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

    bool Text::OnKey(GLFWwindow * window, int key, int action, int mods)
    {
        // Text elements only respond to key presses, not releases
        if(action == GLFW_RELEASE) return false;

        if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_A)
        {
            SelectAll();
            return true;
        }

        if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_C)
        {
            if(isSelecting)
            {
                auto ct = GetSelectionText();
                glfwSetClipboardString(window, ct.c_str());
            }
            return true;
        }

        // All subsequent commands apply only to editable text elements
        if(!isEditable) return false;
    
        if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_X)
        {
            if(isSelecting)
            {
                auto ct = GetSelectionText();
                glfwSetClipboardString(window, ct.c_str());
                RemoveSelection();
            }
            return true;
        }

        if(mods & GLFW_MOD_CONTROL && key == GLFW_KEY_V)
        {
            if(auto ct = glfwGetClipboardString(window))
            {
                RemoveSelection();
                Insert(ct);
            }
            return true;
        }
                    
        switch(key)
        {
        case GLFW_KEY_LEFT:
            if(cursor > 0) MoveSelectionCursor(utf8::prev(GetFocusText() + cursor) - GetFocusText(), (mods & GLFW_MOD_SHIFT) != 0);
            return true;
        case GLFW_KEY_RIGHT: 
            if(cursor < GetFocusTextSize()) MoveSelectionCursor(cursor = utf8::next(GetFocusText() + cursor) - GetFocusText(), (mods & GLFW_MOD_SHIFT) != 0);
            return true;
        case GLFW_KEY_HOME:
            if(cursor > 0) MoveSelectionCursor(0, (mods & GLFW_MOD_SHIFT) != 0);
            return true;
        case GLFW_KEY_END: 
            if(cursor < GetFocusTextSize()) MoveSelectionCursor(GetFocusTextSize(), (mods & GLFW_MOD_SHIFT) != 0);
            return true;
        case GLFW_KEY_BACKSPACE: 
            if(isSelecting) RemoveSelection();
            else if(cursor > 0)
            {
                int prev = utf8::prev(GetFocusText() + cursor) - GetFocusText();
                text.erase(prev, cursor - prev);
                cursor = prev;
                if(onEdit) onEdit(text);
            }
            return true;
        case GLFW_KEY_DELETE:
            if(isSelecting) RemoveSelection();
            else if(cursor < GetFocusTextSize())
            {
                auto next = utf8::next(GetFocusText() + cursor) - GetFocusText();
                text.erase(cursor, next - cursor);
                if(onEdit) onEdit(text);
            }
            return true;
        }

        return false;
    }

    DraggerPtr Text::OnClick(const MouseEvent & e)
    {
        class SelectionDragger : public IDragger
        {
            Text & text;
        public:
            SelectionDragger(Text & text) : text(text) {}
            void OnDrag(int2 newMouse) override { text.MoveSelectionCursor(text.font.GetUnitIndex(text.text, newMouse.x - text.rect.x0), true); }
            void OnRelease() override {}
            void OnCancel() override {}
        };

        isSelecting = false;
        auto newCursor = font.GetUnitIndex(text, e.cursor.x - rect.x0);
        MoveSelectionCursor(newCursor, e.IsShiftHeld());
        return std::make_shared<SelectionDragger>(*this);
    }

    NVGcolor Text::OnDrawBackground(const DrawEvent & e) const
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
        DraggerPtr OnClick(const MouseEvent & e) override { list.SetSelectedIndex(index); return Text::OnClick(e); };
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

    struct DimensionDesc { int int2::*coord, Rect::*r0, Rect::*r1; UCoord URect::*u0, URect::*u1; Cursor cursor; };
    class SplitterBorder : public Element
    {
        class Dragger : public IDragger
        {
            Element & panel;
            URect initialPlacement;
            DimensionDesc dim;
            int click;

            void SetBorderPlacement(const URect & placement)
            {
                panel.children[0].placement = placement;
                panel.children[1].placement.*dim.u1 = placement.*dim.u0;
                panel.children[2].placement.*dim.u0 = placement.*dim.u1;
                panel.SetRect(panel.rect);
            }
        public:
            Dragger(Element & panel, const DimensionDesc & dim, const int2 & click) : panel(panel), initialPlacement(panel.children[0].placement), dim(dim), click(click.*dim.coord) {}
            void OnDrag(int2 newMouse) override
            {
                auto minA = panel.children[1].element->GetMinimumSize();
                auto minB = panel.children[2].element->GetMinimumSize();

                auto borderRect = initialPlacement.resolve(panel.rect);
                int delta = newMouse.*dim.coord - click;
                delta = std::max(delta, panel.rect.*dim.r0 + minA.*dim.coord - borderRect.*dim.r0);
                delta = std::min(delta, panel.rect.*dim.r1 - minB.*dim.coord - borderRect.*dim.r1);
                auto placement = initialPlacement;
                (placement.*dim.u0).b += delta;
                (placement.*dim.u1).b += delta;
                SetBorderPlacement(placement);
            }
            void OnRelease() override {}
            void OnCancel() override { SetBorderPlacement(initialPlacement); }
        };

        Element & panel;
        DimensionDesc dim;
    public:
        SplitterBorder(Element & panel, const DimensionDesc & dim) : panel(panel), dim(dim) {}
        Cursor GetCursor() const override { return dim.cursor; }
        DraggerPtr OnClick(const MouseEvent & e) override { return std::make_shared<Dragger>(panel, dim, e.cursor); }
    };

    Splitter::Splitter(ElementPtr panelA, ElementPtr panelB, Side sideB, int pixelsB)
    {
        const DimensionDesc dimX {&int2::x, &Rect::x0, &Rect::x1, &URect::x0, &URect::x1, Cursor::SizeWE};
        const DimensionDesc dimY {&int2::y, &Rect::y0, &Rect::y1, &URect::y0, &URect::y1, Cursor::SizeNS};
        auto dim = sideB == Left || sideB == Right ? dimX : dimY;
        auto border = std::make_shared<SplitterBorder>(*this, dim);

        URect placement = {{0,0},{0,0},{1,0},{1,0}}, placeA = placement, placeB = placement;
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

    Border::Border(int size, float offset, float width, float radius, NVGcolor border, NVGcolor background, ElementPtr inner) :
        size(size), offset(offset), width(width), radius(radius), border(border), background(background)
    {
        children.push_back({{{0,size},{0,size},{1,-size},{1,-size}},inner});
    }

    NVGcolor Border::OnDrawBackground(const DrawEvent & e) const
    {
        nvgBeginPath(e.vg);
        nvgRect(e.vg, rect.x0, rect.y0, rect.GetWidth(), rect.GetHeight());
	    nvgFillColor(e.vg, background);
	    nvgFill(e.vg);
        return background;
    }

    void Border::OnDrawForeground(const DrawEvent & e) const
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

    std::shared_ptr<Border> Border::CreateBigBorder(ElementPtr inner) { return std::make_shared<Border>(4, 1.5f, 2.0f, 4.5f, nvgRGBA(0,0,0,192), nvgRGBA(64,64,64,255), inner); }
    std::shared_ptr<Border> Border::CreateEditBorder(ElementPtr inner) { return std::make_shared<Border>(2, 0.5f, 1.0f, 2.5f, nvgRGBA(0,0,0,128), nvgRGBA(88,88,88,255), inner); }

    //////////
    // Menu //
    //////////

    class Menu::Barrier : public Element
    {
        std::vector<ElementPtr> popups;
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

        void ShowPopup(size_t level, ElementPtr popup)
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

        DraggerPtr OnClick(const MouseEvent & e) { HidePopups(); return {}; }
    };

    class MenuButton : public Element
    {
        std::function<void()> func;
        bool isPopup;
    public:
        MenuButton(const Font & font, const MenuItem & item, int offset, std::function<void()> func, bool isPopup) : func(func), isPopup(isPopup)
        {
            auto text = std::make_shared<Text>(font);
            text->isTransparent = true;
            text->text = item.label;
            children.push_back({{{0,offset},{0,2},{1,-offset},{1,-2}}, text});

            if(item.hotKey)
            {
                std::ostringstream ss;
                if(item.hotKeyMods & GLFW_MOD_CONTROL) ss << "Ctrl+";
                if(item.hotKeyMods & GLFW_MOD_SHIFT) ss << "Shift+";
                if(item.hotKeyMods & GLFW_MOD_ALT) ss << "Alt+";
                if(item.hotKeyMods & GLFW_MOD_SUPER) ss << "Super+";
                if(item.hotKey >= GLFW_KEY_A && item.hotKey <= GLFW_KEY_Z) ss << static_cast<char>('A' + item.hotKey - GLFW_KEY_A);
                else if(item.hotKey >= GLFW_KEY_0 && item.hotKey <= GLFW_KEY_9) ss << (item.hotKey - GLFW_KEY_0);
                else if(item.hotKey >= GLFW_KEY_F1 && item.hotKey <= GLFW_KEY_F12) ss << 'F' << (1 + item.hotKey - GLFW_KEY_F1);
                else switch(item.hotKey)
                {
                case GLFW_KEY_INSERT: ss << "Ins"; break;
                case GLFW_KEY_DELETE: ss << "Del"; break;
                default: throw std::runtime_error("Unsupported key!"); // TODO: Implement all keys
                }           

                text = std::make_shared<Text>(font);
                text->isTransparent = true;
                text->text = ss.str();
                children.push_back({{{1,-font.GetStringWidth(text->text)-offset},{0,2},{1,-offset},{1,-2}}, text});
            }
        }

        DraggerPtr OnClick(const MouseEvent & e) { func(); return {}; }

        NVGcolor MenuButton::OnDrawBackground(const DrawEvent & e) const
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

    std::weak_ptr<Element> Menu::MakePopup(size_t level, const Font & font, const std::vector<MenuItem> & items, float x, float y)
    {
        auto popup = std::make_shared<Element>();

        auto bar = barrier;
        URect placement = {{0,0}, {0,0}, {1,0}, {0,0}};
        for(auto & item : items)
        {
            placement.y0 = placement.y1;
            placement.y1.b += font.GetLineHeight() + 4;

            auto func = item.onClick;
            if(!item.children.empty())
            {
                auto childPopup = MakePopup(level + 1, font, item.children, x+150-2, y+placement.y0.b);
                func = [bar,childPopup,level]() { if(auto pop = childPopup.lock()) bar->ShowPopup(level, pop); };
            }
            else
            {
                auto f = item.onClick;
                func = [bar,f]() { bar->HidePopups(); f(); };
            }
            popup->AddChild(placement, std::make_shared<MenuButton>(font, item, 10, func, !item.children.empty()));
        }

        popup = std::make_shared<Border>(1, 0.5f, 1.0f, 0, nvgRGBA(0,0,0,255), nvgRGBA(64,64,64,255), popup);

        AddChild({{0,x}, {0,y}, {0,x+150}, {0,y+placement.y1.b+2}}, popup);
        popup->isVisible = false;
        return popup;
    }

    Menu::Menu(ElementPtr inner, const Font & font, const std::vector<MenuItem> & items)
    {
        float menuSize = 24;
        AddChild({{0,0}, {0,0}, {1,0}, {0,menuSize}}, std::make_shared<Fill>(nvgRGBA(32,32,32,255)));
        AddChild({{0,0}, {0,menuSize}, {1,0}, {1,0}}, inner);

        barrier = std::make_shared<Barrier>();
        AddChild({{0,0}, {0,0}, {1,0}, {1,0}}, barrier);

        auto bar = barrier;
        URect placement = {{0,0}, {0,0}, {0,0}, {0,menuSize}};
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

            AddChild(placement, std::make_shared<MenuButton>(font, item, 10, func, false));
        }

        std::reverse(children.begin()+3, children.end());
    }

    //////////////////////
    // DockingContainer //
    //////////////////////

    struct TornPanel : public gui::Element
    {
        DockingContainer & container;
        Window & window;
        const Font & font;
        std::string title;

        TornPanel::TornPanel(DockingContainer & container, Window & window, const Font & font, const std::string & title, gui::ElementPtr child) : container(container), window(window), font(font), title(title)
        {
            AddChild({{0,3},{0,font.GetLineHeight()+5},{1,-3},{1,-3}}, child);
        }

        NVGcolor TornPanel::OnDrawBackground(const DrawEvent & e) const override
        {
            nvgBeginPath(e.vg);
            nvgRect(e.vg, rect.x0+1, rect.y0+1, rect.GetWidth()-2, font.GetLineHeight()+2);
            nvgFillColor(e.vg, nvgRGBA(64,64,64,255));
            nvgFill(e.vg);

            nvgBeginPath(e.vg);
            nvgRect(e.vg, rect.x0+1, rect.y0 + font.GetLineHeight()+3, rect.GetWidth()-2, rect.GetHeight() - (font.GetLineHeight()+4));
            nvgFillColor(e.vg, nvgRGBA(48,48,48,255));
            nvgFill(e.vg);

            font.DrawString(rect.x0 + 2, rect.y0 + 2, nvgRGBA(255,255,255,255), title);

            nvgBeginPath(e.vg);
            nvgRect(e.vg, rect.x0+0.5f, rect.y0+0.5f, rect.GetWidth()-1, rect.GetHeight()-1);
            nvgStrokeColor(e.vg, nvgRGBA(255,255,255,255));
            nvgStrokeWidth(e.vg, 1);
            nvgStroke(e.vg);

            return nvgRGBA(48,48,48,255);
        }

        DraggerPtr TornPanel::OnClick(const MouseEvent & e) override
        {
            if(e.cursor.y >= rect.y0 + font.GetLineHeight()+3) return nullptr;

            class WindowDragger : public IDragger
            {
                TornPanel & panel;
                int2 oldMouse;
            public:
                WindowDragger(TornPanel & panel, int2 oldMouse) : panel(panel), oldMouse(oldMouse) {}
                void OnDrag(int2 newMouse) override
                {
                    panel.window.SetPos(panel.window.GetPos() + newMouse - oldMouse);
                    panel.container.PreviewDockAtScreenCoords(panel.window.GetPos() + oldMouse);
                }
                void OnRelease() override { panel.container.DockAtScreenCoords(panel.title, panel.children[0].element, panel.window.GetPos() + oldMouse); }
                void OnCancel() override { panel.container.CancelPreview(); }
            };

            return std::make_shared<WindowDragger>(*this, e.cursor);
        }
    };

    struct TearablePanel : public gui::Element
    {
        DockingContainer & container;
        const Font & font;
        std::string title;

        TearablePanel::TearablePanel(DockingContainer & container, const Font & font, const std::string & title, ElementPtr child) : container(container), font(font), title(title) 
        {
            AddChild({{0,2},{0,font.GetLineHeight()+4},{1,-2},{1,-2}}, child);
        }

        NVGcolor TearablePanel::OnDrawBackground(const DrawEvent & e) const override
        {
            if(children.empty()) return e.parent;

            nvgBeginPath(e.vg);
            nvgRect(e.vg, rect.x0, rect.y0, rect.GetWidth(), font.GetLineHeight()+2);
            nvgFillColor(e.vg, nvgRGBA(64,64,64,255));
            nvgFill(e.vg);

            nvgBeginPath(e.vg);
            nvgRect(e.vg, rect.x0, rect.y0 + font.GetLineHeight()+2, rect.GetWidth(), rect.GetHeight() - (font.GetLineHeight()+2));
            nvgFillColor(e.vg, nvgRGBA(48,48,48,255));
            nvgFill(e.vg);

            font.DrawString(rect.x0 + 1, rect.y0 + 1, nvgRGBA(255,255,255,255), title);

            return nvgRGBA(48,48,48,255);
        }

        DraggerPtr TearablePanel::OnClick(const MouseEvent & e) override
        {
            if(e.cursor.y >= rect.y0 + font.GetLineHeight()+2 || children.empty()) return nullptr;

            auto win = container.Tear(*this);
            auto child = children[0].element;
            children.clear();
            auto torn = std::make_shared<TornPanel>(container, *win, font, title, child);
            win->SetGuiRoot(torn, font, std::vector<MenuItem>());

            class WindowDragger : public IDragger
            {
                TornPanel & torn;
                int2 oldMouse;
            public:
                WindowDragger(TornPanel & torn, int2 oldMouse) : torn(torn), oldMouse(oldMouse) {}
                void OnDrag(int2 newMouse) override
                {
                    torn.window.SetPos(torn.window.GetPos() + newMouse - oldMouse);
                    oldMouse = newMouse;
                    torn.container.PreviewDockAtWindowCoords(oldMouse);
                }
                void OnRelease() override { torn.container.DockAtWindowCoords(torn.title, torn.children[0].element, oldMouse); }
                void OnCancel() override { torn.container.CancelPreview(); }
            };

            return std::make_shared<WindowDragger>(*torn, e.cursor);
        }
    };

    void DockingContainer::RedrawAll()
    { 
        for(auto & panel : panels)
        {
            if(panel.window)
            {
                if(panel.window->ShouldClose()) panel.window.reset();
                else panel.window->Redraw();
            }
        }
    }

    void DockingContainer::SetPrimaryElement(ElementPtr element)
    {
        children.clear();
        panels.clear();
        AddChild({{0,0},{0,0},{1,0},{1,0}}, element);
        AddChild({{0,0},{0,0},{0,0},{0,0}}, std::make_shared<Fill>(nvgRGBA(64,64,64,128)));
        panels.push_back({element, element, nullptr});
        SetRect(rect);
    }

    void DockingContainer::PreviewDockAtScreenCoords(const int2 & point) { PreviewDockAtWindowCoords(point - mainWindow.GetPos()); }

    void DockingContainer::PreviewDockAtWindowCoords(const int2 & point)
    {
        for(auto & panel : panels)
        {
            if(panel.window) continue;
            if(panel.panel->rect.x0 <= point.x && panel.panel->rect.y0 <= point.y && point.x < panel.panel->rect.x1 && point.y < panel.panel->rect.y1)
            {
                auto prect = panel.panel->rect;
                float dl = point.x - prect.x0;
                float dt = point.y - prect.y0;
                float dr = prect.x1 - point.x;
                float db = prect.y1 - point.y;

                if(dl <= dt && dl <= dr && dl <= db) children[1].placement = {{0,prect.x0-rect.x0},{0,prect.y0-rect.y0},{0,prect.x0-rect.x0+200},{0,prect.y1-prect.y0}};
                else if(dt <= dr && dt <= db) children[1].placement = {{0,prect.x0-rect.x0},{0,prect.y0-rect.y0},{0,prect.x1-rect.x0},{0,prect.y0-prect.y0+200}};
                else if(dr < db) children[1].placement = {{0,prect.x1-rect.x0-200},{0,prect.y0-rect.y0},{0,prect.x1-rect.x0},{0,prect.y1-prect.y0}};
                else children[1].placement = {{0,prect.x0-rect.x0},{0,prect.y1-rect.y0-200},{0,prect.x1-rect.x0},{0,prect.y1-prect.y0}};
                SetRect(rect);
                return;
            }
        }
        CancelPreview();
    }

    void DockingContainer::CancelPreview()
    {
        children[1].placement = {{0,0},{0,0},{0,0},{0,0}};
        SetRect(rect);
    }

    bool DockingContainer::DockElement(ElementPtr & candidate, Element & parent, const std::string & panelTitle, ElementPtr element, Splitter::Side side, int pixels)
    {
        if(candidate.get() == &parent || candidate->children.size() == 1 && candidate->children[0].element.get() == &parent)
        {
            auto panel = std::make_shared<TearablePanel>(*this, font, panelTitle, element);
            candidate = std::make_shared<Splitter>(candidate, panel, side, pixels);

            for(auto & p : panels)
            {
                if(p.content == element)
                {
                    p.panel = panel;
                    p.window->Close();
                    return true;
                }
            }

            panels.push_back({panel, element, nullptr});
            return true;
        }

        for(auto & child : candidate->children)
        {
            if(DockElement(child.element, parent, panelTitle, element, side, pixels)) return true;
        }

        return false;
    }

    void DockingContainer::Dock(Element & parent, const std::string & panelTitle, ElementPtr element, Splitter::Side side, int pixels)
    {
        if(DockElement(children[0].element, parent, panelTitle, element, side, pixels))
        {
            SetRect(rect);
        }
    }

    void DockingContainer::DockAtScreenCoords(const std::string & title, ElementPtr element, const int2 & coords) { DockAtWindowCoords(title, element, coords - mainWindow.GetPos()); }

    void DockingContainer::DockAtWindowCoords(const std::string & title, ElementPtr element, const int2 & coords)
    {
        for(auto & panel : panels)
        {
            if(panel.panel->rect.x0 <= coords.x && panel.panel->rect.y0 <= coords.y && coords.x < panel.panel->rect.x1 && coords.y < panel.panel->rect.y1)
            {
                auto prect = panel.panel->rect;
                float dl = coords.x - prect.x0;
                float dt = coords.y - prect.y0;
                float dr = prect.x1 - coords.x;
                float db = prect.y1 - coords.y;

                if(dl <= dt && dl <= dr && dl <= db) Dock(*panel.panel, title, element, Splitter::Left, 200);
                else if(dt <= dr && dt <= db) Dock(*panel.panel, title, element, Splitter::Top, 200);
                else if(dr < db) Dock(*panel.panel, title, element, Splitter::Right, 200);
                else Dock(*panel.panel, title, element, Splitter::Bottom, 200);
                break;
            }
        }      
        CancelPreview();
    }

    static ElementPtr TearElement(ElementPtr & candidate, Element & element)
    {
        if(candidate->children.size() != 3) return nullptr; // Can only tear from splitters
        
        if(candidate->children[1].element.get() == &element)
        {
            auto tear = candidate->children[1].element;
            candidate = candidate->children[2].element;
            return tear;
        }

        if(candidate->children[2].element.get() == &element)
        {
            auto tear = candidate->children[2].element;
            candidate = candidate->children[1].element;
            return tear;
        }

        if(auto tear = TearElement(candidate->children[1].element, element)) return tear;
        if(auto tear = TearElement(candidate->children[2].element, element)) return tear;
        return nullptr;
    }

    std::shared_ptr<Window> DockingContainer::Tear(Element & element)
    {
        auto tear = TearElement(children[0].element, element);
        SetRect(rect);

        auto pos = mainWindow.GetPos();
        pos.x += element.rect.x0 - 1;
        pos.y += element.rect.y0 - 1;

        glfwWindowHint(GLFW_DECORATED, 0);
        auto win = std::make_shared<Window>("", element.rect.GetWidth()+2, element.rect.GetHeight()+2, &mainWindow, pos);
        glfwDefaultWindowHints();

        for(auto & panel : panels)
        {
            if(panel.panel.get() == &element)
            {
                panel.window = win;
            }
        }
        return win;
    }
}