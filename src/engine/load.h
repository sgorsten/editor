#ifndef ENGINE_LOAD_H
#define ENGINE_LOAD_H

#include "gl.h"
#include "geometry.h"

struct Vertex { float3 position, normal; float2 texCoord; };

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
        uint32_t base = vertices.size();
        for(uint32_t i=0, n=segments; i<n; ++i)
        {
            const float angle = i*6.28f/segments;
            auto dir = axisA * std::cos(angle) + axisB * std::sin(angle);
            vertices.push_back({center0 + dir*radius0, dir});
            vertices.push_back({center1 + dir*radius1, dir});
            uint32_t i0 = i*2, i1 = i0 + 1, i2 = (i0 + 2) % (segments*2), i3 = (i0 + 3) % (segments*2);
            triangles.push_back({base + i0, base + i3, base + i1});
            triangles.push_back({base + i0, base + i2, base + i3});
        }
    }

    void AddBox(const float3 & b0, const float3 & b1)
    {
        static const float3 verts[] = {{1,0,0}, {1,1,0}, {1,1,1}, {1,0,1}, {0,1,0}, {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0}, {0,0,1}, {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}, {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0}};
        static const uint3 tris[] = {{0,1,2}, {0,2,3}, {4,5,6}, {4,6,7}, {8,9,10}, {8,10,11}, {12,13,14}, {12,14,15}, {16,17,18}, {16,18,19}, {20,21,22}, {20,22,23}};
        uint32_t base = vertices.size();
        for(auto & vert : verts) vertices.push_back({{b0 + vert * (b1-b0)},{}});
        for(auto & tri : tris) triangles.push_back(tri + base);
    }
};

Mesh LoadMeshFromObj(const std::string & filepath, bool swapYZ);
std::string LoadTextFile(const std::string & filename);

#endif