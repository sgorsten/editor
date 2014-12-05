#ifndef EDITOR_GUI_H
#define EDITOR_GUI_H

#include "engine/linalg.h"
#include "engine/gl.h"
#include "nanovg.h"

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

class Font;

namespace gl { class Texture; }

struct GLFWwindow;
struct NVGcontext;

class Window;

namespace gui
{
    struct IDragger
    {
        virtual void OnDrag(int2 newMouse) = 0;
        virtual bool OnKey(int key, int action, int mods) { return false; }
        virtual void OnRelease() = 0;
        virtual void OnCancel() = 0;
    };
    typedef std::shared_ptr<IDragger> DraggerPtr;

    enum class Cursor { Arrow, IBeam, SizeNS, SizeWE };

    struct Rect
    {
        int x0,y0,x1,y1;
        float GetCenterX() const { return (x0+x1)*0.5f; }
        float GetCenterY() const { return (y0+y1)*0.5f; }
        int GetWidth() const { return x1-x0; }
        int GetHeight() const { return y1-y0; }
        float GetAspect() const { return (float)GetWidth()/GetHeight(); }
    };

    struct UCoord
    {
        float a,b;
        int resolve(int lo, int hi) const { return lo + static_cast<int>(round(a*(hi-lo) + b)); }
    };

    struct URect
    {
        UCoord x0,y0,x1,y1;
        Rect resolve(Rect r) const { return {x0.resolve(r.x0,r.x1), y0.resolve(r.y0,r.y1), x1.resolve(r.x0,r.x1), y1.resolve(r.y0,r.y1)}; }
    };

    struct Child;

    struct MouseEvent
    {
        int2 cursor;
        int button;
        int mods;

        bool IsShiftHeld() const { return !!(mods & GLFW_MOD_SHIFT); }
        bool IsControlHeld() const { return !!(mods & GLFW_MOD_CONTROL); }
        bool IsAltHeld() const { return !!(mods & GLFW_MOD_ALT); }
    };

    struct DrawEvent
    {
        NVGcontext * vg;    // Current NanoVG context, can be used for drawing
        NVGcolor parent;    // NanoVG color of parent element, can be used to draw masks/fades over top of current element
        bool hasFocus;      // True if this was the last element the user clicked on
        bool isPressed;     // True if the user clicked on this element and has not yet released the mouse
        bool isMouseOver;   // True if the mouse is presently over this element
    };

    typedef std::shared_ptr<struct Element> ElementPtr;

    struct Element
    {
        struct Child
        {
            URect placement;
            ElementPtr element;
        };

        Rect                                                rect;
        std::vector<Child>                                  children;
        bool                                                isVisible;
        bool                                                isTransparent;  // If true, this element will ignore mouse events
        int2                                                minimumSize;    // Minimum size that this element should occupy, independent of children
        
                                                            Element();

        int2                                                GetMinimumSize() const; // Compute minimum size for this element, inclusive of children

        void                                                AddChild(const URect & placement, ElementPtr child);
        void                                                SetRect(const Rect & rect);

        virtual bool                                        IsTabStop() const { return false; }
        virtual Cursor                                      GetCursor() const { return Cursor::Arrow; }
        virtual NVGcolor                                    OnDrawBackground(const DrawEvent & e) const { return e.parent; } // Draw contents before children
        virtual void                                        OnDrawForeground(const DrawEvent & e) const {} // Draw contents after children

        virtual void                                        OnChar(uint32_t codepoint) {}
        virtual bool                                        OnKey(GLFWwindow * window, int key, int action, int mods) { return false; }
        virtual DraggerPtr                                  OnClick(const MouseEvent & e) { return nullptr; } // If a dragger is returned, it will take focus until user releases mouse or hits "escape"
        virtual void                                        OnTab() {}
    };

    // Element which fills its client area with a solid color
    struct Fill : public gui::Element
    {
        NVGcolor color;
        Fill(NVGcolor color) : color(color) {}
        NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override;
    };

    // Element providing selectable, editable text
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
        bool OnKey(GLFWwindow * window, int key, int action, int mods) override;
        gui::DraggerPtr OnClick(const gui::MouseEvent & e) override;
        void OnTab() override { SelectAll(); }
        NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override;

        std::function<void(const std::string &)> onEdit;
    };

    // User-draggable boundary between two child regions
    class Splitter : public gui::Element
    {
    public:
        enum Side { Left, Top, Right, Bottom };

        Splitter(gui::ElementPtr panelA, gui::ElementPtr panelB, Side sideB, int pixelsB);
    };

    // Decoration around a client area
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

    // List box, allowing for user selection
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

    struct MenuItem
    {
        // TODO: Icon, hotkeys, etc
        bool isEnabled;                 // If true, this menu item can be clicked on
        std::string label;              // The string that should be displayed for this item
        std::vector<MenuItem> children; // If nonempty, clicking on this item will open a popup menu
        std::function<void()> onClick;  // If bound, clicking on this item will invoke this function
        int hotKeyMods, hotKey;

        MenuItem() : isEnabled(true), hotKeyMods(), hotKey() {}
        MenuItem(const std::string & label, std::function<void()> onClick, int hotKeyMods=0, int hotKey=0) : isEnabled(true), label(label), onClick(onClick), hotKeyMods(hotKeyMods), hotKey(hotKey) {}

        static MenuItem Popup(std::string label, std::vector<MenuItem> children) { auto r = MenuItem(); r.label = move(label); r.children = move(children); return r; }
    };

    // Supports a menu bar with drop-down menus
    class Menu : public gui::Element
    {
        class Barrier;
        std::shared_ptr<Barrier> barrier;
        std::weak_ptr<gui::Element> MakePopup(size_t level, const Font & font, const std::vector<MenuItem> & items, float x, float y);
    public:
        Menu(gui::ElementPtr inner, const Font & font, const std::vector<MenuItem> & items);
    };

    // Allow for named panels to be "docked", and rearranged by the user
    class DockingContainer : public gui::Element
    {
        struct PanelState
        {
            ElementPtr panel, content;
            std::shared_ptr<Window> window; // If empty, panel is currently docked
        };

        Window & mainWindow;
        const Font & font;
        std::vector<PanelState> panels;

        /*std::vector<std::shared_ptr<Window>> tornWindows;
        std::vector<Element *> dockedPanels;*/

        bool DockElement(ElementPtr & candidate, Element & parent, const std::string & panelTitle, ElementPtr element, Splitter::Side side, int pixels);
    public:
        DockingContainer(Window & mainWindow, const Font & font) : mainWindow(mainWindow), font(font) {}

        void PreviewDockAtScreenCoords(const int2 & point);
        void PreviewDockAtWindowCoords(const int2 & point);
        void CancelPreview();

        void RedrawAll();

        void SetPrimaryElement(ElementPtr element);
        void Dock(Element & parent, const std::string & panelTitle, ElementPtr element, Splitter::Side side, int pixels);

        void DockAtScreenCoords(const std::string & title, ElementPtr element, const int2 & coords);
        void DockAtWindowCoords(const std::string & title, ElementPtr element, const int2 & coords);

        std::shared_ptr<Window> Tear(Element & element);
    };
}

#endif