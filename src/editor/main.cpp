#include "window.h"
#include "widgets.h"

#include "engine/gl.h"
#include "engine/font.h"
#include "engine/transform.h"
#include "engine/geometry.h"

#include "nanovg.h"

#include <vector>
#include <algorithm>
#include <sstream>

struct Vertex
{
    float3 position, normal;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint3> triangles;
    gl::Mesh glMesh;

    Mesh() {}
    Mesh(Mesh && m) : Mesh() { *this = std::move(m); }
    Mesh & operator = (Mesh && m) { vertices=move(m.vertices); triangles=move(m.triangles); glMesh=std::move(m.glMesh); return *this; }

    RayMeshHit Hit(const Ray & ray) const { return IntersectRayMesh(ray, vertices.data(), &Vertex::position, triangles.data(), triangles.size()); }

    void Upload()
    {       
        glMesh.SetVertices(vertices);
        glMesh.SetAttribute(0, &Vertex::position);
        glMesh.SetAttribute(1, &Vertex::normal);
        glMesh.SetElements(triangles);
    }

    void Draw()
    {
        glMesh.Draw();
    }
};

struct Object
{
    std::string name;
    float3 position;
    float3 color;
    Mesh * mesh;
    GLuint prog;

    RayMeshHit Hit(const Ray & ray) const
    {
        return mesh->Hit({ray.start-position,ray.direction});
    }

    void Draw(const float4x4 & viewProj, const float3 & eye, const float3 & lightPos)
    {
        auto model = TranslationMatrix(position);
        auto mvp = mul(viewProj, model);
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_model"), 1, GL_FALSE, &model.x.x);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
        glUniform3fv(glGetUniformLocation(prog, "u_eye"), 1, &eye.x);
        glUniform3fv(glGetUniformLocation(prog, "u_color"), 1, &color.x);
        glUniform3fv(glGetUniformLocation(prog, "u_lightPos"), 1, &lightPos.x);
        mesh->Draw();
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

    void Draw(const float4x4 & viewProj, const float3 & eye)
    {
        auto lightPos = objects.back().position;
        for(auto & obj : objects) obj.Draw(viewProj, eye, lightPos);
    }
};

struct Selection
{
    Object * object;
    GLuint selectionProgram;

    Mesh arrowMesh;
    GLuint arrowProg;

    Selection() : object(), selectionProgram() {}

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

class Raycaster
{
    gui::Rect rect;
    float4x4 invProj;
    Pose viewpoint;
public:
    Raycaster(const gui::Rect & rect, const float4x4 & proj, const Pose & viewpoint) : rect(rect), invProj(inv(proj)), viewpoint(viewpoint) {}

    Ray ComputeRay(const int2 & pixel) const
    {
        auto viewX = (pixel.x - rect.x0) * 2.0f / rect.GetWidth() - 1;
        auto viewY = 1 - (pixel.y - rect.y0) * 2.0f / rect.GetHeight();
        return viewpoint * Ray::Between(TransformCoordinate(invProj, {viewX, viewY, -1}), TransformCoordinate(invProj, {viewX, viewY, 1}));
    }
};

class LinearTranslationDragger : public gui::IDragger
{
    Object & object;
    Raycaster caster;
    float3 direction, initialPosition;
    float initialS;

    float ComputeS(const int2 & mouse) const
    {
        const Ray ray1 = {initialPosition, direction}, ray2 = caster.ComputeRay(mouse);
        const auto r12 = ray2.start - ray1.start;
        const auto e1e2 = dot(ray1.direction, ray2.direction), denom = 1 - e1e2*e1e2;
        return (dot(r12,ray1.direction) - dot(r12,ray2.direction)*e1e2) / denom;
    }
public:
    LinearTranslationDragger(Object & object, const Raycaster & caster, const float3 & direction, const int2 & click) : object(object), caster(caster), direction(direction), initialPosition(object.position), initialS(ComputeS(click)) {}

    void OnDrag(int2 newMouse) override { object.position = initialPosition + direction * (ComputeS(newMouse) - initialS); }
    void OnRelease() override {}
    void OnCancel() override { object.position = initialPosition; }
};

struct View : public gui::Element
{
    class MouselookDragger : public gui::IDragger
    {
        View & view;
        int2 lastMouse;
    public:
        MouselookDragger(View & view, const int2 & click) : view(view), lastMouse(click) {}

