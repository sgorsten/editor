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
    void Draw() const;

    void ComputeNormals()
    {
        for(auto & vert : vertices) vert.normal = float3(0,0,0);
        for(auto & tri : triangles)
        {
            auto & v0 = vertices[tri.x], & v1 = vertices[tri.y], & v2 = vertices[tri.z];
            auto n = cross(v1.position - v0.position, v2.position - v0.position);
            v0.normal += n; v1.normal += n; v2.normal += n;
        }
        for(auto & vert : vertices) vert.normal = norm(vert.normal);
    }

    void AddCylinder(const float3 & center0, float radius0, const float3 & center1, float radius1, const float3 & axisA, const float3 & axisB, int segments)
    {
        auto base = vertices.size();
        for(uint32_t i=0; i<segments; ++i)
        {
            const float angle = i*6.28f/segments;
            auto dir = axisA * std::cos(angle) + axisB * std::sin(angle);
            vertices.push_back({center0 + dir*radius0, dir});
            vertices.push_back({center1 + dir*radius1, dir});
            auto i0 = i*2, i1 = i0 + 1, i2 = (i0 + 2) % (segments*2), i3 = (i0 + 3) % (segments*2);
            triangles.push_back({base + i0, base + i3, base + i1});
            triangles.push_back({base + i0, base + i2, base + i3});
        }
    }
};

struct Object
{
    std::string name;
    Pose pose;

    float3 color;
    Mesh * mesh;
    GLuint prog;

    float3 lightColor;

    RayMeshHit Hit(const Ray & ray) const { return mesh->Hit(pose.Inverse() * ray); }

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
        obj->pose.position = position;
        obj->pose.orientation = {0,0,0,1};
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