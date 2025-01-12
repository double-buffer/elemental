#include "ToolsUtils.h"
#include "ElementalTools.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

// TODO: Do one for each threads
static MemoryArena FileIOMemoryArena;

ElemToolsDataSpan DefaultFileHandler(const char* path)
{
    if (SystemFileExists(path))
    {
        auto data = SystemFileReadBytes(FileIOMemoryArena, path);
        return { .Items = data.Pointer, .Length = (uint32_t)data.Length };
    }

    return {};
}

static ElemToolsLoadFileHandlerPtr loadFileHandlerPtr = DefaultFileHandler;

ElemToolsAPI void ElemToolsConfigureFileIO(ElemToolsLoadFileHandlerPtr loadFileHandler)
{
    SystemAssert(loadFileHandler);
    loadFileHandlerPtr = loadFileHandler;
}

void InitStorageMemoryArena()
{
    if (FileIOMemoryArena.Storage == nullptr)
    {
        FileIOMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }
}

ReadOnlySpan<uint8_t> LoadFileData(const char* path)
{
    InitStorageMemoryArena();

    auto data = loadFileHandlerPtr(path);
    return ReadOnlySpan<uint8_t>(data.Items, data.Length);
}

void ResetLoadFileDataMemory()
{
    SystemClearMemoryArena(FileIOMemoryArena);
}

ElemToolsMessageSpan ConstructErrorMessageSpan(MemoryArena memoryArena, const char* errorMessage)
{
    auto messageItem = SystemPushStruct<ElemToolsMessage>(memoryArena);
    messageItem->Type = ElemToolsMessageType_Error;
    messageItem->Message = errorMessage;

    return
    {
        .Items = messageItem,
        .Length = 1
    };
}

void AddPointToBoundingBox(ElemToolsVector3 point, ElemToolsBoundingBox* boundingBox)
{
    float minX = (point.X < boundingBox->MinPoint.X) ? point.X : boundingBox->MinPoint.X;
    float minY = (point.Y < boundingBox->MinPoint.Y) ? point.Y : boundingBox->MinPoint.Y;
    float minZ = (point.Z < boundingBox->MinPoint.Z) ? point.Z : boundingBox->MinPoint.Z;

    float maxX = (point.X > boundingBox->MaxPoint.X) ? point.X : boundingBox->MaxPoint.X;
    float maxY = (point.Y > boundingBox->MaxPoint.Y) ? point.Y : boundingBox->MaxPoint.Y;
    float maxZ = (point.Z > boundingBox->MaxPoint.Z) ? point.Z : boundingBox->MaxPoint.Z;

    boundingBox->MinPoint = { minX, minY, minZ };
    boundingBox->MaxPoint = { maxX, maxY, maxZ };
}

void AddBoundingBoxToBoundingBox(const ElemToolsBoundingBox* additional, ElemToolsBoundingBox* boundingBox)
{
    float minX = SystemMin(boundingBox->MinPoint.X, additional->MinPoint.X);
    float minY = SystemMin(boundingBox->MinPoint.Y, additional->MinPoint.Y);
    float minZ = SystemMin(boundingBox->MinPoint.Z, additional->MinPoint.Z);

    float maxX = SystemMax(boundingBox->MaxPoint.X, additional->MaxPoint.X);
    float maxY = SystemMax(boundingBox->MaxPoint.Y, additional->MaxPoint.Y);
    float maxZ = SystemMax(boundingBox->MaxPoint.Z, additional->MaxPoint.Z);

    boundingBox->MinPoint = { minX, minY, minZ };
    boundingBox->MaxPoint = { maxX, maxY, maxZ };
}

ElemToolsVector3 GetBoundingBoxCenter(const ElemToolsBoundingBox* boundingBox)
{
    return
    {
        .X = (boundingBox->MinPoint.X + boundingBox->MaxPoint.X) * 0.5f,
        .Y = (boundingBox->MinPoint.Y + boundingBox->MaxPoint.Y) * 0.5f,
        .Z = (boundingBox->MinPoint.Z + boundingBox->MaxPoint.Z) * 0.5f
    };
}

