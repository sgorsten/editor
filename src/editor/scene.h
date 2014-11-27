#ifndef EDITOR_SCENE_H
#define EDITOR_SCENE_H

#include "engine/gl.h"
#include "engine/geometry.h"
#include "engine/json.h"
#include "asset.h"

#include <vector>

struct PointLight { float3 position, color; };
struct LightEnvironment
{
    std::vector<PointLight> lights;
    void Bind(GLuint program) const;
};

struct LightComponent
{
    float3 color;
};

inline JsonArray ToJson(const float3 & obj) { return {obj.x, obj.y, obj.z}; }
inline JsonArray ToJson(const float4 & obj) { return {obj.x, obj.y, obj.z, obj.w}; }
inline JsonArray ToJson(const Pose & obj) { return {ToJson(obj.position), ToJson(obj.orientation)}; }

inline void FromJson(const JsonValue & val, float & obj) { obj = val.number<float>(); }
inline void FromJson(const JsonValue & val, float3 & obj) { FromJson(val[0], obj.x); FromJson(val[1], obj.y); FromJson(val[2], obj.z); }
inline void FromJson(const JsonValue & val, float4 & obj) { FromJson(val[0], obj.x); FromJson(val[1], obj.y); FromJson(val[2], obj.z); FromJson(val[3], obj.w); }
inline void FromJson(const JsonValue & val, Pose & obj) { FromJson(val[0], obj.position); FromJson(val[1], obj.orientation); }

struct Object
{
    std::string name;
    Pose pose;
    float3 localScale = float3(1,1,1);

    float3 color;
    AssetHandle<Mesh> mesh;
    AssetHandle<GLuint> prog;

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

    void Draw(const float4x4 & viewProj, const float3 & eye, const LightEnvironment & lights);

    JsonValue ToJson() const
    {
        auto obj = JsonObject{
            {"name", name},
            {"pose", ::ToJson(pose)},
            {"scale", ::ToJson(localScale)},
            {"diffuse", ::ToJson(color)}
        };
        if(mesh.IsValid()) obj.push_back({"mesh", mesh.GetId()});
        if(prog.IsValid()) obj.push_back({"prog", prog.GetId()});
        if(light) obj.push_back({"light", JsonObject{{"color", ::ToJson(light->color)}}});
        return obj;
    }

    void FromJson(const AssetLibrary & lib, const JsonValue & val)
    {
        name = val["name"].string();
        ::FromJson(val["pose"], pose);
        ::FromJson(val["scale"], localScale);
        ::FromJson(val["diffuse"], color);
        mesh = lib.GetMesh(val["mesh"].string());
        prog = lib.GetProgram(val["prog"].string());
        if(val["light"].isObject())
        {
            light = std::make_unique<LightComponent>();
            ::FromJson(val["light"]["color"], light->color);
        }
    }
};

struct Scene
{
    std::vector<std::shared_ptr<Object>> objects;

    JsonValue ToJson() const
    {
        JsonArray objs;
        for(auto & obj : objects) objs.push_back(obj->ToJson());
        return JsonObject{{"objects", objs}};
    }

    void FromJson(const AssetLibrary & lib, const JsonValue & val)
    {
        objects.clear();
        for(auto & elem : val["objects"].array())
        {
            auto obj = std::make_shared<Object>();
            obj->FromJson(lib, elem);
            objects.push_back(obj);
        }
    }

    std::shared_ptr<Object> Hit(const Ray & ray);

    void Draw(const float4x4 & viewProj, const float3 & eye);

    std::shared_ptr<Object> CreateObject(std::string name, const float3 & position, AssetHandle<Mesh> mesh, AssetHandle<GLuint> prog, const float3 & diffuseColor)
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



#endif