#include "window.h"
#include "widgets.h"
#include "xplat.h"

#include "engine/gl.h"
#include "engine/font.h"
#include "engine/transform.h"
#include "engine/geometry.h"

#include "nanovg.h"
#include "scene.h"

#include <algorithm>
#include <sstream>
#include <fstream>

struct Selection
{
    std::weak_ptr<Object> object;
    AssetHandle<gl::Program> selectionProgram;

    Mesh arrowMesh, circleMesh, scaleMesh;
    AssetHandle<gl::Program> arrowProg;

    Selection() {}

    void Deselect()
    {
        object.reset();
        if(onSelectionChanged) onSelectionChanged();
    }

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
    float4x4 invViewProj;
public:
    Raycaster(const gui::Rect & rect, const float4x4 & proj, const Pose & viewpoint) : rect(rect), invViewProj(inv(mul(proj, LookAtMatrixRh(viewpoint.position, viewpoint.position + viewpoint.Ydir(), viewpoint.Zdir())))) {}

    Ray ComputeRay(const int2 & pixel) const
    {
        auto viewX = (pixel.x - rect.x0) * 2.0f / rect.GetWidth() - 1;
        auto viewY = 1 - (pixel.y - rect.y0) * 2.0f / rect.GetHeight();
        return Ray::Between(TransformCoordinate(invViewProj, {viewX, viewY, -1}), TransformCoordinate(invViewProj, {viewX, viewY, 1}));
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
    LinearTranslationDragger(Object & object, const Raycaster & caster, const float3 & direction, const int2 & click) : object(object), caster(caster), direction(qrot(object.pose.orientation, direction)), initialPosition(object.pose.position), initialS(ComputeS(click)) {}

    void OnDrag(int2 newMouse) override { object.pose.position = initialPosition + direction * (ComputeS(newMouse) - initialS); }
    void OnRelease() override {}
    void OnCancel() override { object.pose.position = initialPosition; }
};

class AxisRotationDragger : public gui::IDragger
{
    Object & object;
    Raycaster caster;
    float3 axis, edge1;
    float4 initialOrientation;

    float3 ComputeEdge(const int2 & mouse) const
    {
        auto ray = caster.ComputeRay(mouse);
        auto hit = IntersectRayPlane(ray, Plane(axis, object.pose.position));
        return ray.GetPoint(hit.t) - object.pose.position;
    }
public:
    AxisRotationDragger(Object & object, const Raycaster & caster, const float3 & axis, const int2 & click) : object(object), caster(caster), axis(qrot(object.pose.orientation, axis)), initialOrientation(object.pose.orientation), edge1(ComputeEdge(click)) {}

    void OnDrag(int2 newMouse) override { object.pose.orientation = qmul(RotationQuaternionFromToVec(edge1, ComputeEdge(newMouse)), initialOrientation); }
    void OnRelease() override {}
    void OnCancel() override { object.pose.orientation = initialOrientation; }
};

class LinearScalingDragger : public gui::IDragger
{
    Object & object;
    Raycaster caster;
    float3 scaleDirection, direction, initialScale;
    float initialS;

    float ComputeS(const int2 & mouse) const
    {
        const Ray ray1 = {object.pose.position, direction}, ray2 = caster.ComputeRay(mouse);
        const auto r12 = ray2.start - ray1.start;
        const auto e1e2 = dot(ray1.direction, ray2.direction), denom = 1 - e1e2*e1e2;
        return (dot(r12,ray1.direction) - dot(r12,ray2.direction)*e1e2) / denom;
    }
public:
    LinearScalingDragger(Object & object, const Raycaster & caster, const float3 & direction, const int2 & click) : object(object), caster(caster), scaleDirection(direction), direction(qrot(object.pose.orientation, direction)), initialScale(object.localScale), initialS(ComputeS(click)) {}

    void OnDrag(int2 newMouse) override 
    { 
        float scale = ComputeS(newMouse) / initialS;
        object.localScale = initialScale + scaleDirection * ((scale - 1) * dot(initialScale, scaleDirection));
    }
    void OnRelease() override {}
    void OnCancel() override { object.localScale = initialScale; }
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

