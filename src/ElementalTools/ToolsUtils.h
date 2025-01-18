#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemSpan.h"

struct ToolsMessageList
{
    Span<ElemToolsMessage> Messages;
    uint32_t MessageCount;
    bool HasErrors;
};

ReadOnlySpan<uint8_t> LoadFileData(const char* path);
void ResetLoadFileDataMemory();

void WriteToMessageList(ElemToolsMessageType type, const char* message, ToolsMessageList* messageList);

ElemToolsMessageSpan ConstructErrorMessageSpan(MemoryArena memoryArena, const char* errorMessage);

void AddPointToBoundingBox(ElemToolsVector3 point, ElemToolsBoundingBox* boundingBox);
void AddBoundingBoxToBoundingBox(const ElemToolsBoundingBox* additional, ElemToolsBoundingBox* boundingBox);
ElemToolsVector3 GetBoundingBoxCenter(const ElemToolsBoundingBox* boundingBox);

ElemToolsVector3 ElemToolsMulScalarV3(ElemToolsVector3 v, float scalar);
ElemToolsVector3 ElemToolsMulV3(ElemToolsVector3 v1, ElemToolsVector3 v2);
float ElemToolsDotProductV3(ElemToolsVector3 v1, ElemToolsVector3 v2);
float ElemToolsMagnitudeSquaredV3(ElemToolsVector3 v);
float ElemToolsMagnitudeV3(ElemToolsVector3 v);
ElemToolsVector3 ElemToolsNormalizeV3(ElemToolsVector3 v);
ElemToolsVector3 ElemToolsCrossProductV3(ElemToolsVector3 v1, ElemToolsVector3 v2);

ElemToolsVector2 operator +(const ElemToolsVector2& v1, const ElemToolsVector2& v2);
ElemToolsVector2 operator -(const ElemToolsVector2& v1, const ElemToolsVector2& v2);

ElemToolsVector3 operator +(const ElemToolsVector3& v1, const ElemToolsVector3& v2);
ElemToolsVector3 operator -(const ElemToolsVector3& v1, const ElemToolsVector3& v2);
ElemToolsVector3 operator *(const ElemToolsVector3& v1, const ElemToolsVector3& v2);
ElemToolsVector3 operator *(const ElemToolsVector3& v1, float scalar);

ElemToolsVector4 ElemToolsCreateQuaternion(ElemToolsVector3 v, float w);
ElemToolsVector4 ElemToolsMulQuat(ElemToolsVector4 q1, ElemToolsVector4 q2);
ElemToolsMatrix4x4 ElemToolsCreateRotationMatrix(ElemToolsVector4 quaternion);

ElemToolsMatrix4x4 ElemToolsCreateIdentityMatrix();
ElemToolsMatrix4x4 ElemToolsCreateScaleMatrix(float scale);
ElemToolsMatrix4x4 ElemToolsCreateTranslationMatrix(ElemToolsVector3 translation);
ElemToolsMatrix4x4 ElemToolsMulMatrix4x4(ElemToolsMatrix4x4 a, ElemToolsMatrix4x4 b);

void DecomposeTransform(ElemToolsMatrix4x4 transform, float* scale, ElemToolsVector4* rotationQuaternion, ElemToolsVector3* translation);
