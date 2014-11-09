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

    gl::Buffer vb,ib;

    Mesh() {}
    Mesh(Mesh && m) : Mesh() { *this = std::move(m); }
    Mesh & operator = (Mesh && m) { vertices=move(m.vertices); triangles=move(m.triangles); vb=std::move(m.vb); ib=std::move(m.ib); return *this; }

    RayMeshHit Hit(const Ray & ray) const { return IntersectRayMesh(ray, vertices.data(), &Vertex::position, triangles.data(), triangles.size()); }

    void Upload()
    {
        vb.Bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), vertices.data(), GL_STATIC_DRAW);

        ib.Bind(GL_ELEMENT_ARRAY_BUFFER);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint3)*triangles.size(), triangles.data(), GL_STATIC_DRAW);
    }

    void Draw()
    {
        const Vertex * null = 0;
        vb.Bind(GL_ARRAY_BUFFER);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &null->position);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &null->normal);

        ib.Bind(GL_ELEMENT_ARRAY_BUFFER);
        glDrawElements(GL_TRIANGLES, triangles.size()*3, GL_UNSIGNED_INT, nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

        glEnable(GL_DEPTH_TEST);
        auto viewProj = mul(PerspectiveMatrixRhGl(1, rect.GetAspect(), 0.25f, 32.0f), viewpoint.Inverse().Matrix());
        scene.Draw(viewProj, viewpoint.position);

        if(selection.object)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1, -1);
            auto col = selection.object->color;
            selection.object->color = {1,1,1};
            selection.object->Draw(viewProj, viewpoint.position, scene.objects.back().position);
            selection.object->color = col;
        }
        
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
    Window              window;      
    Font                font;
    GuiFactory          factory;
    ListControl         objectList;
    gui::ElementPtr     propertyPanel;
    gui::ElementPtr     guiRoot;
    Mesh                mesh,ground,bulb;

    Scene               scene;
    Selection           selection;
    View                view;
public:
    Editor() : window("Editor", 1280, 720), font(window.GetNanoVG(), "../assets/Roboto-Bold.ttf", 18, true, 0x500), factory(font, 2), view(scene, selection)
    {
        const char * vsSource[] = {R"(#version 330
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
)"};

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, vsSource, nullptr);
        glCompileShader(vs);

        GLint status, length;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
        if(status == GL_FALSE)
        {
            glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> buffer(length);
            glGetShaderInfoLog(vs, length, nullptr, buffer.data());
            throw std::runtime_error(buffer.data());
        }

        const char * fsSource[] = {R"(#version 330
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
    light += pow(max(dot(normal, halfDir), 0), 64);
    gl_FragColor = vec4(u_color*light,1);
}
)"};

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, fsSource, nullptr);
        glCompileShader(fs);

        glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
        if(status == GL_FALSE)
        {
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> buffer(length);
            glGetShaderInfoLog(fs, length, nullptr, buffer.data());
            throw std::runtime_error(buffer.data());
        }

        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);

        glGetProgramiv(prog, GL_LINK_STATUS, &status);
        if(status == GL_FALSE) throw std::runtime_error("Link error.");

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