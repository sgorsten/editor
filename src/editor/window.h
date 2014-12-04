#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "gui.h"
#include "engine/font.h"
#include "engine/utf8.h"

#include <algorithm>
#include <sstream>

struct NVGcontext;
namespace gui { struct MenuItem; }

struct Context
{
    GLFWwindow * mainWindow;
    GLFWcursor * cursors[4];
    NVGcontext * vg;

    Context();
    Context(GLFWwindow * mainWindow);
    ~Context();
};

class Window
{
    struct Shortcut { int mods, key; std::function<void()> onInvoke; };

    // Core window state
    std::shared_ptr<Context> context;
    GLFWwindow * window;
    gui::ElementPtr root;
    std::vector<Shortcut> shortcuts;

    // Layout, needs to be regenerated whenever window is resized or gui changes
    int width, height;
    std::vector<gui::ElementPtr> tabStops;
    
    // Text manipulation logic
    gui::ElementPtr mouseover, focus;
    gui::DraggerPtr dragger;
    int lastX, lastY;

    void CancelDrag();
    void TabTo(gui::ElementPtr element);
    void GatherShortcuts(const gui::MenuItem & item);
public:
    Window(const char * title, int width, int height, const Window * parent = nullptr);
    ~Window();

    bool IsMainWindow() const { return window == context->mainWindow; }
    NVGcontext * GetNanoVG() const { return context->vg; }

    bool ShouldClose() const { return !!glfwWindowShouldClose(window); }

    void RefreshLayout();
    void SetGuiRoot(gui::ElementPtr element, const Font & menuFont, const std::vector<gui::MenuItem> & menuItems);
    void Redraw();
};

#endif