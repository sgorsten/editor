#include "font.h"
#include "gui.h"
#include "window.h"
#include "gl.h"

#include <vector>
#include <algorithm>
#include <sstream>

struct float3 { float x,y,z; };

class GuiFactory
{
    const Font & font;
    int spacing;
    gui::Element::BackgroundComponent editBorder;
public:
    GuiFactory(const Font & font, int spacing, gui::Element::BackgroundComponent editBorder) : font(font), spacing(spacing), editBorder(editBorder) {}

    gui::ElementPtr AddBorder(int pixels, const gl::Texture * texture, gui::ElementPtr inner) const
    {
        auto panel = std::make_shared<gui::Element>();
        panel->back = {texture,pixels};
        panel->children.push_back({{{0,pixels},{0,pixels},{1,-pixels},{1,-pixels}},inner});
        return panel;
    }

    gui::ElementPtr MakeLabel(const std::string & text) const 
    {
        auto elem = std::make_shared<gui::Element>();
        elem->text = {{1,1,1,1},&font,text,false};
        return elem;
    }
    gui::ElementPtr MakePropertyMap(std::vector<std::pair<std::string, gui::ElementPtr>> properties) const
    {
        int y0 = 0;
        auto panel = std::make_shared<gui::Element>();
        for(auto & pair : properties)
        {
            int y1 = y0 + editBorder.border;
            int y2 = y1 + font.GetLineHeight();
            int y3 = y2 + editBorder.border;
            panel->children.push_back({{{0,0},{0,y1},{0.5,-spacing*0.5f},{0,y2}}, MakeLabel(pair.first)});
            panel->children.push_back({{{0.5,spacing*0.5f},{0,y0},{1,0},{0,y3}}, pair.second});
            y0 = y3 + spacing;
        }
        return panel;
    }

    gui::ElementPtr MakeEdit(const std::string & text, std::function<void(const std::string & text)> onEdit={}) const 
    { 
        auto elem = std::make_shared<gui::Element>();
        elem->cursor = gui::Cursor::IBeam;
        elem->text = {{1,1,1,1}, &font, text, true};
        elem->onEdit = onEdit;
        return AddBorder(editBorder.border, editBorder.image, elem);
    }
    gui::ElementPtr MakeStringEdit(std::string & value) const
    {
        std::ostringstream ss;
        ss << value; 
        return MakeEdit(ss.str(), [&value](const std::string & text) { value = text; });
    }
    gui::ElementPtr MakeFloatEdit(float & value) const
    {
        std::ostringstream ss;
        ss << value; 
        return MakeEdit(ss.str(), [&value](const std::string & text) { std::istringstream(text) >> value; });
    }
    gui::ElementPtr MakeVectorEdit(float3 & value) const
    {
        auto panel = std::make_shared<gui::Element>();
        panel->children.push_back({{{0.0f/3, 0},{0,0},{1.0f/3,-spacing*2.0f/3},{1,0}},MakeFloatEdit(value.x)});
        panel->children.push_back({{{1.0f/3,+spacing*1.0f/3},{0,0},{2.0f/3,-spacing*1.0f/3},{1,0}},MakeFloatEdit(value.y)});
        panel->children.push_back({{{2.0f/3,+spacing*2.0f/3},{0,0},{3.0f/3, 0},{1,0}},MakeFloatEdit(value.z)});
        return panel;
    }