        void OnDrag(int2 newMouse) override { view.OnDrag(newMouse - lastMouse); lastMouse = newMouse; }
        bool OnKey(int key, int action, int mods) override
        {
            switch(key)
            {
            case GLFW_KEY_W: view.bf = action != GLFW_RELEASE; break;
            case GLFW_KEY_A: view.bl = action != GLFW_RELEASE; break;
            case GLFW_KEY_S: view.bb = action != GLFW_RELEASE; break;
            case GLFW_KEY_D: view.br = action != GLFW_RELEASE; break;
            }
            return true;
        }
        void OnRelease() override { view.bf = view.bl = view.bb = view.br = false; }
        void OnCancel() override {}
    };

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

    void OnDrag(const int2 & delta)
    {
        yaw -= delta.x * 0.01f;
        pitch -= delta.y * 0.01f;
        viewpoint.orientation = qmul(RotationQuaternion({0,1,0}, yaw), RotationQuaternion({1,0,0}, pitch));
    }
    
    void OnKey(GLFWwindow * window, int key, int action, int mods) override
    {
        
    }

    NVGcolor OnDrawBackground(const gui::DrawEvent & e) const override
    {
        int winWidth, winHeight;
        glfwGetWindowSize(glfwGetCurrentContext(), &winWidth, &winHeight);

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glViewport(rect.x0, winHeight - rect.y1, rect.GetWidth(), rect.GetHeight());
        glScissor(rect.x0, winHeight - rect.y1, rect.GetWidth(), rect.GetHeight());
        glEnable(GL_SCISSOR_TEST);
        glClearColor(0,0,1,1);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        auto viewProj = mul(PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.25f, 32.0f), viewpoint.Inverse().Matrix());
        scene.Draw(viewProj, viewpoint.position);

        if(selection.object)
        {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1, -1);
            auto prog = selection.object->prog;
            selection.object->prog = selection.selectionProgram;
            selection.object->Draw(viewProj, viewpoint.position, {});
            selection.object->prog = prog;
            glPopAttrib();

            if(e.hasFocus)
            {
                glClear(GL_DEPTH_BUFFER_BIT);
                auto model = TranslationMatrix(selection.object->position);
                auto mvp = mul(viewProj, model);
                glUseProgram(selection.arrowProg);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_model"), 1, GL_FALSE, &model.x.x);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
                glUniform3fv(glGetUniformLocation(selection.arrowProg, "u_eye"), 1, &viewpoint.position.x);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_color"), 0.2f, 0.2f, 1.0f);
                glUniform3fv(glGetUniformLocation(selection.arrowProg, "u_lightPos"), 1, &scene.objects.back().position.x);
                selection.arrowMesh.Draw();

                model = Pose(selection.object->position, RotationQuaternion({1,0,0},-1.57f)).Matrix();
                mvp = mul(viewProj, model);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_model"), 1, GL_FALSE, &model.x.x);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_color"), 0.2f, 1.0f, 0.2f);
                selection.arrowMesh.Draw();

                model = Pose(selection.object->position, RotationQuaternion({0,1,0},+1.57f)).Matrix();
                mvp = mul(viewProj, model);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_model"), 1, GL_FALSE, &model.x.x);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_color"), 1.0f, 0.2f, 0.2f);
                selection.arrowMesh.Draw();
            }
        }

        glPopAttrib();

        if(e.hasFocus)
        {
	        nvgBeginPath(e.vg);
	        nvgRect(e.vg, rect.x0+1.5f, rect.y0+1.5f, rect.GetWidth()-3, rect.GetHeight()-3);
	        nvgStrokeColor(e.vg, nvgRGBA(255,255,255,128));
            nvgStrokeWidth(e.vg, 1);
	        nvgStroke(e.vg);
        }

        return {};
    }

    gui::DraggerPtr OnClick(const gui::MouseEvent & e) override
    {
        // Mouselook when the user drags the right mouse button
        if(e.button == GLFW_MOUSE_BUTTON_RIGHT) return std::make_shared<MouselookDragger>(*this, e.cursor);

        // Respond to left mouse button clicks with a raycast
        if(e.button == GLFW_MOUSE_BUTTON_LEFT)
        {
            Raycaster caster(rect, PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.25f, 32.0f), viewpoint);
            Ray ray = caster.ComputeRay(e.cursor);
            
            // If an object is selected, check if we have clicked on its gizmo
            if(selection.object)
            {
                auto localRay = Pose(selection.object->position, {0,0,0,1}).Inverse() * ray;
                auto hit = selection.arrowMesh.Hit(localRay);
                if(hit.hit) return std::make_shared<LinearTranslationDragger>(*selection.object, caster, float3(0,0,1), e.cursor);

                localRay = Pose(selection.object->position, RotationQuaternion({1,0,0},-1.57f)).Inverse() * ray;
                hit = selection.arrowMesh.Hit(localRay);
                if(hit.hit) return std::make_shared<LinearTranslationDragger>(*selection.object, caster, float3(0,1,0), e.cursor);

                localRay = Pose(selection.object->position, RotationQuaternion({0,1,0},+1.57f)).Inverse() * ray;
                hit = selection.arrowMesh.Hit(localRay);
                if(hit.hit) return std::make_shared<LinearTranslationDragger>(*selection.object, caster, float3(1,0,0), e.cursor);
            }

            // Otherwise see if we have selected a new object
            if(auto obj = scene.Hit(ray)) selection.SetSelection(obj);
            // If we did not click on an object directly, perhaps we should box select? 
        }

        return nullptr;
    }
};

