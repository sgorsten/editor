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

void Window::OnClick(gui::ElementPtr clickfocus, int mouseX, int mouseY, bool holdingShift)
{
    if(clickfocus != focus)
    {
        focus = clickfocus;
        isSelecting = false;
    }
    if(focus && focus->text.font) MoveSelectionCursor(focus->text.font->GetUnitIndex(focus->text.text, mouseX - focus->rect.x0), holdingShift);
    if(focus && focus->onClick) focus->onClick(mouseX, mouseY);
}

void Window::MoveSelectionCursor(int newCursor, bool holdingShift)
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

void Window::SelectAll()
{
    if(!focus) return;
    isSelecting = true;
    mark = 0;
    cursor = GetFocusTextSize();
}

void Window::RemoveSelection()
{
    if(!isSelecting || !focus || !focus->text.isEditable) return;
    auto left = GetSelectionLeftIndex(), right = GetSelectionRightIndex();
    focus->text.text.erase(left, right-left);
    cursor = left;
    isSelecting = false;
    OnEdit();
}

void Window::Insert(const char * text)
{
    if(!focus->text.isEditable) return;
    focus->text.text.insert(cursor, text);
    cursor += strlen(text);
    OnEdit();
}

Window::Window(const char * title, int width, int height) : window(), width(width), height(height), focus(), isSelecting(), vg()
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
        if(w->focus)
        {
            if(w->isSelecting) w->RemoveSelection();
            w->Insert(utf8::units(codepoint).data());
        }
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
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow * window, double cx, double cy)
    {
        int x = static_cast<int>(cx), y = static_cast<int>(cy);
        auto w = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            if(w->focus)
            {
                if(w->focus->text.font)
                {
                    w->MoveSelectionCursor(w->focus->text.font->GetUnitIndex(w->focus->text.text, x - w->focus->rect.x0), true);
                }
                else if(w->focus->onDrag)
                {
                   w->focus->onDrag(x - w->lastX, y - w->lastY);
                }
            }
        }
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

        // First handle tabbing behavior
        if(key == GLFW_KEY_TAB)
        {
            if(action == GLFW_RELEASE) return;
            if(w->focus)
            {
                auto focusIt = std::find(begin(w->tabStops), end(w->tabStops), w->focus);
                if(focusIt == end(w->tabStops) || ++focusIt == end(w->tabStops)) w->OnClick(nullptr, 0, 0, false);
                else
                {
                    w->OnClick(*focusIt, 0, 0, false);
                    w->SelectAll();
                }
            }
            if(!w->focus)
            {
                if(!w->tabStops.empty())
                {
                    w->OnClick(w->tabStops.front(), 0, 0, false);
                    w->SelectAll();
                }
            }
            return;
        }

        // All remaining keys apply to current "focus" element
        if(!w->focus) return;

        // If element is editable, keypressed apply to text editing behavior only
        if(w->focus->text.isEditable)
        {
            if(action == GLFW_RELEASE) return;
            if(mods & GLFW_MOD_CONTROL) switch(key)
            {
            case GLFW_KEY_A:
                w->SelectAll();
                break;
            case GLFW_KEY_X:
                if(w->isSelecting)
                {
                    auto ct = w->GetSelectionText();
                    glfwSetClipboardString(window, ct.c_str());
                    w->RemoveSelection();
                }
                break;
            case GLFW_KEY_C:
                if(w->isSelecting)
                {
                    auto ct = w->GetSelectionText();
                    glfwSetClipboardString(window, ct.c_str());
                }
                break;               
            case GLFW_KEY_V:
                if(auto ct = glfwGetClipboardString(window))
                {
                    w->RemoveSelection();
                    w->Insert(ct);
                }
                break;
            }
                    
            switch(key)
            {
            case GLFW_KEY_LEFT:
                if(w->cursor > 0) w->MoveSelectionCursor(utf8::prev(w->GetFocusText() + w->cursor) - w->GetFocusText(), (mods & GLFW_MOD_SHIFT) != 0);
                break;
            case GLFW_KEY_RIGHT: 
                if(w->cursor < w->GetFocusTextSize()) w->MoveSelectionCursor(w->cursor = utf8::next(w->GetFocusText() + w->cursor) - w->GetFocusText(), (mods & GLFW_MOD_SHIFT) != 0);
                break;
            case GLFW_KEY_HOME:
                if(w->cursor > 0) w->MoveSelectionCursor(0, (mods & GLFW_MOD_SHIFT) != 0);
                break;
            case GLFW_KEY_END: 
                if(w->cursor < w->GetFocusTextSize()) w->MoveSelectionCursor(w->GetFocusTextSize(), (mods & GLFW_MOD_SHIFT) != 0);
                break;
            case GLFW_KEY_BACKSPACE: 
                if(w->isSelecting) w->RemoveSelection();
                else if(w->focus->text.isEditable && w->cursor > 0)
                {
                    int prev = utf8::prev(w->GetFocusText() + w->cursor) - w->GetFocusText();
                    w->focus->text.text.erase(prev, w->cursor - prev);
                    w->cursor = prev;
                    w->OnEdit();
                }
                break;
            case GLFW_KEY_DELETE:
                if(w->isSelecting) w->RemoveSelection();
                else if(w->focus->text.isEditable && w->cursor < w->GetFocusTextSize())
                {
                    auto next = utf8::next(w->GetFocusText() + w->cursor) - w->GetFocusText();
                    w->focus->text.text.erase(w->cursor, next - w->cursor);
                    w->OnEdit();
                }
                break;
            }
            return;
        }
            
        // Finally, dispatch to custom key handler if one is present
        if(w->focus->onKey) w->focus->onKey(key, action, mods);
    });
}