    enum Mode { Translation, Rotation, Scaling };

    Scene & scene;
    Selection & selection;
    Pose viewpoint;
    float yaw=0,pitch=0;
    bool bf=0,bl=0,bb=0,br=0;
    Mode mode=Translation;

    const Mesh & GetGizmoMesh() const
    {
        switch(mode)
        {
        default: case Translation: return selection.arrowMesh;
        case Rotation: return selection.circleMesh;
        case Scaling: return selection.scaleMesh;
        }
    }  

    View(Scene & scene, Selection & selection) : scene(scene), selection(selection) {}

    bool OnKey(GLFWwindow * window, int key, int action, int mods) override
    {
        if(action != GLFW_PRESS) return false;
        switch(key)
        {
        case GLFW_KEY_W: mode = Translation; return true;
        case GLFW_KEY_E: mode = Rotation; return true;
        case GLFW_KEY_R: mode = Scaling; return true;
        default: return false;
        }
    }

    void OnUpdate(float timestep)
    {
        float3 dir;
        if(bf) dir.y += 1;
        if(bl) dir.x -= 1;
        if(bb) dir.y -= 1;
        if(br) dir.x += 1;
        if(mag2(dir) > 0) viewpoint.position = viewpoint.TransformCoord(norm(dir) * (timestep * 8));
    }

    void OnDrag(const int2 & delta)
    {
        yaw -= delta.x * 0.01f;
        pitch -= delta.y * 0.01f;
        viewpoint.orientation = qmul(RotationQuaternionAxisAngle({0,0,1}, yaw), RotationQuaternionAxisAngle({1,0,0}, pitch));
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
        auto proj = PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.25f, 32.0f);
        auto view = LookAtMatrixRh(viewpoint.position, viewpoint.position + viewpoint.Ydir(), viewpoint.Zdir());
        auto viewProj = mul(proj, view);
    
        RenderContext ctx;
        glGenBuffers(1, &ctx.perSceneBuffer);

        scene.Draw(ctx, viewProj, viewpoint.position);

        if(auto obj = selection.object.lock())
        {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1, -1);
            auto prog = obj->prog;
            obj->prog = selection.selectionProgram;
            obj->Draw(viewProj, viewpoint.position);
            obj->prog = prog;
            glPopAttrib();

            glClear(GL_DEPTH_BUFFER_BIT);
            for(auto & axis : {float3(1,0,0), float3(0,1,0), float3(0,0,1)})
            {
                auto model = (obj->pose * Pose({0,0,0}, RotationQuaternionFromToVec({0,0,1}, axis))).Matrix();
                auto color = axis * 0.4f + 0.1f;
                glUseProgram(selection.arrowProg.GetAsset().GetObject());
                selection.arrowProg.GetAsset().Uniform("u_model", model);
                selection.arrowProg.GetAsset().Uniform("u_modelIT", model);
                selection.arrowProg.GetAsset().Uniform("u_modelViewProj", mul(viewProj, model));
                selection.arrowProg.GetAsset().Uniform("u_eye", viewpoint.position);
                selection.arrowProg.GetAsset().Uniform("u_diffuse", color);
                selection.arrowProg.GetAsset().Uniform("u_emissive", color);
                GetGizmoMesh().Draw();
            }
        }

        glPopAttrib();

        if(e.hasFocus)
        {
	        nvgBeginPath(e.vg);
	        nvgRect(e.vg, rect.x0+1.5f, rect.y0+1.5f, rect.GetWidth()-3.0f, rect.GetHeight()-3.0f);
	        nvgStrokeColor(e.vg, nvgRGBA(255,255,255,128));
            nvgStrokeWidth(e.vg, 1);
	        nvgStroke(e.vg);
        }