Mesh MakeBox(const float3 & halfDims)
{
    Mesh mesh;
    mesh.vertices = {
        {{+1,-1,-1},{+1,0,0}}, {{+1,+1,-1},{+1,0,0}}, {{+1,+1,+1},{+1,0,0}}, {{+1,-1,+1},{+1,0,0}},
        {{-1,+1,-1},{-1,0,0}}, {{-1,-1,-1},{-1,0,0}}, {{-1,-1,+1},{-1,0,0}}, {{-1,+1,+1},{-1,0,0}},
        {{-1,+1,-1},{0,+1,0}}, {{-1,+1,+1},{0,+1,0}}, {{+1,+1,+1},{0,+1,0}}, {{+1,+1,-1},{0,+1,0}},
        {{-1,-1,+1},{0,-1,0}}, {{-1,-1,-1},{0,-1,0}}, {{+1,-1,-1},{0,-1,0}}, {{+1,-1,+1},{0,-1,0}},
        {{-1,-1,+1},{0,0,+1}}, {{+1,-1,+1},{0,0,+1}}, {{+1,+1,+1},{0,0,+1}}, {{-1,+1,+1},{0,0,+1}},
        {{+1,-1,-1},{0,0,-1}}, {{-1,-1,-1},{0,0,-1}}, {{-1,+1,-1},{0,0,-1}}, {{+1,+1,-1},{0,0,-1}}
    };
    mesh.triangles = {{0,1,2}, {0,2,3}, {4,5,6}, {4,6,7}, {8,9,10}, {8,10,11}, {12,13,14}, {12,14,15}, {16,17,18}, {16,18,19}, {20,21,22}, {20,22,23}};
    for(auto & vert : mesh.vertices) vert.position *= halfDims;
    mesh.Upload();
    return mesh;
}

class Editor
{
    Window                  window;      
    Font                    font;
    GuiFactory              factory;
    std::shared_ptr<ListBox>objectList;
    gui::ElementPtr         propertyPanel;
    gui::ElementPtr         guiRoot;
    Mesh                    mesh,ground,bulb;

    Scene                   scene;
    Selection               selection;
    std::shared_ptr<View>   view;

