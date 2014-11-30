#ifndef EDITOR_SCENE_H
#define EDITOR_SCENE_H

#include "engine/gl.h"
#include "engine/geometry.h"
#include "engine/pack.h"

#include <vector>

struct PointLight { float3 position, color; };
struct LightEnvironment
{
    std::vector<PointLight> lights;
    void Bind(gl::Buffer & buffer, const gl::BlockDesc & perScene) const;
};

struct LightComponent
{
    float3 color;
};

struct Object
{
    std::string name;
    Pose pose;
    float3 localScale = float3(1,1,1);

    float3 color;
    AssetHandle<Mesh> mesh;
    AssetHandle<gl::Program> prog;

    std::unique_ptr<LightComponent> light;

    Object() {}
    Object(const Object & r) : name(r.name), pose(r.pose), localScale(r.localScale), color(r.color), mesh(r.mesh), prog(r.prog), light(r.light ? std::make_unique<LightComponent>(*r.light) : nullptr) {}

    RayMeshHit Hit(const Ray & ray) const
    { 
        if(!mesh.IsValid()) return RayMeshHit({false}, 0);
        auto localRay = pose.Inverse() * ray;
        localRay.start /= localScale;
        localRay.direction /= localScale;
        return mesh.GetAsset().Hit(localRay); 
    }

    void Draw();
};

struct RenderContext
{
    gl::Buffer perScene;
};

struct Scene
{
    std::vector<std::shared_ptr<Object>> objects;

    JsonValue ToJson() const;

    void FromJson(const AssetLibrary & lib, const JsonValue & val);

    std::shared_ptr<Object> Hit(const Ray & ray);

    void Draw(RenderContext & ctx);

    std::shared_ptr<Object> CreateObject(std::string name, const float3 & position, AssetHandle<Mesh> mesh, AssetHandle<gl::Program> prog, const float3 & diffuseColor)
    {
        auto obj = std::make_shared<Object>();
        obj->name = name;
        obj->pose.position = position;
        obj->pose.orientation = {0,0,0,1};
        obj->mesh = mesh;
        obj->prog = prog;
        obj->color = diffuseColor;
        objects.push_back(obj);
        return obj;
    }

    std::shared_ptr<Object> DuplicateObject(const Object & original)
    {
        auto obj = std::make_shared<Object>(original);
        objects.push_back(obj);
        return obj;    
    }

    void DeleteObject(std::shared_ptr<Object> object)
    {
        auto it = std::find(begin(objects), end(objects), object);
        if(it != end(objects)) objects.erase(it);
    }
};

template<class F> void VisitFields(LightComponent & o, F f)
{
    f("color", o.color);
}

template<class F> void VisitFields(Object & o, F f)
{
    f("name", o.name);
    f("pose", o.pose);
    f("scale", o.localScale);
    f("diffuse", o.color);
    f("mesh", o.mesh);
    f("prog", o.prog);
    f("light", o.light);
}

template<class F> void VisitFields(Scene & o, F f)
{
    f("objects", o.objects);
}

inline JsonValue Scene::ToJson() const
{
    JsonSerializer j;
    return j.Save(*this);
}

inline void Scene::FromJson(const AssetLibrary & lib, const JsonValue & val)
{
    JsonDeserializer j(lib);
    j.Load(*this, val);
}

#endif