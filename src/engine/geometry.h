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

#endif