        return {};
    }

    gui::DraggerPtr CreateGizmoDragger(Object & obj, Raycaster caster, const float3 & axis, const int2 & cursor)
    {
        switch(mode)
        {
        default: case Translation: return std::make_shared<LinearTranslationDragger>(obj, caster, axis, cursor);
        case Rotation: return std::make_shared<AxisRotationDragger>(obj, caster, axis, cursor);
        case Scaling: return std::make_shared<LinearScalingDragger>(obj, caster, axis, cursor);
        }
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
                gui::DraggerPtr best; float bestT;
                for(auto & axis : {float3(1,0,0), float3(0,1,0), float3(0,0,1)})
                {
                    auto localRay = (obj->pose * Pose({0,0,0}, RotationQuaternionFromToVec({0,0,1}, axis))).Inverse() * ray;
                    auto hit = GetGizmoMesh().Hit(localRay);
                    if(hit.hit && (!best || hit.t < bestT))
                    {
                        best = CreateGizmoDragger(*obj, caster, axis, e.cursor);
                        bestT = hit.t;
                    }                    
                }
                if(best) return best;
            }

            // Otherwise see if we have selected a new object
            if(auto obj = scene.Hit(ray)) selection.SetSelection(obj);
            else selection.Deselect();

            // If we did not click on an object directly, perhaps we should box select?             
        }

        return nullptr;
    }
};

Mesh MakeBox(const float3 & halfDims)
{
    Mesh mesh;
    mesh.AddBox(-halfDims, +halfDims);
    mesh.ComputeNormals();
    mesh.Upload();
    return mesh;
}

std::string LoadTextFile(const std::string & filename)
{
    std::string contents;
    FILE * f = fopen(filename.c_str(), "rb");
    if(f)
    {
        fseek(f, 0, SEEK_END);
        auto len = ftell(f);
        contents.resize(len);
        fseek(f, 0, SEEK_SET);
        fread(&contents[0], 1, contents.size(), f);
        fclose(f);
    }
    return contents;
}

#include <iostream>

class Editor
{
    Window                  window;      
    AssetLibrary            assets;
    Font                    font;
    GuiFactory              factory;
    std::shared_ptr<ListBox>objectList;
    gui::ElementPtr         objectListPanel;
    gui::ElementPtr         propertyPanel;
    gui::ElementPtr         guiRoot;

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
        objectList->onSelectionChanged = [this]() { if(objectList->GetSelectedIndex() >= 0) selection.SetSelection(scene.objects[objectList->GetSelectedIndex()]); else selection.Deselect(); };
        RefreshPropertyPanel();
    }

    void RefreshPropertyPanel()
    {
        propertyPanel->children.clear();

        if(auto obj = selection.object.lock())
        {
            std::vector<std::pair<std::string, gui::ElementPtr>> props;
            props.push_back({"Name", factory.MakeEdit(obj->name, [this](const std::string & text)
            {
                int selectedIndex = objectList->GetSelectedIndex();
                scene.objects[selectedIndex]->name = text;
                objectList->SetItemText(selectedIndex, text);
            })});
            props.push_back({"Position", factory.MakeVectorEdit(obj->pose.position)});
            props.push_back({"Orientation", factory.MakeVectorEdit(obj->pose.orientation)});
            props.push_back({"Scale", factory.MakeVectorEdit(obj->localScale)});
            props.push_back({"Diffuse Color", factory.MakeVectorEdit(obj->color)});
            auto pmap = factory.MakePropertyMap(props);
            float y0 = pmap->GetMinimumSize().y;
            propertyPanel->AddChild({{0,0},{0,0},{1,0},{0,y0}}, pmap);

            if(obj->light)
            {
                props.clear();
                props.push_back({"Emissive Color", factory.MakeVectorEdit(obj->light->color)});
                pmap = Border::CreateBigBorder(factory.MakePropertyMap(props));

                y0 += 8;
                auto y1 = y0+font.GetLineHeight();
                propertyPanel->AddChild({{0,0},{0,y0},{1,0},{0,y1}}, factory.MakeLabel("Light Component:"));
                y0 = y1;
                y1 += pmap->GetMinimumSize().y;
                propertyPanel->AddChild({{0,0},{0,y0},{1,0},{0,y1}}, pmap);
            }
        }

        window.RefreshLayout();
    }
