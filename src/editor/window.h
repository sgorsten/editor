#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "gui.h"
#include "engine/font.h"
#include "engine/utf8.h"

#include <algorithm>
#include <sstream>

struct NVGcontext;
struct MenuItem;

class Window
{
    struct Shortcut { int mods, key; std::function<void()> onInvoke; };

    // Core window state
    GLFWwindow * window;
    GLFWcursor * cursors[4];
    gui::ElementPtr root;
    std::vector<Shortcut> shortcuts;

    // Layout, needs to be regenerated whenever window is resized or gui changes
    int width, height;
    std::vector<gui::ElementPtr> tabStops;
    
    // Text manipulation logic
    gui::ElementPtr mouseover, focus;
    gui::DraggerPtr dragger;
    int lastX, lastY;

    // Rendering helpers
    NVGcontext * vg;

    void CancelDrag();
    void TabTo(gui::ElementPtr element);
    void GatherShortcuts(const MenuItem & item);
public:
    Window(const char * title, int width, int height);
    ~Window();

    NVGcontext * GetNanoVG() const { return vg; }

    bool ShouldClose() const { return !!glfwWindowShouldClose(window); }

    void RefreshLayout();
    void SetGuiRoot(gui::ElementPtr element, const Font & menuFont, const std::vector<MenuItem> & menuItems);
    void Redraw();
};

#endif