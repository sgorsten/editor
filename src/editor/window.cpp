#include "window.h"
#include "engine/gl.h"

#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

gui::ElementPtr GetElement(const gui::ElementPtr & element, int x, int y)
{
    if(!element->isVisible || element->isTransparent) return nullptr;

    for(auto it = element->children.rbegin(), end = element->children.rend(); it != end; ++it)
    {
        if(auto elem = GetElement(it->element, x, y))
        {
            return elem;
        }
    }

    if(x >= element->rect.x0 && y >= element->rect.y0 && x < element->rect.x1 && y < element->rect.y1)
    {
        return element;
    }

    return nullptr;
}

void Window::CancelDrag()
{
    if(dragger)
    {
        dragger->OnCancel();
        dragger = nullptr;
    }
}

void Window::TabTo(gui::ElementPtr element)
{
    CancelDrag();

    focus = element;
    if(focus) focus->OnTab();
}

Window::Window(const char * title, int width, int height) : window(), width(width), height(height), focus(), vg()
{
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK) throw std::runtime_error("Could not init glew");

    vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
	if (vg == NULL) throw std::runtime_error("Could not init nanovg");

    struct Color { uint8_t r,g,b,a; } W{255,255,255,255}, B{0,0,0,255}, _{0,0,0,0};
    Color arrow[] = {
        B,_,_,_,_,_,_,_,_,_,_,_,  
        B,B,_,_,_,_,_,_,_,_,_,_,
        B,W,B,_,_,_,_,_,_,_,_,_,  
        B,W,W,B,_,_,_,_,_,_,_,_,
        B,W,W,W,B,_,_,_,_,_,_,_,  
        B,W,W,W,W,B,_,_,_,_,_,_,
        B,W,W,W,W,W,B,_,_,_,_,_,  
        B,W,W,W,W,W,W,B,_,_,_,_,
        B,W,W,W,W,W,W,W,B,_,_,_,  
        B,W,W,W,W,W,W,W,W,B,_,_,
        B,W,W,W,W,W,W,W,W,W,B,_,  
        B,W,W,W,W,W,W,W,W,W,W,B,
        B,W,W,W,W,W,W,B,B,B,B,B,
        B,W,W,W,B,W,W,B,_,_,_,_,
        B,W,W,B,_,B,W,W,B,_,_,_,
        B,W,B,_,_,B,W,W,B,_,_,_,
        B,B,_,_,_,_,B,W,W,B,_,_,
        _,_,_,_,_,_,B,W,W,B,_,_,
        _,_,_,_,_,_,_,B,B,_,_,_,
    }, ibeam[] = {
        B,B,B,_,B,B,B,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        _,_,_,B,_,_,_,
        B,B,B,_,B,B,B,
    }, sizens[] = {
        _,_,_,_,B,_,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,B,W,W,W,B,_,_,
        _,B,W,W,W,W,W,B,_,
        B,W,W,W,W,W,W,W,B,
        B,B,B,B,W,B,B,B,B,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,B,W,B,_,_,_,
        B,B,B,B,W,B,B,B,B,
        B,W,W,W,W,W,W,W,B,
        _,B,W,W,W,W,W,B,_,
        _,_,B,W,W,W,B,_,_,
        _,_,_,B,W,B,_,_,_,
        _,_,_,_,B,_,_,_,_,
    }, sizewe[] = {
        _,_,_,_,B,B,_,_,_,_,_,_,_,_,_,_,_,B,B,_,_,_,_,
        _,_,_,B,W,B,_,_,_,_,_,_,_,_,_,_,_,B,W,B,_,_,_,
        _,_,B,W,W,B,_,_,_,_,_,_,_,_,_,_,_,B,W,W,B,_,_,
        _,B,W,W,W,B,B,B,B,B,B,B,B,B,B,B,B,B,W,W,W,B,_,
        B,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,B,
        _,B,W,W,W,B,B,B,B,B,B,B,B,B,B,B,B,B,W,W,W,B,_,
        _,_,B,W,W,B,_,_,_,_,_,_,_,_,_,_,_,B,W,W,B,_,_,
        _,_,_,B,W,B,_,_,_,_,_,_,_,_,_,_,_,B,W,B,_,_,_,
        _,_,_,_,B,B,_,_,_,_,_,_,_,_,_,_,_,B,B,_,_,_,_,
    };
    GLFWimage image = {12,19,&arrow[0].r};
    cursors[(int)gui::Cursor::Arrow] = glfwCreateCursor(&image, 0, 0);
    image = {7,16,&ibeam[0].r};
    cursors[(int)gui::Cursor::IBeam] = glfwCreateCursor(&image, 3, 8);
    image = {9,23,&sizens[0].r};
    cursors[(int)gui::Cursor::SizeNS] = glfwCreateCursor(&image, 4, 11);
    image = {23,9,&sizewe[0].r};
    cursors[(int)gui::Cursor::SizeWE] = glfwCreateCursor(&image, 11, 4);

    glfwSetWindowSizeCallback(window, [](GLFWwindow * window, int width, int height)
    {
        reinterpret_cast<Window *>(glfwGetWindowUserPointer(window))->RefreshLayout();
    });

    glfwSetWindowRefreshCallback(window, [](GLFWwindow * window)
    {
        reinterpret_cast<Window *>(glfwGetWindowUserPointer(window))->Redraw();
    });

    glfwSetCharCallback(window, [](GLFWwindow * window, unsigned int codepoint)
    {
        auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        if(w->focus) w->focus->OnChar(codepoint);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow * window, int button, int action, int mods)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        if(action == GLFW_PRESS)
        {
            w->CancelDrag();

            w->focus = w->mouseover;
            if(w->focus) w->dragger = w->focus->OnClick({{(int)xpos, (int)ypos}, button, mods});
        }
        if(action == GLFW_RELEASE)
        {
            if(w->dragger)
            {
                w->dragger->OnRelease();
                w->dragger.reset();
            }

            w->mouseover = GetElement(w->root, (int)xpos, (int)ypos);
            glfwSetCursor(window, w->cursors[(int)(w->mouseover ? w->mouseover->GetCursor() : gui::Cursor::Arrow)]);
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow * window, double cx, double cy)
    {
        int x = static_cast<int>(cx), y = static_cast<int>(cy);
        auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        if(w->dragger) w->dragger->OnDrag(int2(cx,cy));
        else
        {
            w->mouseover = GetElement(w->root, x, y);
            glfwSetCursor(window, w->cursors[(int)(w->mouseover ? w->mouseover->GetCursor() : gui::Cursor::Arrow)]);
        }
        w->lastX = x;
        w->lastY = y;
    });

    glfwSetKeyCallback(window, [](GLFWwindow * window, int key, int scancode, int action, int mods)
    {
        auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

        // Handle certain global UI keys
        if(action != GLFW_RELEASE) switch(key)
        {
        case GLFW_KEY_ESCAPE: // Escape cancels the current dragger
            w->CancelDrag();
            return;
        case GLFW_KEY_TAB: // Tab iterates through editable controls
            if(w->focus)
            {
                auto focusIt = std::find(begin(w->tabStops), end(w->tabStops), w->focus);
                if(focusIt == end(w->tabStops) || ++focusIt == end(w->tabStops)) w->TabTo(nullptr);
                else w->TabTo(*focusIt);
            }
            if(!w->focus && !w->tabStops.empty()) w->TabTo(w->tabStops.front());
            return;
        }

        if(w->dragger && w->dragger->OnKey(key, action, mods)) return; // If one is present, a dragger can consume keystrokes
        if(w->focus) w->focus->OnKey(window, key, action, mods); // All remaining keys apply to current "focus" element
    });
}