    bool                    quit;
public:
    Editor() : window("Editor", 1280, 720), font(window.GetNanoVG(), "../assets/Roboto-Bold.ttf", 18, true, 0x500), factory(font, 2), quit()
    {
        view = std::make_shared<View>(scene, selection);

        auto vs = gl::CompileShader(GL_VERTEX_SHADER, R"(#version 330
uniform mat4 u_model;
uniform mat4 u_modelViewProj;
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
out vec3 position;
out vec3 normal;
void main()
{
    gl_Position = u_modelViewProj * vec4(v_position,1);
    position = (u_model * vec4(v_position,1)).xyz;
    normal = (u_model * vec4(v_normal,0)).xyz;
}
)");

        auto fs = gl::CompileShader(GL_FRAGMENT_SHADER, R"(#version 330
uniform vec3 u_lightPos;
uniform vec3 u_eye;
uniform vec3 u_color;
in vec3 position;
in vec3 normal;
void main()
{
    vec3 lightDir = normalize(u_lightPos - position);
    vec3 eyeDir = normalize(u_eye - position);
    vec3 halfDir = normalize(lightDir + eyeDir);

    float light = 0.3;
    light += max(dot(normal, lightDir), 0);
    light += pow(max(dot(normal, halfDir), 0), 128);
    gl_FragColor = vec4(u_color*light,1);
}
)");

        auto fs2 = gl::CompileShader(GL_FRAGMENT_SHADER, "#version 330\nvoid main() { gl_FragColor = vec4(1,1,1,1); }");

        auto prog = gl::LinkProgram(vs, fs);
        selection.selectionProgram = gl::LinkProgram(vs, fs2);

        for(uint32_t i=0; i<12; ++i)
        {
            const float a = i*6.28f/12, c = std::cos(a), s = std::sin(a);
            selection.arrowMesh.vertices.push_back({{c*0.05f,s*0.05f,0},{c,s,0}});
            selection.arrowMesh.vertices.push_back({{c*0.05f,s*0.05f,1},{c,s,0}});
            selection.arrowMesh.triangles.push_back({{i*2,i*2+1,(i*2+3)%24}});
            selection.arrowMesh.triangles.push_back({{i*2,(i*2+3)%24,(i*2+2)%24}});
        }
        selection.arrowMesh.Upload();
        selection.arrowProg = prog;

        mesh = MakeBox({0.5f,0.5f,0.5f});
        ground = MakeBox({4,0.1f,4});
        bulb = MakeBox({0.1f,0.1f,0.1f});
        scene.objects = {
            {"Ground",{0,-0.1f,0},{0.4f,0.4f,0.4f},&ground,prog},
            {"Alpha",{-0.6f,0.5f,0},{1,0,0},&mesh,prog},
            {"Beta", {+0.6f,0.5f,0},{0,1,0},&mesh,prog},
            {"Gamma",{ 0.0f,1.5f,0},{1,1,0},&mesh,prog},
            {"Light",{ 0.0f,3.0f,1.0},{1,1,1},&bulb,prog},
        };
        view->viewpoint.position = {0,1,4};

        objectList = std::make_shared<ListBox>(font, 2);
        for(auto & obj : scene.objects)
        {
            objectList->AddItem(obj.name);
        }

        propertyPanel = std::make_shared<gui::Element>();
        auto topRightPanel = Border::CreateBigBorder(objectList);
        auto bottomRightPanel = Border::CreateBigBorder(propertyPanel);
        auto rightPanel = std::make_shared<Splitter>(bottomRightPanel, topRightPanel, Splitter::Top, 200);
        auto mainPanel = std::make_shared<Splitter>(view, rightPanel, Splitter::Right, 400);
        
        auto menuBar = std::make_shared<Menu>(font);
        auto fileMenu = menuBar->AddPopup("File");
        fileMenu->AddItem("New",  [](){});
        fileMenu->AddItem("Open", [](){});
        fileMenu->AddItem("Save", [](){});
        fileMenu->AddItem("Exit", [this]() { quit = true; });
        auto editMenu = menuBar->AddPopup("Edit");
        editMenu->AddItem("Cut",   [](){});
        editMenu->AddItem("Copy",  [](){});
        editMenu->AddItem("Paste", [](){});

        auto guiRoot = std::make_shared<gui::Element>();
        guiRoot->AddChild({{0,0}, {0,32}, {1,0}, {1,0}}, mainPanel);
        guiRoot->AddChild({{0,0}, {0,0}, {1,0}, {1,0}}, menuBar->GetModalBarrier());
        guiRoot->AddChild({{0,0}, {0,0}, {1,0}, {0,32}}, menuBar);
        guiRoot->AddChild({{0,0}, {0,32}, {0,100}, {0,112}}, fileMenu);
        guiRoot->AddChild({{0,100}, {0,32}, {0,200}, {0,112}}, editMenu);
    
        selection.onSelectionChanged = [this]()
        {
            objectList->SetSelectedIndex(selection.object ? selection.object - scene.objects.data() : -1);

            std::vector<std::pair<std::string, gui::ElementPtr>> props;
            if(selection.object)
            {
                auto & obj = *selection.object;
                props.push_back({"Name", factory.MakeEdit(obj.name, [this](const std::string & text)
                {
                    int selectedIndex = objectList->GetSelectedIndex();
                    scene.objects[selectedIndex].name = text;
                    objectList->SetItemText(selectedIndex, text);
                })});
                props.push_back({"Position", factory.MakeVectorEdit(obj.position)});
                props.push_back({"Color", factory.MakeVectorEdit(obj.color)});
            }
            propertyPanel->children = {{{{0,0},{0,0},{1,0},{1,0}}, factory.MakePropertyMap(props)}};

            window.RefreshLayout();
        };
        objectList->onSelectionChanged = [this]() { selection.SetSelection(&scene.objects[objectList->GetSelectedIndex()]); };

        selection.onSelectionChanged();

        window.SetGuiRoot(guiRoot);
    }

    int Run()
    {
        auto t0 = glfwGetTime();
        while(!quit && !window.ShouldClose())
        {
            glfwPollEvents();

            const auto t1 = glfwGetTime();
            const auto timestep = static_cast<float>(t1 - t0);
            t0 = t1;

            view->OnUpdate(timestep);
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