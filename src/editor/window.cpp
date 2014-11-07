#include "window.h"
#include "engine/gl.h"

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

Window::Window(const char * title, int width, int height) : window(), /*context(),*/ width(width), height(height), focus(), isSelecting()
{
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);

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

static void DrawElement(const gui::Element & elem, const gui::Element * focus, size_t cursorIndex, size_t selectLeft, size_t selectRight)
{
    if(elem.onDraw)
    {
        elem.onDraw(elem.rect);
    }
    else
    {
        if(elem.back.image)
        {
            const float s[] = {0, (float)elem.back.border/elem.back.image->GetWidth(), 1-(float)elem.back.border/elem.back.image->GetWidth(), 1};
            const float t[] = {0, (float)elem.back.border/elem.back.image->GetHeight(), 1-(float)elem.back.border/elem.back.image->GetHeight(), 1};
            const int x[] = {elem.rect.x0, elem.rect.x0+elem.back.border, elem.rect.x1-elem.back.border, elem.rect.x1};
            const int y[] = {elem.rect.y0, elem.rect.y0+elem.back.border, elem.rect.y1-elem.back.border, elem.rect.y1};
                
            glEnable(GL_TEXTURE_2D);
            elem.back.image->Bind();
            glBegin(GL_QUADS);
            for(int i=1; i<4; ++i)
            {
                for(int j=1; j<4; ++j)
                {
                    glTexCoord2f(s[j-1],t[i-1]); glVertex2f(x[j-1],y[i-1]);
                    glTexCoord2f(s[j-0],t[i-1]); glVertex2f(x[j-0],y[i-1]);
                    glTexCoord2f(s[j-0],t[i-0]); glVertex2f(x[j-0],y[i-0]);
                    glTexCoord2f(s[j-1],t[i-0]); glVertex2f(x[j-1],y[i-0]);
                }
            }
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }

        if(elem.text.font)
        {
            const auto & font = *elem.text.font;

            if(&elem == focus)
            {
                const int x0 = elem.rect.x0 + font.GetStringWidth(elem.text.text.substr(0,selectLeft));
                const int x1 = x0 + font.GetStringWidth(elem.text.text.substr(selectLeft,selectRight-selectLeft));
                const int y0 = elem.rect.y0, y1 = y0 + font.GetLineHeight();
                glDisable(GL_TEXTURE_2D);
                glBegin(GL_QUADS);
                glColor3f(0,1,1);
                glVertex2i(x0,y0);
                glVertex2i(x1,y0);
                glVertex2i(x1,y1);
                glVertex2i(x0,y1);
                glEnd();
            }

            font.DrawString(elem.rect.x0, elem.rect.y0, elem.text.color.r, elem.text.color.g, elem.text.color.b, elem.text.text);

            if(&elem == focus)
            {
                int x0 = elem.rect.x0 + font.GetStringWidth(elem.text.text.substr(0,cursorIndex)), x1 = x0 + 1;
                int y0 = elem.rect.y0, y1 = y0 + font.GetLineHeight();
                glDisable(GL_TEXTURE_2D);
                glBegin(GL_QUADS);
                glColor3f(1,1,1);
                glVertex2i(x0,y0);
                glVertex2i(x1,y0);
                glVertex2i(x1,y1);
                glVertex2i(x0,y1);
                glEnd();
            }
        }
    }

    for(const auto & child : elem.children)
    {
        DrawElement(*child.element, focus, cursorIndex, selectLeft, selectRight);
    }
}

void Window::Redraw()
{
    glfwMakeContextCurrent(window);

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glOrtho(0, width, height, 0, -1, +1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    DrawElement(*root, focus.get(), cursor, isSelecting ? GetSelectionLeftIndex() : 0, isSelecting ? GetSelectionRightIndex() : 0);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();

    glfwSwapBuffers(window);
}