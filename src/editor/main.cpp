#include "window.h"
#include "widgets.h"

#include "engine/gl.h"
#include "engine/font.h"
#include "engine/transform.h"

#include <vector>
#include <algorithm>
#include <sstream>

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
    gl::Texture         border, edit, sizer;
    GuiFactory          factory;
    ListControl         objectList;
    gui::ElementPtr     propertyPanel;
    gui::ElementPtr     guiRoot;
    std::vector<Object> objects;

    Pose                view;
    bool                bf,bl,bb,br;
    float               yaw,pitch;
public:
    Editor() : window("Editor", 1280, 720), font("C:/windows/fonts/arialbd.ttf", 16, true, 0x500), factory(font, 2, {&edit,1})
    {
        objects = {
            {"Alpha",{-1,-1,-5},{1,0,0}},
            {"Beta", {+1,-1,-5},{0,1,0}},
            {"Gamma",{ 0,+1,-5},{1,1,0}}
        };
        bf=bl=bb=br=false;
        yaw=pitch=0;

        border.Load("../assets/border.png");
        edit.Load("../assets/edit.png");
        sizer.Load("../assets/sizer.png");

        auto viewport = std::make_shared<gui::Element>();
        viewport->onDrag = [this](int dx, int dy)
        {
            yaw -= dx * 0.01f;
            pitch -= dy * 0.01f;
            view.orientation = qmul(RotationQuaternion({0,1,0}, yaw), RotationQuaternion({1,0,0}, pitch));
        };
        viewport->onKey = [this](int key, int action, int mods)
        {
            switch(key)
            {
            case GLFW_KEY_W: bf = action != GLFW_RELEASE; break;
            case GLFW_KEY_A: bl = action != GLFW_RELEASE; break;
            case GLFW_KEY_S: bb = action != GLFW_RELEASE; break;
            case GLFW_KEY_D: br = action != GLFW_RELEASE; break;
            }
        };
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
            glLoad(PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.1f, 8.0f));
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoad(view.Inverse().Matrix());

            glBegin(GL_QUADS);
            for(auto & obj : objects)
            {
                glColor(obj.color);
                glVertex3f(obj.position.x-0.1f, obj.position.y-0.1f, obj.position.z);
                glVertex3f(obj.position.x+0.1f, obj.position.y-0.1f, obj.position.z);
                glVertex3f(obj.position.x+0.1f, obj.position.y+0.1f, obj.position.z);
                glVertex3f(obj.position.x-0.1f, obj.position.y+0.1f, obj.position.z);
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
        auto rightPanel = factory.MakeNSSizer(&sizer, topRightPanel, bottomRightPanel, 200);
        guiRoot = factory.MakeWESizer(&sizer, viewport, rightPanel, -400);
    
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
        auto t0 = glfwGetTime();
        while(!window.ShouldClose())
        {
            glfwPollEvents();

            const auto t1 = glfwGetTime();
            const auto timestep = static_cast<float>(t1 - t0);
            t0 = t1;

            float3 dir;
            if(bf) dir.z -= 1;
            if(bl) dir.x -= 1;
            if(bb) dir.z += 1;
            if(br) dir.x += 1;
            if(mag2(dir) > 0) view.position = view.TransformCoord(norm(dir) * (timestep * 8));

            window.Redraw();
        }
        return 0;
    }
};

#include <iostream>

int main(int argc, char * argv[])
{
    glfwInit();
    int result = -1;
    try { result = Editor().Run(); }
    catch(const std::exception & e) { std::cerr << "Unhandled exception caught: " << e.what() << std::endl; }
    catch(...) { std::cerr << "Unknown unhandled exception caught." << std::endl; }
    glfwTerminate();
    return result;
}