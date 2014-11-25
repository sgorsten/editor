#include "transform.h"

float4 RotationQuaternionAxisAngle(const float3 & axisOfRotation, float angleInRadians)
{
    return {axisOfRotation * std::sin(angleInRadians/2), std::cos(angleInRadians/2)};
}

float4 RotationQuaternionFromToVec(const float3 & fromVector, const float3 & toVector)
{
    auto a = norm(fromVector), b = norm(toVector);
    return RotationQuaternionAxisAngle(normz(cross(a,b)), std::acos(dot(a,b)));
}

float4x4 TranslationMatrix(const float3 & translation)
{
    return {{1,0,0,0},{0,1,0,0},{0,0,1,0},{translation,1}};
}

float4x4 RigidTransformationMatrix(const float4 & rot, const float3 & vec)
{
    return {{qxdir(rot),0},{qydir(rot),0},{qzdir(rot),0},{vec,1}};
}

float4x4 ScaledTransformationMatrix(const float3 & scale, const float4 & rot, const float3 & vec)
{
    return {{qxdir(rot)*scale.x,0},{qydir(rot)*scale.y,0},{qzdir(rot)*scale.z,0},{vec,1}};
}

float4x4 PerspectiveMatrixRhGl(float verticalFieldOfViewInRadians, float aspectRatioWidthOverHeight, float nearClipDistance, float farClipDistance)
{
    const auto yf = 1/std::tan(verticalFieldOfViewInRadians/2), xf = yf/aspectRatioWidthOverHeight, dz = nearClipDistance-farClipDistance;
    return {{xf,0,0,0}, {0,yf,0,0}, {0,0,(nearClipDistance+farClipDistance)/dz,-1}, {0,0,2*nearClipDistance*farClipDistance/dz,0}};
}