ElemToolsVector2 operator +(const ElemToolsVector2& v1, const ElemToolsVector2& v2)
{
	ElemToolsVector2 result;

	result.X = v1.X + v2.X;
	result.Y = v1.Y + v2.Y;

	return result;
}

ElemToolsVector2 operator -(const ElemToolsVector2& v1, const ElemToolsVector2& v2)
{
	ElemToolsVector2 result;

	result.X = v1.X - v2.X;
	result.Y = v1.Y - v2.Y;

	return result;
}

ElemToolsVector3 operator +(const ElemToolsVector3& v1, const ElemToolsVector3& v2)
{
	ElemToolsVector3 result;

	result.X = v1.X + v2.X;
	result.Y = v1.Y + v2.Y;
	result.Z = v1.Z + v2.Z;

	return result;
}

ElemToolsVector3 operator -(const ElemToolsVector3& v1, const ElemToolsVector3& v2)
{
	ElemToolsVector3 result;

	result.X = v1.X - v2.X;
	result.Y = v1.Y - v2.Y;
	result.Z = v1.Z - v2.Z;

	return result;
}

ElemToolsVector3 operator *(const ElemToolsVector3& v1, const ElemToolsVector3& v2)
{
    ElemToolsVector3 result;

	result.X = v1.X * v2.X;
	result.Y = v1.Y * v2.Y;
	result.Z = v1.Z * v2.Z;

	return result;
}

ElemToolsVector3 operator *(const ElemToolsVector3& v, float scalar)
{
	ElemToolsVector3 result;

	result.X = v.X * scalar;
	result.Y = v.Y * scalar;
	result.Z = v.Z * scalar;

	return result;
}

ElemToolsVector3 ElemToolsMulScalarV3(ElemToolsVector3 v, float scalar)
{
	ElemToolsVector3 result;

	result.X = v.X * scalar;
	result.Y = v.Y * scalar;
	result.Z = v.Z * scalar;

	return result;
}

ElemToolsVector3 ElemToolsMulV3(ElemToolsVector3 v1, ElemToolsVector3 v2)
{
	ElemToolsVector3 result;

	result.X = v1.X * v2.X;
	result.Y = v1.Y * v2.Y;
	result.Z = v1.Z * v2.Z;

	return result;
}

