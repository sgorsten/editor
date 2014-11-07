#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include "linalg.h"

float4 RotationQuaternion(const float3 & axisOfRotation, float angleInRadians);
float4x4 PerspectiveMatrixRhGl(float verticalFieldOfViewInRadians, float aspectRatioWidthOverHeight, float nearClipDistance, float farClipDistance);

#endif