Window::~Window()
{
    nvgDeleteGL3(vg);
    glfwDestroyWindow(window);
}

static void CollectTabStops(std::vector<gui::ElementPtr> & tabStops, const gui::ElementPtr & elem)
{
    if(elem->IsTabStop()) tabStops.push_back(elem);
    for(auto & child : elem->children) CollectTabStops(tabStops, child.element);
}

void Window::RefreshLayout()
{
    tabStops.clear();
    glfwGetWindowSize(window, &width, &height);
    if(!root) return;
    root->SetRect({0,0,width,height});
    CollectTabStops(tabStops, root);
}

void Window::SetGuiRoot(gui::ElementPtr element)
{
    root = element; 
    RefreshLayout();
}

static void DrawElement(NVGcontext * vg, const gui::Element & elem, const gui::Element * mouseover, const gui::Element * focus, NVGcolor parentBackground)
{
    if(!elem.isVisible) return;
    gui::DrawEvent e = {vg,parentBackground};
    e.hasFocus = &elem == focus;
    e.isMouseOver = &elem == mouseover;

    nvgSave(vg);
	nvgScissor(vg, elem.rect.x0, elem.rect.y0, elem.rect.GetWidth(), elem.rect.GetHeight());
    NVGcolor background = elem.OnDrawBackground(e);
    for(const auto & child : elem.children) DrawElement(vg, *child.element, mouseover, focus, background);
    elem.OnDrawForeground(e);
    nvgRestore(vg);
}

void Window::Redraw()
{
    glfwMakeContextCurrent(window);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(0, 0, width, height);
    glClearColor(0,0,1,0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg, width, height, 1.0f);
    DrawElement(vg, *root, mouseover.get(), focus.get(), nvgRGBA(0,0,0,0));
    nvgEndFrame(vg);

    glPopAttrib();
    glfwSwapBuffers(window);
}