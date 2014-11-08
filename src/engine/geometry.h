#ifndef ENGINE_GEOMETRY_H
#define ENGINE_GEOMETRY_H

#include "linalg.h"
#include "transform.h"

struct Ray
{
    float3 start, direction; 
    static Ray Between(const float3 & start, const float3 & end) { return {start,norm(end-start)}; }
    friend Ray operator * (const Pose & pose, const Ray & ray) { return {pose.TransformCoord(ray.start), pose.TransformVector(ray.direction)}; }
};

struct RayTriHit { bool hit; float t,u,v; };
RayTriHit IntersectRayTriangle(const Ray & ray, const float3 & vertex0, const float3 & vertex1, const float3 & vertex2);

struct RayMeshHit : RayTriHit { size_t triangle; RayMeshHit(const RayTriHit & hit, size_t triangle) : RayTriHit(hit), triangle(triangle) {} };
template<class VERTEX> RayMeshHit IntersectRayMesh(const Ray & ray, const VERTEX * verts, float3 (VERTEX::*position), const uint3 * tris, size_t numTris)
{
    RayMeshHit best = {{false},0};
    for(size_t i=0; i<numTris; ++i)
    {
        auto & tri = tris[i];
        auto hit = IntersectRayTriangle(ray, verts[tri.x].*position, verts[tri.y].*position, verts[tri.z].*position);
        if(hit.hit && (!best.hit || hit.t < best.t)) best = {hit,i};
    }
    return best;
}

#endif