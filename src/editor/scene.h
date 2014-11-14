#ifndef EDITOR_SCENE_H
#define EDITOR_SCENE_H

#include "engine/gl.h"
#include "engine/geometry.h"

#include <memory>
#include <vector>

struct PointLight { float3 position, color; };
struct LightEnvironment
{
    std::vector<PointLight> lights;
    void Bind(GLuint program) const;
};

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

    void Upload();
    void Draw();
};

struct Object
{
    std::string name;
    float3 position;

    float3 color;
    Mesh * mesh;
    GLuint prog;

    float3 lightColor;

    RayMeshHit Hit(const Ray & ray) const { return mesh->Hit({ray.start-position,ray.direction}); }

    void Draw(const float4x4 & viewProj, const float3 & eye, const LightEnvironment & lights);
};

struct Scene
{
    std::vector<std::shared_ptr<Object>> objects;

    std::shared_ptr<Object> Hit(const Ray & ray);

    void Draw(const float4x4 & viewProj, const float3 & eye);

    std::shared_ptr<Object> CreateObject(std::string name, const float3 & position, Mesh * mesh, GLuint prog, const float3 & diffuseColor, const float3 & emissiveColor)
    {
        auto obj = std::make_shared<Object>();
        obj->name = name;
        obj->position = position;
        obj->mesh = mesh;
        obj->prog = prog;
        obj->color = diffuseColor;
        obj->lightColor = emissiveColor;
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