#include "window.h"
#include "widgets.h"

#include "engine/gl.h"
#include "engine/font.h"
#include "engine/transform.h"
#include "engine/geometry.h"

#include "nanovg.h"
#include "scene.h"

#include <algorithm>
#include <sstream>

struct Selection
{
    std::weak_ptr<Object> object;
    GLuint selectionProgram;

    Mesh arrowMesh;
    GLuint arrowProg;

    Selection() : selectionProgram() {}

    void SetSelection(std::shared_ptr<Object> object)
    {
        if(object != this->object.lock())
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

        if(auto obj = selection.object.lock())
        {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1, -1);
            auto prog = obj->prog;
            obj->prog = selection.selectionProgram;
            obj->Draw(viewProj, viewpoint.position, {});
            obj->prog = prog;
            glPopAttrib();

            if(e.hasFocus)
            {
                glClear(GL_DEPTH_BUFFER_BIT);
                auto model = TranslationMatrix(obj->position);
                auto mvp = mul(viewProj, model);
                glUseProgram(selection.arrowProg);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_model"), 1, GL_FALSE, &model.x.x);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
                glUniform3fv(glGetUniformLocation(selection.arrowProg, "u_eye"), 1, &viewpoint.position.x);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_diffuse"), 0.1f, 0.1f, 0.5f);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_emissive"), 0.1f, 0.1f, 0.5f);
                selection.arrowMesh.Draw();

