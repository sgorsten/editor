#include "transform.h"

#include <cmath>

float4 RotationQuaternion(const float3 & axisOfRotation, float angleInRadians)
{
    return {axisOfRotation * std::sin(angleInRadians/2), std::cos(angleInRadians/2)};
}

float4x4 PerspectiveMatrixRhGl(float verticalFieldOfViewInRadians, float aspectRatioWidthOverHeight, float nearClipDistance, float farClipDistance)
{
    const auto yf = 1/std::tan(verticalFieldOfViewInRadians/2), xf = yf/aspectRatioWidthOverHeight, dz = nearClipDistance-farClipDistance;
    return {{xf,0,0,0}, {0,yf,0,0}, {0,0,(nearClipDistance+farClipDistance)/dz,-1}, {0,0,2*nearClipDistance*farClipDistance/dz,0}};
}