public:
    Editor() : window("Editor", 1280, 720), font(window.GetNanoVG(), "../assets/Roboto-Bold.ttf", 18, true, 0x500), factory(font, 2), quit()
    {
        view = std::make_shared<View>(scene, selection);

        auto vs = R"(#version 330
uniform mat4 u_model;
uniform mat4 u_modelIT;
uniform mat4 u_modelViewProj;
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
out vec3 position;
out vec3 normal;
void main()
{
    gl_Position = u_modelViewProj * vec4(v_position,1);
    position = (u_model * vec4(v_position,1)).xyz;
    normal = normalize((u_modelIT * vec4(v_normal,0)).xyz);
}
)";

        auto fs = R"(#version 330
struct PointLight
{
    vec3 position;
    vec3 color;
};
uniform PerScene
{
    PointLight u_lights[8];
};
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
        light += u_lights[i].color * u_diffuse * max(dot(normal, lightDir), 0);

        vec3 halfDir = normalize(lightDir + eyeDir);
        light += u_lights[i].color * u_diffuse * pow(max(dot(normal, halfDir), 0), 128);
    }
    gl_FragColor = vec4(light,1);
}
)";

        auto prog = assets.RegisterProgram("diffuse", gl::Program(vs, fs));
        selection.selectionProgram = assets.RegisterProgram("selection", gl::Program(vs, "#version 330\nvoid main() { gl_FragColor = vec4(1,1,1,1); }"));

        for(auto & block : prog.GetAsset().GetBlocks())
        {
            std::cout << "Block " << block.index << ": " << block.name << " (" << block.dataSize << " B)" << std::endl;
            for(auto & uniform : block.uniforms)
            {
                std::cout << "  " << uniform.offset << ": " << uniform.name << " : " << uniform.type << "[" << uniform.size << "] " << uniform.arrayStride << "/" << uniform.matrixStride << std::endl;
            }
        }

        selection.arrowMesh.AddCylinder({0,0,0}, 0.00f, {0,0,0}, 0.05f, {1,0,0}, {0,1,0}, 12);
        selection.arrowMesh.AddCylinder({0,0,0}, 0.05f, {0,0,1}, 0.05f, {1,0,0}, {0,1,0}, 12);
        selection.arrowMesh.AddCylinder({0,0,1}, 0.05f, {0,0,1}, 0.10f, {1,0,0}, {0,1,0}, 12);
        selection.arrowMesh.AddCylinder({0,0,1}, 0.10f, {0,0,1.2f}, 0.0f, {1,0,0}, {0,1,0}, 12);
        selection.arrowMesh.ComputeNormals();
        selection.arrowMesh.Upload();
        selection.arrowProg = prog;

        selection.circleMesh.AddCylinder({0,0,-0.02f}, 0.90f, {0,0,-0.02f}, 1.00f, {1,0,0}, {0,1,0}, 32);
        selection.circleMesh.AddCylinder({0,0,-0.02f}, 1.00f, {0,0,+0.02f}, 1.00f, {1,0,0}, {0,1,0}, 32);
        selection.circleMesh.AddCylinder({0,0,+0.02f}, 1.00f, {0,0,+0.02f}, 0.90f, {1,0,0}, {0,1,0}, 32);
        selection.circleMesh.AddCylinder({0,0,+0.02f}, 0.90f, {0,0,-0.02f}, 0.90f, {1,0,0}, {0,1,0}, 32);
        selection.circleMesh.ComputeNormals();
        selection.circleMesh.Upload();

        selection.scaleMesh.AddCylinder({0,0,0}, 0.00f, {0,0,0}, 0.05f, {1,0,0}, {0,1,0}, 12);
        selection.scaleMesh.AddCylinder({0,0,0}, 0.05f, {0,0,1}, 0.05f, {1,0,0}, {0,1,0}, 12);
        selection.scaleMesh.AddBox({-0.1f,-0.1f,1}, {+0.1f,+0.1f,1.2f});
        selection.scaleMesh.ComputeNormals();
        selection.scaleMesh.Upload();

        auto mesh = assets.RegisterMesh("box", MakeBox({0.5f,0.5f,0.5f}));
        auto ground = assets.RegisterMesh("ground", MakeBox({4,4,0.1f}));
        auto bulb = assets.RegisterMesh("bulb", MakeBox({0.1f,0.1f,0.1f}));
        scene.CreateObject("Ground",{    0,0,-0.1f},ground,prog,{0.4f,0.4f,0.4f});
        scene.CreateObject("Alpha", {-0.6f,0, 0.5f},mesh,prog,{1,0,0});
        scene.CreateObject("Beta",  {+0.6f,0, 0.5f},mesh,prog,{0,1,0});
        scene.CreateObject("Gamma", { 0.0f,0, 1.5f},mesh,prog,{1,1,0});
        auto lt = scene.CreateObject("Light", { 0.0f,-1.0f,3.0f},bulb,prog,{0,0,0});
        lt->light = std::make_unique<LightComponent>();
        lt->light->color = {1,1,1};
        view->viewpoint.position = {0,-4,1};

        objectListPanel = std::make_shared<gui::Element>();
        propertyPanel = std::make_shared<gui::Element>();
        auto topRightPanel = Border::CreateBigBorder(objectListPanel);
        auto bottomRightPanel = Border::CreateBigBorder(propertyPanel);
        auto rightPanel = std::make_shared<Splitter>(bottomRightPanel, topRightPanel, Splitter::Top, 200);
        auto mainPanel = std::make_shared<Splitter>(view, rightPanel, Splitter::Right, 400);
        
        RefreshObjectList();
            
        selection.onSelectionChanged = [this]()
        {
            auto it = std::find(begin(scene.objects), end(scene.objects), selection.object.lock());
            objectList->SetSelectedIndex(it != end(scene.objects) ? it - begin(scene.objects) : -1);
            RefreshPropertyPanel();
        };
        selection.onSelectionChanged();

        window.SetGuiRoot(mainPanel, font, std::vector<MenuItem>{
            MenuItem::Popup("File", {
                {"New", [](){}, GLFW_MOD_CONTROL, GLFW_KEY_N},
                MenuItem::Popup("Open", {
                    {"Game", [](){}},
                    {"Level", [this](){ 
                        auto f = ChooseFile({{"Scene files","scene"}}, true);
                        if(f.empty()) return;
                        scene.FromJson(assets, jsonFrom(LoadTextFile(f)));
                        RefreshObjectList();
                    }}
                }),
                {"Save", [this](){ 
                    auto f = ChooseFile({{"Scene files","scene"}}, false);
                    if(!f.empty()) std::ofstream(f, std::ofstream::binary) << tabbed(scene.ToJson(), 4);
                }, GLFW_MOD_CONTROL, GLFW_KEY_S},
                {"Exit", [this]() { quit = true; }, GLFW_MOD_ALT, GLFW_KEY_F4}
            }),
            MenuItem::Popup("Edit", {
                {"Cut",   [](){}, GLFW_MOD_CONTROL, GLFW_KEY_X},
                {"Copy",  [](){}, GLFW_MOD_CONTROL, GLFW_KEY_C},
                {"Paste", [](){}, GLFW_MOD_CONTROL, GLFW_KEY_V}
            }),
            MenuItem::Popup("Object", {
                {"New", [this]() { 
                    scene.CreateObject("New Object", {0,0,0}, assets.GetMesh("box"), assets.GetProgram("diffuse"), {1,1,1});
                    RefreshObjectList();
                }},
                {"Duplicate", [this]() { 
                    auto obj = scene.DuplicateObject(*selection.object.lock());
                    RefreshObjectList();
                    selection.SetSelection(obj);
                }, GLFW_MOD_CONTROL, GLFW_KEY_D},
                {"Delete", [this]() { 
                    scene.DeleteObject(selection.object.lock());
                    RefreshObjectList();
                }, 0, GLFW_KEY_DELETE},
                MenuItem::Popup("Components", {
                    {"Add Light", [this]() { 
                        if(auto obj = selection.object.lock())
                        {
                            obj->light = std::make_unique<LightComponent>();
                        }
                    }}
                })
            })
        });
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