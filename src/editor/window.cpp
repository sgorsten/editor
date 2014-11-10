#include "window.h"
#include "engine/gl.h"

#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

gui::ElementPtr GetElement(const gui::ElementPtr & element, int x, int y)
{
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

void Window::OnClick(gui::ElementPtr clickFocus, int mouseX, int mouseY, bool holdingShift)
{
    if(dragger)
    {
        dragger->OnRelease();
        dragger = nullptr;
    }
    
    if(focus != clickFocus)
    {
        focus = clickFocus;
    }    

    if(focus)
    {
        dragger = focus->OnClick({{mouseX, mouseY}, holdingShift});
    }
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
        auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        if(action == GLFW_PRESS)
        {
            double cx, cy;
            glfwGetCursorPos(window, &cx, &cy);
            if(auto elem = GetElement(w->root, cx, cy))
            {
                w->OnClick(elem, cx, cy, (mods & GLFW_MOD_SHIFT) != 0);
            }
            else w->OnClick(nullptr, 0, 0, false);
        }
        if(action == GLFW_RELEASE)
        {
            if(w->dragger)
            {
                w->dragger->OnRelease();
                w->dragger.reset();
            }
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow * window, double cx, double cy)
    {
        int x = static_cast<int>(cx), y = static_cast<int>(cy);
        auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        if(w->dragger) w->dragger->OnDrag(int2(cx,cy));
        else
        {
            auto elem = GetElement(w->root, x, y);
            glfwSetCursor(window, w->cursors[(int)elem->cursor]);
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
            if(w->dragger)
            {
                w->dragger->OnCancel();
                w->dragger.reset();
            }
            return;
        case GLFW_KEY_TAB: // Tab iterates through editable controls
            if(w->focus)
            {
                auto focusIt = std::find(begin(w->tabStops), end(w->tabStops), w->focus);
                if(focusIt == end(w->tabStops) || ++focusIt == end(w->tabStops)) w->OnClick(nullptr, 0, 0, false);
                else
                {
                    w->OnClick(*focusIt, 0, 0, false);
                    //w->SelectAll();
                }
            }
            if(!w->focus)
            {
                if(!w->tabStops.empty())
                {
                    w->OnClick(w->tabStops.front(), 0, 0, false);
                    //w->SelectAll();
                }
            }
            return;
        }

        // All remaining keys apply to current "focus" element
        if(w->focus) w->focus->OnKey(window, key, action, mods);
    });
}

Window::~Window()
{
    nvgDeleteGL3(vg);
    glfwDestroyWindow(window);
}

static void CollectTabStops(std::vector<gui::ElementPtr> & tabStops, const gui::ElementPtr & elem)
{
    //if(elem->text.isEditable) tabStops.push_back(elem);
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

static void DrawElement(NVGcontext * vg, const gui::Element & elem, const gui::Element * focus, NVGcolor parentBackground)
{
    gui::DrawEvent e = {vg,parentBackground};
    e.hasFocus = &elem == focus;

    nvgSave(vg);
	nvgScissor(vg, elem.rect.x0, elem.rect.y0, elem.rect.GetWidth(), elem.rect.GetHeight());
    NVGcolor background = elem.OnDrawBackground(e);
    for(const auto & child : elem.children) DrawElement(vg, *child.element, focus, background);
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

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glOrtho(0, width, height, 0, -1, +1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    nvgBeginFrame(vg, width, height, 1.0f);

    DrawElement(vg, *root, focus.get(), nvgRGBA(0,0,0,0));
    nvgEndFrame(vg);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();

    glfwSwapBuffers(window);
}