                model = Pose(obj->position, RotationQuaternion({1,0,0},-1.57f)).Matrix();
                mvp = mul(viewProj, model);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_model"), 1, GL_FALSE, &model.x.x);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_diffuse"), 0.1f, 0.5f, 0.1f);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_emissive"), 0.1f, 0.5f, 0.1f);
                selection.arrowMesh.Draw();

                model = Pose(obj->position, RotationQuaternion({0,1,0},+1.57f)).Matrix();
                mvp = mul(viewProj, model);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_model"), 1, GL_FALSE, &model.x.x);
                glUniformMatrix4fv(glGetUniformLocation(selection.arrowProg, "u_modelViewProj"), 1, GL_FALSE, &mvp.x.x);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_diffuse"), 0.5f, 0.1f, 0.1f);
                glUniform3f(glGetUniformLocation(selection.arrowProg, "u_emissive"), 0.5f, 0.1f, 0.1f);
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
            if(auto obj = selection.object.lock())
            {
                auto localRay = Pose(obj->position, {0,0,0,1}).Inverse() * ray;
                auto hit = selection.arrowMesh.Hit(localRay);
                if(hit.hit) return std::make_shared<LinearTranslationDragger>(*obj, caster, float3(0,0,1), e.cursor);

                localRay = Pose(obj->position, RotationQuaternion({1,0,0},-1.57f)).Inverse() * ray;
                hit = selection.arrowMesh.Hit(localRay);
                if(hit.hit) return std::make_shared<LinearTranslationDragger>(*obj, caster, float3(0,1,0), e.cursor);

                localRay = Pose(obj->position, RotationQuaternion({0,1,0},+1.57f)).Inverse() * ray;
                hit = selection.arrowMesh.Hit(localRay);
                if(hit.hit) return std::make_shared<LinearTranslationDragger>(*obj, caster, float3(1,0,0), e.cursor);
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
    gui::ElementPtr         objectListPanel;
    gui::ElementPtr         propertyPanel;
    gui::ElementPtr         guiRoot;
    Mesh                    mesh,ground,bulb;

    Scene                   scene;
    Selection               selection;
    std::shared_ptr<View>   view;

    bool                    quit;

    void RefreshObjectList()
    {
        // TODO: Back up and restore selection
        objectList = std::make_shared<ListBox>(font, 2);
        for(auto & obj : scene.objects) objectList->AddItem(obj->name);
        auto it = std::find(begin(scene.objects), end(scene.objects), selection.object.lock());
        objectList->SetSelectedIndex(it != end(scene.objects) ? it - begin(scene.objects) : -1);
        objectListPanel->children = {{{{0,0},{0,0},{1,0},{1,0}}, objectList}};
        objectList->onSelectionChanged = [this]() { selection.SetSelection(scene.objects[objectList->GetSelectedIndex()]); };
        RefreshPropertyPanel();
    }

    void RefreshPropertyPanel()
    {
        std::vector<std::pair<std::string, gui::ElementPtr>> props;
        if(auto obj = selection.object.lock())
        {
            props.push_back({"Name", factory.MakeEdit(obj->name, [this](const std::string & text)
            {
                int selectedIndex = objectList->GetSelectedIndex();
                scene.objects[selectedIndex]->name = text;
                objectList->SetItemText(selectedIndex, text);
            })});
            props.push_back({"Position", factory.MakeVectorEdit(obj->position)});
            props.push_back({"Diffuse Color", factory.MakeVectorEdit(obj->color)});
            props.push_back({"Emissive Color", factory.MakeVectorEdit(obj->lightColor)});
        }
        propertyPanel->children = {{{{0,0},{0,0},{1,0},{1,0}}, factory.MakePropertyMap(props)}};
        window.RefreshLayout();
    }
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
struct PointLight
{
    vec3 position;
    vec3 color;
};
uniform PointLight u_lights[8];
uniform vec3 u_eye;
uniform vec3 u_emissive;
uniform vec3 u_diffuse;
in vec3 position;
in vec3 normal;
void main()
{
    vec3 eyeDir = normalize(u_eye - position);
    vec3 light = u_emissive;
    for(int i=0; i<8; ++i)
    {
        vec3 lightDir = normalize(u_lights[i].position - position);
        light += u_lights[i].color * max(dot(normal, lightDir), 0);

        vec3 halfDir = normalize(lightDir + eyeDir);
        light += u_lights[i].color * pow(max(dot(normal, halfDir), 0), 128);
    }
    gl_FragColor = vec4(u_diffuse*light,1);
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
        scene.CreateObject("Ground",{    0,-0.1f,  0},&ground,prog,{0.4f,0.4f,0.4f},{0,0,0});
        scene.CreateObject("Alpha", {-0.6f, 0.5f,  0},&mesh,prog,{1,0,0},{0,0,0});
        scene.CreateObject("Beta",  {+0.6f, 0.5f,  0},&mesh,prog,{0,1,0},{0,0,0});
        scene.CreateObject("Gamma", { 0.0f, 1.5f,  0},&mesh,prog,{1,1,0},{0,0,0});
        scene.CreateObject("Light", { 0.0f, 3.0f,1.0},&bulb,prog,{1,1,1},{1,1,1});
        view->viewpoint.position = {0,1,4};

        objectListPanel = std::make_shared<gui::Element>();
        propertyPanel = std::make_shared<gui::Element>();
        auto topRightPanel = Border::CreateBigBorder(objectListPanel);
        auto bottomRightPanel = Border::CreateBigBorder(propertyPanel);
        auto rightPanel = std::make_shared<Splitter>(bottomRightPanel, topRightPanel, Splitter::Top, 200);
        auto mainPanel = std::make_shared<Splitter>(view, rightPanel, Splitter::Right, 400);
        
        auto guiRoot = std::make_shared<Menu>(mainPanel, font, std::vector<MenuItem>{
            MenuItem::Popup("File", {
                {"New", [](){}},
                MenuItem::Popup("Open", {
                    {"Game", [](){}},
                    {"Level", [](){}}
                }),
                {"Save", [](){}},
                {"Exit", [this]() { quit = true; }}
            }),
            MenuItem::Popup("Edit", {
                {"Cut",   [](){}},
                {"Copy",  [](){}},
                {"Paste", [](){}}
            }),
            MenuItem::Popup("Object", {
                {"New", [this,prog]() { 
                    scene.CreateObject("New Object", {0,0,0}, &mesh, prog, {1,1,1}, {0,0,0});
                    RefreshObjectList();
                }}
            })
        });

        RefreshObjectList();
            
        selection.onSelectionChanged = [this]()
        {
            auto it = std::find(begin(scene.objects), end(scene.objects), selection.object.lock());
            objectList->SetSelectedIndex(it != end(scene.objects) ? it - begin(scene.objects) : -1);
            RefreshPropertyPanel();
        };
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