float ElemToolsDotProductV3(ElemToolsVector3 v1, ElemToolsVector3 v2)
{
	return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

float ElemToolsMagnitudeSquaredV3(ElemToolsVector3 v)
{
	return ElemToolsDotProductV3(v, v);
}

float ElemToolsMagnitudeV3(ElemToolsVector3 v)
{
	return sqrtf(ElemToolsMagnitudeSquaredV3(v));
}

ElemToolsVector3 ElemToolsNormalizeV3(ElemToolsVector3 v)
{
	float magnitude = ElemToolsMagnitudeV3(v);

	if (magnitude > 0.0f)
	{
		ElemToolsVector3 result = ElemToolsMulScalarV3(v, (1.0f / ElemToolsMagnitudeV3(v)));
		return result;
	}

	return v;
}
// TODO: CLEANUP
ElemToolsVector3 ElemToolsCrossProductV3(ElemToolsVector3 v1, ElemToolsVector3 v2) 
{
    ElemToolsVector3 result;
    result.X = v1.Y * v2.Z - v1.Z * v2.Y;
    result.Y = v1.Z * v2.X - v1.X * v2.Z;
    result.Z = v1.X * v2.Y - v1.Y * v2.X;
    return result;
}

ElemToolsVector4 ElemToolsCreateQuaternion(ElemToolsVector3 v, float w)
{
	ElemToolsVector4 result;
    
	result.X = v.X * sinf(w * 0.5f);
	result.Y = v.Y * sinf(w * 0.5f);
	result.Z = v.Z * sinf(w * 0.5f);
	result.W = cosf(w * 0.5f);

	return result;
}

float ElemToolsDotProductQuat(ElemToolsVector4 v1, ElemToolsVector4 v2) 
{
    return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

ElemToolsVector4 ElemToolsMulQuat(ElemToolsVector4 q1, ElemToolsVector4 q2)
{
    float x = q2.X * q1.W + q1.X * q2.W + ElemToolsCrossProductV3(q1.XYZ, q2.XYZ).X;
    float y = q2.Y * q1.W + q1.Y * q2.W + ElemToolsCrossProductV3(q1.XYZ, q2.XYZ).Y;
    float z = q2.Z * q1.W + q1.Z * q2.W + ElemToolsCrossProductV3(q1.XYZ, q2.XYZ).Z;
    
    float w = q1.W * q2.W - ElemToolsDotProductQuat(q1, q2);

    return (ElemToolsVector4) { .X = x, .Y = y, .Z = z, .W = w };
}

ElemToolsMatrix4x4 ElemToolsCreateIdentityMatrix()
{
    ElemToolsMatrix4x4 result;

    result.m[0][0] = 1.0f;
    result.m[0][1] = 0.0f;
    result.m[0][2] = 0.0f;
    result.m[0][3] = 0.0f;

    result.m[1][0] = 0.0f;
    result.m[1][1] = 1.0f;
    result.m[1][2] = 0.0f;
    result.m[1][3] = 0.0f;

    result.m[2][0] = 0.0f;
    result.m[2][1] = 0.0f;
    result.m[2][2] = 1.0f;
    result.m[2][3] = 0.0f;

    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}

ElemToolsMatrix4x4 ElemToolsCreateScaleMatrix(float scale)
{
    ElemToolsMatrix4x4 result;

    result.Rows[0] = {{ scale, 0.0f, 0.0f, 0.0f }};
    result.Rows[1] = {{ 0.0f, scale, 0.0f, 0.0f }};
    result.Rows[2] = {{ 0.0f, 0.0f, scale, 0.0f }};
    result.Rows[3] = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

    return result;
}

ElemToolsMatrix4x4 ElemToolsCreateTranslationMatrix(ElemToolsVector3 translation)
{
    ElemToolsMatrix4x4 result;

    result.Rows[0] = {{ 1.0f, 0.0f, 0.0f, 0.0f }};
    result.Rows[1] = {{ 0.0f, 1.0f, 0.0f, 0.0f }};
    result.Rows[2] = {{ 0.0f, 0.0f, 1.0f, 0.0f }};
    result.Rows[3] = {{ translation.X, translation.Y, translation.Z, 1.0f }};

    return result;
}

ElemToolsMatrix4x4 ElemToolsMulMatrix4x4(ElemToolsMatrix4x4 a, ElemToolsMatrix4x4 b)
{
    ElemToolsMatrix4x4 result;

    for (uint32_t i = 0; i < 4; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            result.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
        }
    }

    return result;
}

ElemToolsMatrix4x4 ElemToolsCreateRotationMatrix(ElemToolsVector4 quaternion)
{
	float xw = quaternion.X *quaternion.W , xx = quaternion.X *quaternion.X , yy = quaternion.Y *quaternion.Y ,
      	yw = quaternion.Y *quaternion.W , xy = quaternion.X *quaternion.Y , yz = quaternion.Y *quaternion.Z ,
      	zw = quaternion.Z *quaternion.W , xz = quaternion.X *quaternion.Z , zz = quaternion.Z *quaternion.Z ;

    ElemToolsMatrix4x4 result;

	result.Rows[0] = (ElemToolsVector4) { .X = 1-2*(yy+zz), .Y = 2*(xy+zw), .Z = 2*(xz-yw), .W = 0.0f };
	result.Rows[1] = (ElemToolsVector4) { .X = 2*(xy-zw), .Y = 1-2*(xx+zz), .Z = 2*(yz+xw), .W = 0.0f };
	result.Rows[2] = (ElemToolsVector4) { .X = 2*(xz+yw), .Y = 2*(yz-xw), .Z = 1-2*(xx+yy), .W = 0.0f };
	result.Rows[3] = (ElemToolsVector4) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f, .W = 1.0f };

    return result;
}

