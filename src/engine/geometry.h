#ifndef ENGINE_GEOMETRY_H
#define ENGINE_GEOMETRY_H

#include "linalg.h"
#include "transform.h"

struct Plane
{
    float4 coeff; // Coefficients of plane equation, in ax * by * cz + d form
    Plane(const float3 & axis, const float3 & point) : coeff(axis, -dot(axis,point)) {}
    const float3 & GetNormal() const { return coeff.xyz(); }
};

struct Ray
{
    float3 start, direction; 
    float3 GetPoint(float t) const { return start + direction * t; }
    static Ray Between(const float3 & start, const float3 & end) { return {start,norm(end-start)}; }
    friend Ray operator * (const Pose & pose, const Ray & ray) { return {pose.TransformCoord(ray.start), pose.TransformVector(ray.direction)}; }
};

struct RayPlaneHit { bool hit; float t; };
RayPlaneHit IntersectRayPlane(const Ray & ray, const Plane & plane);

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