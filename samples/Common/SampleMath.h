#pragma once

#include <stdint.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef ElemToolsAPI
typedef ElemToolsVector3 ElemVector3;
typedef ElemToolsVector4 ElemVector4;
#else
#include "../Elemental/Elemental.h"
#endif 

/**
 * WARNING: This math code is just for demonstration purpose only. All the math functions here focus
 * on readability and not performance. Don't use it in your production code!
 */

// TODO: Review the angle functions, we are using left handed coordinate system. So positive rotation should
// be in clockwise order

float SamplePow2f(float value)
{
    return value * value;
}

float SampleNormalizeAngle(float angle) 
{
    angle = fmod(angle + M_PI, 2 * M_PI);

    if (angle < 0) 
    {
        angle += 2 * M_PI;
    }

    return angle - M_PI;
}

typedef struct
{
    float X, Y;
} SampleVector2;

typedef union
{
    struct
    {
        float X, Y, Z, W;
    };

    struct
    {
        ElemVector3 XYZ;
    }; 

    struct
    {
        SampleVector2 XY;
    }; 
} SampleVector4;

#define V2Zero (SampleVector2) { .X = 0.0f, .Y = 0.0f }
#define V3Zero (ElemVector3) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f }

SampleVector2 SampleInverseV2(SampleVector2 v)
{
	SampleVector2 result;

	result.X = -v.X;
	result.Y = -v.Y;

	return result;
}

SampleVector2 SampleAddV2(SampleVector2 v1, SampleVector2 v2)
{
	SampleVector2 result;

	result.X = v1.X + v2.X;
	result.Y = v1.Y + v2.Y;

	return result;
}

SampleVector2 SampleSubstractV2(SampleVector2 v1, SampleVector2 v2)
{
    SampleVector2 result;

    result.X = v1.X - v2.X;
    result.Y = v1.Y - v2.Y;
    
    return result;
}

SampleVector2 SampleMulScalarV2(SampleVector2 v, float scalar)
{
	SampleVector2 result;

	result.X = v.X * scalar;
	result.Y = v.Y * scalar;

	return result;
}

SampleVector2 SampleDivideScalarV2(SampleVector2 v, float scalar)
{
	SampleVector2 result;

	result.X = v.X / scalar;
	result.Y = v.Y / scalar;

	return result;
}

float SampleDotProductV2(SampleVector2 v1, SampleVector2 v2)
{
	return v1.X * v2.X + v1.Y * v2.Y;
}

float SampleMagnitudeSquaredV2(SampleVector2 v)
{
	return SampleDotProductV2(v, v);
}

float SampleMagnitudeV2(SampleVector2 v)
{
	return sqrtf(SampleMagnitudeSquaredV2(v));
}

SampleVector2 SampleNormalizeV2(SampleVector2 v)
{
	float magnitude = SampleMagnitudeV2(v);

	if (magnitude > 0.0f)
	{
		SampleVector2 result = SampleMulScalarV2(v, (1.0f / SampleMagnitudeV2(v)));
		return result;
	}

	return v;
}

ElemVector3 SampleInverseV3(ElemVector3 v)
{
	ElemVector3 result;

	result.X = -v.X;
	result.Y = -v.Y;
	result.Z = -v.Z;

	return result;
}

ElemVector3 SampleAddV3(ElemVector3 v1, ElemVector3 v2)
{
	ElemVector3 result;

	result.X = v1.X + v2.X;
	result.Y = v1.Y + v2.Y;
	result.Z = v1.Z + v2.Z;

	return result;
}

ElemVector3 SampleMinV3(ElemVector3 v1, ElemVector3 v2)
{
	ElemVector3 result;

	result.X = v1.X - v2.X;
	result.Y = v1.Y - v2.Y;
	result.Z = v1.Z - v2.Z;

	return result;
}

ElemVector3 SampleMulScalarV3(ElemVector3 v, float scalar)
{
	ElemVector3 result;

	result.X = v.X * scalar;
	result.Y = v.Y * scalar;
	result.Z = v.Z * scalar;

	return result;
}

ElemVector3 SampleDivideScalarV3(ElemVector3 v, float scalar)
{
	ElemVector3 result;

	result.X = v.X / scalar;
	result.Y = v.Y / scalar;
	result.Z = v.Z / scalar;

	return result;
}

ElemVector3 SampleMulV3(ElemVector3 v1, ElemVector3 v2)
{
	ElemVector3 result;

	result.X = v1.X * v2.X;
	result.Y = v1.Y * v2.Y;
	result.Z = v1.Z * v2.Z;

	return result;
}

