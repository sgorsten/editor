#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "gui.h"
#include "engine/font.h"
#include "engine/utf8.h"

#include <algorithm>
#include <sstream>

struct NVGcontext;

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
    gui::DraggerPtr dragger;
    int lastX, lastY;

    // Rendering helpers
    NVGcontext * vg;

    void OnClick(gui::ElementPtr clickfocus, int mouseX, int mouseY, bool holdingShift);
public:
    Window(const char * title, int width, int height);
    ~Window();

    NVGcontext * GetNanoVG() const { return vg; }

    bool ShouldClose() const { return !!glfwWindowShouldClose(window); }

    void RefreshLayout();
    void SetGuiRoot(gui::ElementPtr element);
    void Redraw();
};

#endif