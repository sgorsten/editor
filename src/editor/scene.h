#ifndef EDITOR_SCENE_H
#define EDITOR_SCENE_H

#include "engine/gl.h"
#include "engine/geometry.h"

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
    std::vector<Object> objects;

    Object * Hit(const Ray & ray);

    void Draw(const float4x4 & viewProj, const float3 & eye);
};



#endif