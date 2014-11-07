#include "window.h"
#include "widgets.h"

#include "engine/gl.h"
#include "engine/font.h"
#include "engine/transform.h"
#include "engine/geometry.h"

#include <vector>
#include <algorithm>
#include <sstream>

struct Mesh
{
    std::vector<float3> vertices;
    std::vector<uint3> triangles;

    bool Hit(const Ray & ray) const
    {
        for(auto & tri : triangles) if(IntersectRayTriangle(ray, vertices[tri.x], vertices[tri.y], vertices[tri.z]).hit) return true;
        return false;
    }

    void Draw() const
    {
        glBegin(GL_TRIANGLES);
        for(auto & tri : triangles)
        {
            glVertex(vertices[tri.x]);
            glVertex(vertices[tri.y]);
            glVertex(vertices[tri.z]);
        }
        glEnd();
    }
};

struct Object
{
    std::string name;
    float3 position;
    float3 color;
    const Mesh * mesh;

    bool Hit(const Ray & ray) const
    {
        return mesh->Hit({ray.start-position,ray.direction});
    }

    void Draw() const
    {
        glPushMatrix();
        glMult(TranslationMatrix(position));
        glColor(color);
        mesh->Draw();
        glPopMatrix();
    }
};

struct Scene
{
    std::vector<Object> objects;

    Object * Hit(const Ray & ray)
    {
        for(auto & obj : objects) if(obj.Hit(ray)) return &obj;
        return nullptr;
    }

    void Draw() const
    {
        for(auto & obj : objects) obj.Draw();
    }
};

struct View
{
    Scene & scene;
    Pose viewpoint;
    float yaw=0,pitch=0;
    bool bf=0,bl=0,bb=0,br=0;

    View(Scene & scene) : scene(scene) {}

    void OnUpdate(float timestep)
    {
        float3 dir;
        if(bf) dir.z -= 1;
        if(bl) dir.x -= 1;
        if(bb) dir.z += 1;
        if(br) dir.x += 1;
        if(mag2(dir) > 0) viewpoint.position = viewpoint.TransformCoord(norm(dir) * (timestep * 8));
    }

    void OnDrag(int dx, int dy)
    {
        yaw -= dx * 0.01f;
        pitch -= dy * 0.01f;
        viewpoint.orientation = qmul(RotationQuaternion({0,1,0}, yaw), RotationQuaternion({1,0,0}, pitch));
    }
    
    void OnKey(int key, int action, int mods)
    {
        switch(key)
        {
        case GLFW_KEY_W: bf = action != GLFW_RELEASE; break;
        case GLFW_KEY_A: bl = action != GLFW_RELEASE; break;
        case GLFW_KEY_S: bb = action != GLFW_RELEASE; break;
        case GLFW_KEY_D: br = action != GLFW_RELEASE; break;
        }
    }

    void OnDraw(const gui::Rect & rect) const
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
        glLoad(viewpoint.Inverse().Matrix());

        scene.Draw();
        
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();
    }

    gui::ElementPtr CreateViewport(const GuiFactory & factory)
    {
        auto viewport = std::make_shared<gui::Element>();
        viewport->onClick = [this,viewport](int x, int y) // LOL fix this
        {
            auto & vp = *viewport;
            auto rect = vp.rect;
            auto viewX = (x - rect.x0) * 2.0f / rect.GetWidth() - 1;
            auto viewY = 1 - (y - rect.y0) * 2.0f / rect.GetHeight();

            auto mat = inv(PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.1f, 8.0f));
            auto p0 = mul(mat, float4(viewX, viewY, -1, 1)), p1 = mul(mat, float4(viewX, viewY, 1, 1));
            Ray ray = viewpoint * Ray::Between(p0.xyz()/p0.w, p1.xyz()/p1.w);
            
            if(auto obj = scene.Hit(ray))
            {
                obj->color = float3(1,1,1) - obj->color;
            }
        };
        viewport->onDrag = [this](int dx, int dy) { OnDrag(dx, dy); };
        viewport->onKey = [this](int key, int action, int mods) { OnKey(key, action, mods); };
        viewport->onDraw = [this](const gui::Rect & rect) { OnDraw(rect); };
        return viewport;
    }
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
    Mesh                mesh;

    Scene               scene;
    View                view;
public:
    Editor() : window("Editor", 1280, 720), font("C:/windows/fonts/arialbd.ttf", 16, true, 0x500), factory(font, 2, {&edit,1}), view(scene)
    {
        mesh = {
            {{-0.1f,-0.1f,0}, {+0.1f,-0.1f,0}, {+0.1f,+0.1f,0}, {-0.1f,+0.1f,0}},
            {{0,1,2}, {0,2,3}}
        };
        scene.objects = {
            {"Alpha",{-1,-1,-5},{1,0,0},&mesh},
            {"Beta", {+1,-1,-5},{0,1,0},&mesh},
            {"Gamma",{ 0,+1,-5},{1,1,0},&mesh}
        };

        border.Load("../assets/border.png");
        edit.Load("../assets/edit.png");
        sizer.Load("../assets/sizer.png");

        for(auto & obj : scene.objects)
        {
            objectList.AddItem(factory, obj.name);
        }

        propertyPanel = std::make_shared<gui::Element>();
        auto topRightPanel = factory.AddBorder(4, &border, objectList.GetPanel());
        auto bottomRightPanel = factory.AddBorder(4, &border, propertyPanel);
        auto rightPanel = factory.MakeNSSizer(&sizer, topRightPanel, bottomRightPanel, 200);
        guiRoot = factory.MakeWESizer(&sizer, view.CreateViewport(factory), rightPanel, -400);
    
        objectList.onSelectionChanged = [this]()
        {
            int selectedIndex = objectList.GetSelectedIndex();
            auto & obj = scene.objects[selectedIndex];

            std::vector<std::pair<std::string, gui::ElementPtr>> props;
            props.push_back({"Name", factory.MakeEdit(obj.name, [this](const std::string & text)
            {
                int selectedIndex = objectList.GetSelectedIndex();
                scene.objects[selectedIndex].name = text;
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

            view.OnUpdate(timestep);
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