    gui::ElementPtr MakeNSSizer(const gl::Texture * image, gui::ElementPtr top, gui::ElementPtr bottom, int split)
    {
        auto panel = std::make_shared<gui::Element>();
        auto weak = std::weak_ptr<gui::Element>(panel);

        auto sizer = std::make_shared<gui::Element>();
        sizer->cursor = gui::Cursor::SizeNS;
        sizer->back = {image,0};
        sizer->onDrag = [weak](int dx, int dy)
        {
            if(auto panel = weak.lock())
            {
                panel->children[0].placement.y0.b += dy;
                panel->children[0].placement.y1.b += dy;
                panel->children[1].placement.y1 = panel->children[0].placement.y0;
                panel->children[2].placement.y0 = panel->children[0].placement.y1;
                panel->SetRect(panel->rect);
            }
        };

        int height = image->GetHeight();
        if(split > 0) panel->children.push_back({{{0,0},{0,split},{1,0},{0,split+height}}, sizer});
        if(split < 0) panel->children.push_back({{{0,0},{1,split-height},{1,0},{1,split}}, sizer});
        if(split == 0) panel->children.push_back({{{0,0},{0.5,-height*0.5f},{1,0},{0.5,height*0.5f}}, sizer});
        panel->children.push_back({{{0,0},{0,0},{1,0},panel->children[0].placement.y0},top});
        panel->children.push_back({{{0,0},panel->children[0].placement.y1,{1,0},{1,0}},bottom});
        return panel;
    }

    gui::ElementPtr MakeWESizer(const gl::Texture * image, gui::ElementPtr left, gui::ElementPtr right, int split)
    {
        auto panel = std::make_shared<gui::Element>();
        auto weak = std::weak_ptr<gui::Element>(panel);

        auto sizer = std::make_shared<gui::Element>();
        sizer->cursor = gui::Cursor::SizeWE;
        sizer->back = {image,0};
        sizer->onDrag = [weak](int dx, int dy)
        {
            if(auto panel = weak.lock())
            {
                panel->children[0].placement.x0.b += dx;
                panel->children[0].placement.x1.b += dx;
                panel->children[1].placement.x1 = panel->children[0].placement.x0;
                panel->children[2].placement.x0 = panel->children[0].placement.x1;
                panel->SetRect(panel->rect);
            }
        };

        int width = image->GetWidth();
        if(split > 0) panel->children.push_back({{{0,split},{0,0},{0,split+width},{1,0}}, sizer});
        if(split < 0) panel->children.push_back({{{1,split-width},{0,0},{1,split},{1,0}}, sizer});
        if(split == 0) panel->children.push_back({{{0.5,-width*0.5f},{0,0},{0.5,width*0.5f},{1,0}}, sizer});
        panel->children.push_back({{{0,0},{0,0},panel->children[0].placement.x0,{1,0}},left});
        panel->children.push_back({{panel->children[0].placement.x1,{0,0},{1,0},{1,0}},right});
        return panel;
    }
};

class ListControl
{
    gui::ElementPtr panel;
    int selectedIndex;
public:
    ListControl() : panel(std::make_shared<gui::Element>()), selectedIndex(-1) {}

    gui::ElementPtr GetPanel() const { return panel; }
    int GetSelectedIndex() const { return selectedIndex; }

    void SetSelectedIndex(int index)
    {
        for(size_t i=0; i<panel->children.size(); ++i)
        {
            if(i != index) panel->children[i].element->text.color = {0.7f,0.7f,0.7f,1};
            else panel->children[i].element->text.color = {1,1,1,1};
        }
        if(index != selectedIndex)
        {
            selectedIndex = index;
            if(onSelectionChanged) onSelectionChanged();
        }
    }    

    void AddItem(const GuiFactory & factory, const std::string & text)
    {
        auto label = factory.MakeLabel(text);

        float y0 = panel->children.empty() ? 0 : panel->children.back().placement.y1.b+2;
        float y1 = y0 + label->text.font->GetLineHeight();
        int index = panel->children.size();
        panel->children.push_back({{{0,0},{0,y0},{1,0},{0,y1}},label});
        label->onClick = [this,index]() { SetSelectedIndex(index); };
    }

    std::function<void()> onSelectionChanged;
};

struct Object
{
    std::string name;
    float3 position;
    float3 color;
};