Window::~Window()
{
    nvgDeleteGL3(vg);
    glfwDestroyWindow(window);
}

static void CollectTabStops(std::vector<gui::ElementPtr> & tabStops, const gui::ElementPtr & elem)
{
    if(elem->text.isEditable) tabStops.push_back(elem);
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

static void DrawElement(NVGcontext * vg, const gui::Element & elem, const gui::Element * focus, size_t cursorIndex, size_t selectLeft, size_t selectRight, NVGcolor parentBackground)
{
    if(elem.onDraw)
    {
        elem.onDraw(elem.rect);
    }
    else
    {
        // Compute the background color for this element
        NVGcolor background = parentBackground;
        int minSize = 0;
        switch(elem.style)
        {    
        case gui::BORDER:
            minSize = 12;
        case gui::BACKGROUND:
            background = nvgRGBA(64,64,64,255);
            break;
        case gui::EDIT:
            background = nvgRGBA(88,88,88,255);
            minSize = 6;
            break;
        }

        // If it differs from the parent element, fill the client area with the new background color
        if(memcmp(&background, &parentBackground, sizeof(background)))
        {
            if((elem.rect.GetWidth() < minSize || elem.rect.GetHeight() < minSize) && parentBackground.a != 0) return;
            nvgBeginPath(vg);
            nvgRect(vg, elem.rect.x0, elem.rect.y0, elem.rect.GetWidth(), elem.rect.GetHeight());
	        nvgFillColor(vg, background);
	        nvgFill(vg);
        }
        if(parentBackground.a == 0) parentBackground = background; // If there was no parent color, reuse this color

        // Begin scissoring to client rect
    	nvgSave(vg);
	    nvgScissor(vg, elem.rect.x0, elem.rect.y0, elem.rect.GetWidth(), elem.rect.GetHeight());

        // If we have an assigned font, handle rendering of text component
        if(elem.text.font)
        {
            const auto & font = *elem.text.font;

            if(&elem == focus && selectLeft < selectRight)
            {
                const int x = elem.rect.x0 + font.GetStringWidth(elem.text.text.substr(0,selectLeft));
                const int w = font.GetStringWidth(elem.text.text.substr(selectLeft,selectRight-selectLeft));

                nvgBeginPath(vg);
                nvgRect(vg, x, elem.rect.y0, w, font.GetLineHeight());
                nvgFillColor(vg, nvgRGBA(0,255,255,128));
                nvgFill(vg);
            }          

            font.DrawString(elem.rect.x0, elem.rect.y0, elem.text.color.r, elem.text.color.g, elem.text.color.b, elem.text.text);

            if(&elem == focus)
            {
                int x = elem.rect.x0 + font.GetStringWidth(elem.text.text.substr(0,cursorIndex));

                nvgBeginPath(vg);
                nvgRect(vg, x, elem.rect.y0, 1, font.GetLineHeight());
                nvgFillColor(vg, nvgRGBA(255,255,255,192));
                nvgFill(vg);
            }

            auto transparentBackground = parentBackground;
            transparentBackground.a = 0;
            auto bg = nvgLinearGradient(vg, elem.rect.x1-6, elem.rect.y0, elem.rect.x1, elem.rect.y0, transparentBackground, parentBackground);
            nvgBeginPath(vg);
            nvgRect(vg, elem.rect.x1-6, elem.rect.y0, 6, font.GetLineHeight());
            nvgFillPaint(vg, bg);
            nvgFill(vg);
        }

        // Draw children in back-to-front order
        for(const auto & child : elem.children)
        {
            DrawElement(vg, *child.element, focus, cursorIndex, selectLeft, selectRight, background);
        }

        // Render border effects after all children have been finished
        switch(elem.style)
        {
        case gui::BORDER:
            // Restore corners of background
            nvgBeginPath(vg);
            nvgRect(vg, elem.rect.x0, elem.rect.y0, elem.rect.GetWidth(), elem.rect.GetHeight());
            nvgRoundedRect(vg, elem.rect.x0+1, elem.rect.y0+1, elem.rect.GetWidth()-2, elem.rect.GetHeight()-2, 5.0f);
            nvgPathWinding(vg, NVG_HOLE);
	        nvgFillColor(vg, parentBackground);
	        nvgFill(vg);

            // Fill boundary
            nvgBeginPath(vg);
            nvgRoundedRect(vg, elem.rect.x0+0.5f, elem.rect.y0+0.5f, elem.rect.GetWidth()-1, elem.rect.GetHeight()-1, 5.5f);
            nvgRoundedRect(vg, elem.rect.x0+2.5f, elem.rect.y0+2.5f, elem.rect.GetWidth()-5, elem.rect.GetHeight()-5, 3.5f);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillColor(vg, nvgRGBA(0,0,0,192));
            nvgFill(vg);
            break;
        case gui::EDIT:
            // Restore corners of background
            nvgBeginPath(vg);
            nvgRect(vg, elem.rect.x0, elem.rect.y0, elem.rect.GetWidth(), elem.rect.GetHeight());
            nvgRoundedRect(vg, elem.rect.x0+1, elem.rect.y0+1, elem.rect.GetWidth()-2, elem.rect.GetHeight()-2, 2);
            nvgPathWinding(vg, NVG_HOLE);
	        nvgFillColor(vg, parentBackground);
	        nvgFill(vg);

            // Draw a drop-shadow inside the box
            auto bg = nvgBoxGradient(vg, elem.rect.x0+1, elem.rect.y0+1, elem.rect.GetWidth()-2, elem.rect.GetHeight()-2, 2, 4, nvgRGBA(67,67,67,0), nvgRGBA(67,67,67,255));
	        nvgBeginPath(vg);
	        nvgRoundedRect(vg, elem.rect.x0+1, elem.rect.y0+1, elem.rect.GetWidth()-2, elem.rect.GetHeight()-2, 2);
	        nvgFillPaint(vg, bg);
	        nvgFill(vg);

            // Stroke an outline for the box
	        nvgBeginPath(vg);
	        nvgRoundedRect(vg, elem.rect.x0+0.5f, elem.rect.y0+0.5f, elem.rect.GetWidth()-1, elem.rect.GetHeight()-1, 2.5f);
	        nvgStrokeColor(vg, nvgRGBA(0,0,0,128));
	        nvgStroke(vg);
            break;
        }

        nvgRestore(vg);
    }
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

    DrawElement(vg, *root, focus.get(), cursor, isSelecting ? GetSelectionLeftIndex() : 0, isSelecting ? GetSelectionRightIndex() : 0, nvgRGBA(0,0,0,0));
    nvgEndFrame(vg);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();

    glfwSwapBuffers(window);
}