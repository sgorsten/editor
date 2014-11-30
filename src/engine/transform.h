#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include "linalg.h"

inline float3 TransformDirection(const float4x4 & transformMatrix, const float3 & direction) { return mul(transformMatrix, float4(direction,0)).xyz(); } // Works for rigid transforms only
inline float3 TransformCoordinate(const float4x4 & transformMatrix, const float3 & coordinate) { auto r = mul(transformMatrix, float4(coordinate,1)); return r.xyz() / r.w; }

float4 RotationQuaternionAxisAngle(const float3 & axisOfRotation, float angleInRadians);
float4 RotationQuaternionFromToVec(const float3 & fromVector, const float3 & toVector);

float4x4 TranslationMatrix(const float3 & translationVec);
float4x4 RigidTransformationMatrix(const float4 & rotationQuat, const float3 & translationVec);
float4x4 ScaledTransformationMatrix(const float3 & scalingFactors, const float4 & rotationQuat, const float3 & translationVec);
float4x4 PerspectiveMatrixRhGl(float verticalFieldOfViewInRadians, float aspectRatioWidthOverHeight, float nearClipDistance, float farClipDistance);
float4x4 LookAtMatrixRh(const float3 & eye, const float3 & center, const float3 & up);

struct Pose
{
    float3      position;
    float4      orientation;

                Pose(const float3 & pos, const float4 & ori)   : position(pos), orientation(ori) {}
                Pose()                                         : Pose({0,0,0}, {0,0,0,1}) {}
                Pose(const float3 & position)                  : Pose(position, {0,0,0,1}) {}
                Pose(const float4 & orientation)               : Pose({0,0,0}, orientation) {}

    float3      TransformVector(const float3 & vec) const      { return qrot(orientation, vec); }
    float3      TransformCoord(const float3 & coord) const     { return position + TransformVector(coord); }

    Pose        operator * (const Pose & pose) const           { return {TransformCoord(pose.position), qmul(orientation,pose.orientation)}; }
    float3      operator * (const float3 & coord) const        { return TransformCoord(coord); }

    Pose        Inverse() const                                { auto invOri = qinv(orientation); return {qrot(invOri, -position), invOri}; }
    float4x4    Matrix() const                                 { return RigidTransformationMatrix(orientation, position); }
    float3      Xdir() const                                   { return qxdir(orientation); } // Equivalent to TransformVector({1,0,0})
    float3      Ydir() const                                   { return qydir(orientation); } // Equivalent to TransformVector({0,1,0})
    float3      Zdir() const                                   { return qzdir(orientation); } // Equivalent to TransformVector({0,0,1})
};

#endif