class Editor
{
    Window              window;      
    Font                font;
    gl::Texture         border, edit, nssizer, wesizer;
    GuiFactory          factory;
    ListControl         objectList;
    gui::ElementPtr     propertyPanel;
    gui::ElementPtr     guiRoot;
    std::vector<Object> objects;
public:
    Editor() : window("Editor", 1280, 720), font("C:/windows/fonts/arialbd.ttf", 16, true, 0x500), factory(font, 2, {&edit,1})
    {
        objects = {
            {"Alpha",{-0.5f,-0.5f,0},{1,0,0}},
            {"Beta",{+0.5f,-0.5f,0},{0,1,0}},
            {"Gamma",{0.0f,+0.5f,0},{1,1,0}}
        };

        border.Load("../assets/border.png");
        edit.Load("../assets/edit.png");
        nssizer.Load("../assets/nssizer.png");
        wesizer.Load("../assets/wesizer.png");

        auto viewport = std::make_shared<gui::Element>();
        viewport->onDraw = [this](const gui::Rect & rect)
        {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glViewport(rect.x0, rect.y0, rect.x1-rect.x0, rect.y1-rect.y0);
            glScissor(rect.x0, rect.y0, rect.x1-rect.x0, rect.y1-rect.y0);
            glEnable(GL_SCISSOR_TEST);
            glClearColor(0,0,1,1);
            glClear(GL_COLOR_BUFFER_BIT);
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            glBegin(GL_QUADS);
            for(auto & obj : objects)
            {
                glColor3fv(&obj.color.x);
                glTexCoord2f(0,0); glVertex2f(obj.position.x-0.1f, obj.position.y-0.1f);
                glTexCoord2f(1,0); glVertex2f(obj.position.x+0.1f, obj.position.y-0.1f);
                glTexCoord2f(1,1); glVertex2f(obj.position.x+0.1f, obj.position.y+0.1f);
                glTexCoord2f(0,1); glVertex2f(obj.position.x-0.1f, obj.position.y+0.1f);
            }
            glEnd();
        
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glPopAttrib();
        };

        for(auto & obj : objects)
        {
            objectList.AddItem(factory, obj.name);
        }

        propertyPanel = std::make_shared<gui::Element>();
        auto topRightPanel = factory.AddBorder(4, &border, objectList.GetPanel());
        auto bottomRightPanel = factory.AddBorder(4, &border, propertyPanel);
        auto rightPanel = factory.MakeNSSizer(&nssizer, topRightPanel, bottomRightPanel, 200);
        guiRoot = factory.MakeWESizer(&wesizer, viewport, rightPanel, -400);
    
        objectList.onSelectionChanged = [this]()
        {
            int selectedIndex = objectList.GetSelectedIndex();
            auto & obj = objects[selectedIndex];

            std::vector<std::pair<std::string, gui::ElementPtr>> props;
            props.push_back({"Name", factory.MakeEdit(obj.name, [this](const std::string & text)
            {
                int selectedIndex = objectList.GetSelectedIndex();
                objects[selectedIndex].name = text;
                objectList.GetPanel()->children[selectedIndex].element->text.text = text;                
            })});
            props.push_back({"Position", factory.MakeVectorEdit(obj.position)});
            props.push_back({"Color", factory.MakeVectorEdit(obj.color)});

            propertyPanel->children = {{{{0,0},{0,0},{1,0},{1,0}}, factory.MakePropertyMap(props)}};
            window.RefreshLayout();
        };
        objectList.SetSelectedIndex(0);

        window.SetGuiRoot(guiRoot);
    }

    int Run()
    {
        while(!window.ShouldClose())
        {
            glfwPollEvents();
            window.Redraw();
        }
        return 0;
    }
};

#include <iostream>

int main(int argc, char * argv[]) try
{
    glfwInit();
    auto result = Editor().Run();
    glfwTerminate();
    return result;
}
catch(const std::exception & e)
{
    std::cerr << "Unhandled exception caught: " << e.what() << std::endl;
    glfwTerminate();
    return -1;
}