float SampleDotProductV3(ElemVector3 v1, ElemVector3 v2)
{
	return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

float SampleMagnitudeSquaredV3(ElemVector3 v)
{
	return SampleDotProductV3(v, v);
}

float SampleMagnitudeV3(ElemVector3 v)
{
	return sqrtf(SampleMagnitudeSquaredV3(v));
}

ElemVector3 SampleNormalizeV3(ElemVector3 v)
{
	float magnitude = SampleMagnitudeV3(v);

	if (magnitude > 0.0f)
	{
		ElemVector3 result = SampleMulScalarV3(v, (1.0f / SampleMagnitudeV3(v)));
		return result;
	}

	return v;
}

ElemVector3 SampleCrossProductV3(ElemVector3 v1, ElemVector3 v2) 
{
    ElemVector3 result;
    result.X = v1.Y * v2.Z - v1.Z * v2.Y;
    result.Y = v1.Z * v2.X - v1.X * v2.Z;
    result.Z = v1.X * v2.Y - v1.Y * v2.X;
    return result;
}

SampleVector4 SampleCreateQuaternion(ElemVector3 v, float w)
{
	SampleVector4 result;
    
	result.X = v.X * sinf(w * 0.5f);
	result.Y = v.Y * sinf(w * 0.5f);
	result.Z = v.Z * sinf(w * 0.5f);
	result.W = cosf(w * 0.5f);

	return result;
}

float SampleDotProductQuat(SampleVector4 v1, SampleVector4 v2) 
{
    return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

SampleVector4 SampleMulQuat(SampleVector4 q1, SampleVector4 q2)
{
    float x = q2.X * q1.W + q1.X * q2.W + SampleCrossProductV3(q1.XYZ, q2.XYZ).X;
    float y = q2.Y * q1.W + q1.Y * q2.W + SampleCrossProductV3(q1.XYZ, q2.XYZ).Y;
    float z = q2.Z * q1.W + q1.Z * q2.W + SampleCrossProductV3(q1.XYZ, q2.XYZ).Z;
    
    float w = q1.W * q2.W - SampleDotProductQuat(q1, q2);

    return (SampleVector4) { .X = x, .Y = y, .Z = z, .W = w };
}

// TODO: Get rid of this
typedef union
{
    float m[4][4];
    SampleVector4 Rows[4];
} SampleMatrix4x4;

SampleMatrix4x4 SampleCreateIdentityMatrix()
{
    SampleMatrix4x4 result;

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

// TODO: To review
SampleMatrix4x4 SampleCreateTransformMatrix(SampleVector4 quaternion, ElemVector3 translation)
{
	float xw = quaternion.X *quaternion.W , xx = quaternion.X *quaternion.X , yy = quaternion.Y *quaternion.Y ,
      	yw = quaternion.Y *quaternion.W , xy = quaternion.X *quaternion.Y , yz = quaternion.Y *quaternion.Z ,
      	zw = quaternion.Z *quaternion.W , xz = quaternion.X *quaternion.Z , zz = quaternion.Z *quaternion.Z ;

    SampleMatrix4x4 result;

	result.Rows[0] = (SampleVector4) { .X = 1-2*(yy+zz), .Y = 2*(xy+zw), .Z = 2*(xz-yw), .W = 0.0f };
	result.Rows[1] = (SampleVector4) { .X = 2*(xy-zw), .Y = 1-2*(xx+zz), .Z = 2*(yz+xw), .W = 0.0f };
	result.Rows[2] = (SampleVector4) { .X = 2*(xz+yw), .Y = 2*(yz-xw), .Z = 1-2*(xx+yy), .W = 0.0f };
	result.Rows[3] = (SampleVector4) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f, .W = 1.0f };

    return result;
}

SampleMatrix4x4 SampleTransposeMatrix(SampleMatrix4x4 matrix)
{
    SampleMatrix4x4 result;

    result.m[0][0] = matrix.m[0][0];
    result.m[0][1] = matrix.m[1][0];
    result.m[0][2] = matrix.m[2][0];
    result.m[0][3] = matrix.m[3][0];

    result.m[1][0] = matrix.m[0][1];
    result.m[1][1] = matrix.m[1][1];
    result.m[1][2] = matrix.m[2][1];
    result.m[1][3] = matrix.m[3][1];

    result.m[2][0] = matrix.m[0][2];
    result.m[2][1] = matrix.m[1][2];
    result.m[2][2] = matrix.m[2][2];
    result.m[2][3] = matrix.m[3][2];

    result.m[3][0] = matrix.m[0][3];
    result.m[3][1] = matrix.m[1][3];
    result.m[3][2] = matrix.m[2][3];
    result.m[3][3] = matrix.m[3][3];

    return result;
}

SampleMatrix4x4 SampleCreateRotationMatrix(float angle)
{
    // BUG: Uninit values!
    SampleMatrix4x4 result = {};
    float c = cosf(angle);
    float s = sinf(angle);

    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[0][2] = 0.0f;

    result.m[1][0] = s;
    result.m[1][1] = c;
    result.m[1][2] = 0.0f;

    result.m[2][0] = 0.0f;
    result.m[2][1] = 0.0f;
    result.m[2][2] = 1.0f;

    result.m[3][3] = 1.0f;

    return result;
}

SampleMatrix4x4 SampleCreateScaleMatrix(float scale)
{
    // BUG: Uninit values!
    SampleMatrix4x4 result = {};

    result.m[0][0] = scale;
    result.m[0][1] = 0.0f;
    result.m[0][2] = 0.0f;

    result.m[1][0] = 0.0f;
    result.m[1][1] = scale;
    result.m[1][2] = 0.0f;

    result.m[2][0] = 0.0f;
    result.m[2][1] = 0.0f;
    result.m[2][2] = scale;
    
    result.m[3][3] = 1.0f;

    return result;
}

SampleMatrix4x4 SampleCreateTranslationMatrix(float tx, float ty)
{
    // BUG: Uninit values!
    SampleMatrix4x4 result = SampleCreateIdentityMatrix();
    result.Rows[3].XY = (SampleVector2) { tx, ty };

    return result;
}

SampleMatrix4x4 SampleCreateLookAtLHMatrix(ElemVector3 eyePosition, ElemVector3 targetPosition, ElemVector3 upDirection)
{
    ElemVector3 forwardDirection = SampleNormalizeV3(SampleMinV3(targetPosition, eyePosition));
    ElemVector3 rightDirection = SampleNormalizeV3(SampleCrossProductV3(upDirection, forwardDirection));
    ElemVector3 upDirectionNew = SampleCrossProductV3(forwardDirection, rightDirection);

    SampleMatrix4x4 result;

    result.Rows[0] = (SampleVector4) { .X = rightDirection.X, .Y = upDirectionNew.X, .Z = forwardDirection.X, .W = 0.0f };
    result.Rows[1] = (SampleVector4) { .X = rightDirection.Y, .Y = upDirectionNew.Y, .Z = forwardDirection.Y, .W = 0.0f };
    result.Rows[2] = (SampleVector4) { .X = rightDirection.Z, .Y = upDirectionNew.Z, .Z = forwardDirection.Z, .W = 0.0f };
    result.Rows[3] = (SampleVector4) { .X = -SampleDotProductV3(rightDirection, eyePosition), .Y = -SampleDotProductV3(upDirectionNew, eyePosition), .Z = -SampleDotProductV3(forwardDirection, eyePosition), .W = 1.0f };

    return result;
}

SampleMatrix4x4 SampleCreatePerspectiveProjectionMatrix(float fovY, float aspectRatio, float zNear)
{
    float height = 1.0f / tanf(fovY * 0.5f);

    SampleMatrix4x4 result;

    result.Rows[0] = (SampleVector4) { .X = height / aspectRatio, .Y = 0.0f, .Z = 0.0f, .W = 0.0f };
    result.Rows[1] = (SampleVector4) { .X = 0.0f, .Y = height, .Z = 0.0f, .W = 0.0f };
    result.Rows[2] = (SampleVector4) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f, .W = 1.0f };
    result.Rows[3] = (SampleVector4) { .X = 0.0f, .Y = 0.0f, .Z = zNear, .W = 0.0f };

    return result;
}

SampleMatrix4x4 SampleMulMatrix4x4(SampleMatrix4x4 a, SampleMatrix4x4 b)
{
    SampleMatrix4x4 result;

    for (uint32_t i = 0; i < 4; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            result.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
        }
    }

    return result;
}

SampleVector2 SampleTransformPointV2(SampleVector2 point, SampleMatrix4x4 m)
{
    SampleVector2 result;

    result.X = m.m[0][0] * point.X + m.m[0][1] * point.Y + m.m[0][2];
    result.Y = m.m[1][0] * point.X + m.m[1][1] * point.Y + m.m[1][2];

    return result;
}

ElemVector3 SampleTransformPointV3(ElemVector3 point, SampleMatrix4x4 m)
{
    ElemVector3 result;

    result.X = m.m[0][0] * point.X + m.m[0][1] * point.Y + m.m[0][2] * point.Z + m.m[0][3];
    result.Y = m.m[1][0] * point.X + m.m[1][1] * point.Y + m.m[1][2] * point.Z + m.m[1][3];
    result.Z = m.m[2][0] * point.X + m.m[2][1] * point.Y + m.m[2][2] * point.Z + m.m[2][3];

    return result;
}

