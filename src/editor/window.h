#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "gui.h"
#include "font.h"
#include "utf8.h"

#include <algorithm>
#include <sstream>

class Window
{
    // Core window state
    GLFWwindow * window;
    GLFWcursor * cursors[4];
    gui::ElementPtr root;

    // Layout, needs to be regenerated whenever window is resized or gui changes
    int width, height;
    std::vector<gui::ElementPtr> tabStops;
    
    // Text manipulation logic
    gui::ElementPtr focus;
    size_t cursor, mark;
    bool isSelecting;
    int lastX, lastY;

    const char * GetFocusText() const { return focus ? focus->text.text.data() : nullptr; }
    size_t GetFocusTextSize() const { return focus ? focus->text.text.size() : 0; }   
    size_t GetSelectionLeftIndex() const { return std::min(cursor, mark); }
    size_t GetSelectionRightIndex() const { return std::max(cursor, mark); }
    std::string GetSelectionText() const { return std::string(GetFocusText() + GetSelectionLeftIndex(), GetFocusText() + GetSelectionRightIndex()); }

    void OnEdit() { if(focus->onEdit) focus->onEdit(focus->text.text); }
    void RemoveSelection();
    void Insert(const char * text);
    void MoveSelectionCursor(int newCursor, bool holdingShift);
    void SelectAll();
    void OnClick(gui::ElementPtr clickfocus, int mouseX, int mouseY, bool holdingShift);
public:
    Window(const char * title, int width, int height);
    ~Window();

    bool ShouldClose() const { return !!glfwWindowShouldClose(window); }

    void RefreshLayout();
    void SetGuiRoot(gui::ElementPtr element);
    void Redraw();
};

#endif