ElemToolsVector3 ElemToolsTransformPointV3(ElemToolsVector3 point, ElemToolsMatrix4x4 m)
{
    ElemToolsVector3 result;

    result.X = m.m[0][0] * point.X + m.m[0][1] * point.Y + m.m[0][2] * point.Z + m.m[0][3];
    result.Y = m.m[1][0] * point.X + m.m[1][1] * point.Y + m.m[1][2] * point.Z + m.m[1][3];
    result.Z = m.m[2][0] * point.X + m.m[2][1] * point.Y + m.m[2][2] * point.Z + m.m[2][3];

    return result;
}

void DecomposeTransform(ElemToolsMatrix4x4 transform, float* scale, ElemToolsVector4* rotationQuaternion, ElemToolsVector3* translation)
{
    translation->X = transform.Rows[3].X;
    translation->Y = transform.Rows[3].Y;
    translation->Z = transform.Rows[3].Z;

	// compute determinant to determine handedness
	float det =
	    transform.m[0][0] * (transform.m[1][1] * transform.m[2][2] - transform.m[2][1] * transform.m[1][2]) -
	    transform.m[0][1] * (transform.m[1][0] * transform.m[2][2] - transform.m[1][2] * transform.m[2][0]) +
	    transform.m[0][2] * (transform.m[1][0] * transform.m[2][1] - transform.m[1][1] * transform.m[2][0]);

	float sign = (det < 0.f) ? -1.f : 1.f;

	// recover scale from axis lengths
    float scales[3];

	scales[0] = sqrtf(transform.m[0][0] * transform.m[0][0] + transform.m[0][1] * transform.m[0][1] + transform.m[0][2] * transform.m[0][2]) * sign;
    scales[1] = sqrtf(transform.m[1][0] * transform.m[1][0] + transform.m[1][1] * transform.m[1][1] + transform.m[1][2] * transform.m[1][2]) * sign;
	scales[2] = sqrtf(transform.m[2][0] * transform.m[2][0] + transform.m[2][1] * transform.m[2][1] + transform.m[2][2] * transform.m[2][2]) * sign;

    *scale = scales[0];

	// normalize axes to get a pure rotation matrix
	float rsx = (scales[0] == 0.f) ? 0.f : 1.f / scales[0];
	float rsy = (scales[1] == 0.f) ? 0.f : 1.f / scales[1];
	float rsz = (scales[2] == 0.f) ? 0.f : 1.f / scales[2];

	float r00 = transform.m[0][0] * rsx, r10 = transform.m[1][0] * rsy, r20 = transform.m[2][0] * rsz;
	float r01 = transform.m[0][1] * rsx, r11 = transform.m[1][1] * rsy, r21 = transform.m[2][1] * rsz;
	float r02 = transform.m[0][2] * rsx, r12 = transform.m[1][2] * rsy, r22 = transform.m[2][2] * rsz;

	// "branchless" version of Mike Day's matrix to quaternion conversion
	int qc = r22 < 0 ? (r00 > r11 ? 0 : 1) : (r00 < -r11 ? 2 : 3);
	float qs1 = qc & 2 ? -1.f : 1.f;
	float qs2 = qc & 1 ? -1.f : 1.f;
	float qs3 = (qc - 1) & 2 ? -1.f : 1.f;

	float qt = 1.f - qs3 * r00 - qs2 * r11 - qs1 * r22;
	float qs = 0.5f / sqrtf(qt);

    float* rotation = (float*)rotationQuaternion;

	rotation[qc ^ 0] = qs * qt;
	rotation[qc ^ 1] = qs * (r01 + qs1 * r10);
	rotation[qc ^ 2] = qs * (r20 + qs2 * r02);
	rotation[qc ^ 3] = qs * (r12 + qs3 * r21);

    rotationQuaternion->X = rotation[0];
    rotationQuaternion->Y = rotation[1];
    rotationQuaternion->Z = rotation[2];
    rotationQuaternion->W = rotation[3];
}
