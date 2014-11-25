#include "geometry.h"

RayPlaneHit IntersectRayPlane(const Ray & ray, const Plane & plane)
{
    auto denom = dot(ray.direction, plane.GetNormal());
    if(std::abs(denom) < 0.0001f) return {false};
    return {true, -(dot(ray.start, plane.GetNormal()) + plane.coeff.w) / denom};    
}

RayTriHit IntersectRayTriangle(const Ray & ray, const float3 & v0, const float3 & v1, const float3 & v2)
{
    auto e1 = v1 - v0, e2 = v2 - v0, h = cross(ray.direction, e2);
    auto a = dot(e1, h);
    if (a > -0.0001f && a < 0.0001f) return {false};

    float f = 1 / a;
    auto s = ray.start - v0;
	auto u = f * dot(s, h);
	if (u < 0 || u > 1) return {false};

	auto q = cross(s, e1);
	auto v = f * dot(ray.direction, q);
	if (v < 0 || u + v > 1) return {false};

    auto t = f * dot(e2, q);
    if(t < 0) return {false};

    return {true,t,u,v};
}