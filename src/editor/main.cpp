#include "window.h"
#include "widgets.h"

#include "engine/gl.h"
#include "engine/font.h"
#include "engine/transform.h"
#include "engine/geometry.h"

#include <vector>
#include <algorithm>
#include <sstream>

struct Vertex
{
    float3 position, normal;
    void Draw() const
    {
        glNormal(normal);
        glVertex(position);
    }
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint3> triangles;

    RayMeshHit Hit(const Ray & ray) const { return IntersectRayMesh(ray, vertices.data(), &Vertex::position, triangles.data(), triangles.size()); }

    void Draw() const
    {
        glBegin(GL_TRIANGLES);
        for(auto & tri : triangles)
        {
            vertices[tri.x].Draw();
            vertices[tri.y].Draw();
            vertices[tri.z].Draw();
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

    RayMeshHit Hit(const Ray & ray) const
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
        Object * best = nullptr;
        float bestT = 0;
        for(auto & obj : objects)
        {
            auto hit = obj.Hit(ray);
            if(hit.hit && (!best || hit.t < bestT))
            {
                best = &obj;
                bestT = hit.t;
            }
        }
        return best;
    }

    void Draw() const
    {
        for(auto & obj : objects) obj.Draw();
    }
};

struct Selection
{
    Object * object;

    Selection() : object() {}

    void SetSelection(Object * object)
    {
        if(object != this->object)
        {
            this->object = object; 
            if(onSelectionChanged) onSelectionChanged();
        }
    }
    
    std::function<void()> onSelectionChanged;
};

struct View
{
    Scene & scene;
    Selection & selection;
    Pose viewpoint;
    float yaw=0,pitch=0;
    bool bf=0,bl=0,bb=0,br=0;

    View(Scene & scene, Selection & selection) : scene(scene), selection(selection) {}

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
        glLoad(PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.25f, 32.0f));
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoad(viewpoint.Inverse().Matrix());

        float pos[] = {0.2f,1,0.3f,0}, ambient[] = {0.5f,0.5f,0.5f,1.0f};
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        glEnable(GL_COLOR_MATERIAL);

        glEnable(GL_DEPTH_TEST);

        scene.Draw();

        if(selection.object)
        {
            glPushMatrix();
            glMult(TranslationMatrix(selection.object->position));
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1, -1);
            glDisable(GL_LIGHTING);
            glColor3f(1,1,1);
            selection.object->mesh->Draw();
            glPopMatrix();
        }
        
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

            auto mat = inv(PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.25f, 32.0f));
            auto p0 = mul(mat, float4(viewX, viewY, -1, 1)), p1 = mul(mat, float4(viewX, viewY, 1, 1));
            Ray ray = viewpoint * Ray::Between(p0.xyz()/p0.w, p1.xyz()/p1.w);
            
            if(auto obj = scene.Hit(ray)) selection.SetSelection(obj);
        };
        viewport->onDrag = [this](int dx, int dy) { OnDrag(dx, dy); };
        viewport->onKey = [this](int key, int action, int mods) { OnKey(key, action, mods); };
        viewport->onDraw = [this](const gui::Rect & rect) { OnDraw(rect); };
        return viewport;
    }
};

Mesh MakeBox(const float3 & halfDims)
{
    Mesh mesh = {
        {{{+1,-1,-1},{+1,0,0}}, {{+1,+1,-1},{+1,0,0}}, {{+1,+1,+1},{+1,0,0}}, {{+1,-1,+1},{+1,0,0}},
         {{-1,+1,-1},{-1,0,0}}, {{-1,-1,-1},{-1,0,0}}, {{-1,-1,+1},{-1,0,0}}, {{-1,+1,+1},{-1,0,0}},
         {{-1,+1,-1},{0,+1,0}}, {{-1,+1,+1},{0,+1,0}}, {{+1,+1,+1},{0,+1,0}}, {{+1,+1,-1},{0,+1,0}},
         {{-1,-1,+1},{0,-1,0}}, {{-1,-1,-1},{0,-1,0}}, {{+1,-1,-1},{0,-1,0}}, {{+1,-1,+1},{0,-1,0}},
         {{-1,-1,+1},{0,0,+1}}, {{+1,-1,+1},{0,0,+1}}, {{+1,+1,+1},{0,0,+1}}, {{-1,+1,+1},{0,0,+1}},
         {{+1,-1,-1},{0,0,-1}}, {{-1,-1,-1},{0,0,-1}}, {{-1,+1,-1},{0,0,-1}}, {{+1,+1,-1},{0,0,-1}}},
        {{{0,1,2}, {0,2,3}, {4,5,6}, {4,6,7}, {8,9,10}, {8,10,11}, {12,13,14}, {12,14,15}, {16,17,18}, {16,18,19}, {20,21,22}, {20,22,23}}}
    };
    for(auto & vert : mesh.vertices) vert.position *= halfDims;
    return mesh;
}

class Editor
{
    Window              window;      
    Font                font;
    GuiFactory          factory;
    ListControl         objectList;
    gui::ElementPtr     propertyPanel;
    gui::ElementPtr     guiRoot;
    Mesh                mesh,ground;

    Scene               scene;
    Selection           selection;
    View                view;
public:
    Editor() : window("Editor", 1280, 720), font(window.GetNanoVG(), "../assets/Roboto-Bold.ttf", 18, true, 0x500), factory(font, 2), view(scene, selection)
    {
        mesh = MakeBox({0.5f,0.5f,0.5f});
        ground = MakeBox({4,0.1f,4});
        scene.objects = {
            {"Ground",{0,-0.1f,0},{0.4f,0.4f,0.4f},&ground},
            {"Alpha",{-0.6f,0.5f,0},{1,0,0},&mesh},
            {"Beta", {+0.6f,0.5f,0},{0,1,0},&mesh},
            {"Gamma",{ 0.0f,1.5f,0},{1,1,0},&mesh}
        };
        view.viewpoint.position = {0,1,4};

        for(auto & obj : scene.objects)
        {
            objectList.AddItem(factory, obj.name);
        }

        propertyPanel = std::make_shared<gui::Element>();
        auto topRightPanel = factory.AddBorder(4, gui::BORDER, objectList.GetPanel());
        auto bottomRightPanel = factory.AddBorder(4, gui::BORDER, propertyPanel);
        auto rightPanel = factory.MakeNSSizer(topRightPanel, bottomRightPanel, 200);
        guiRoot = factory.MakeWESizer(view.CreateViewport(factory), rightPanel, -400);
    
        selection.onSelectionChanged = [this]()
        {
            objectList.SetSelectedIndex(selection.object ? selection.object - scene.objects.data() : -1);

            std::vector<std::pair<std::string, gui::ElementPtr>> props;
            if(selection.object)
            {
                auto & obj = *selection.object;
                props.push_back({"Name", factory.MakeEdit(obj.name, [this](const std::string & text)
                {
                    int selectedIndex = objectList.GetSelectedIndex();
                    scene.objects[selectedIndex].name = text;
                    objectList.GetPanel()->children[selectedIndex].element->text.text = text;                
                })});
                props.push_back({"Position", factory.MakeVectorEdit(obj.position)});
                props.push_back({"Color", factory.MakeVectorEdit(obj.color)});
            }
            propertyPanel->children = {{{{0,0},{0,0},{1,0},{1,0}}, factory.MakePropertyMap(props)}};

            window.RefreshLayout();
        };
        objectList.onSelectionChanged = [this]() { selection.SetSelection(&scene.objects[objectList.GetSelectedIndex()]); };

        selection.onSelectionChanged();

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