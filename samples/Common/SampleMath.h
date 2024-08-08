#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

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

typedef struct
{
    float X, Y, Z;
} SampleVector3;

typedef union
{
    struct
    {
        float X, Y, Z, W;
    };

    struct
    {
        SampleVector3 XYZ;
    }; 

    struct
    {
        SampleVector2 XY;
    }; 
} SampleVector4;

#define V2Zero (SampleVector2) { .X = 0.0f, .Y = 0.0f }
#define V3Zero (SampleVector3) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f }

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

SampleVector3 SampleInverseV3(SampleVector3 v)
{
	SampleVector3 result;

	result.X = -v.X;
	result.Y = -v.Y;
	result.Z = -v.Z;

	return result;
}

SampleVector3 SampleAddV3(SampleVector3 v1, SampleVector3 v2)
{
	SampleVector3 result;

	result.X = v1.X + v2.X;
	result.Y = v1.Y + v2.Y;
	result.Z = v1.Z + v2.Z;

	return result;
}

SampleVector3 SampleMulScalarV3(SampleVector3 v, float scalar)
{
	SampleVector3 result;

	result.X = v.X * scalar;
	result.Y = v.Y * scalar;
	result.Z = v.Z * scalar;

	return result;
}

SampleVector3 SampleDivideScalarV3(SampleVector3 v, float scalar)
{
	SampleVector3 result;

	result.X = v.X / scalar;
	result.Y = v.Y / scalar;
	result.Z = v.Z / scalar;

	return result;
}

SampleVector3 SampleMulV3(SampleVector3 v1, SampleVector3 v2)
{
	SampleVector3 result;

	result.X = v1.X * v2.X;
	result.Y = v1.Y * v2.Y;
	result.Z = v1.Z * v2.Z;

	return result;
}

float SampleDotProductV3(SampleVector3 v1, SampleVector3 v2)
{
	return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

float SampleMagnitudeSquaredV3(SampleVector3 v)
{
	return SampleDotProductV3(v, v);
}

float SampleMagnitudeV3(SampleVector3 v)
{
	return sqrtf(SampleMagnitudeSquaredV3(v));
}

SampleVector3 SampleNormalizeV3(SampleVector3 v)
{
	float magnitude = SampleMagnitudeV3(v);

	if (magnitude > 0.0f)
	{
		SampleVector3 result = SampleMulScalarV3(v, (1.0f / SampleMagnitudeV3(v)));
		return result;
	}

	return v;
}

SampleVector3 SampleCrossProductV3(SampleVector3 v1, SampleVector3 v2) 
{
    SampleVector3 result;
    result.X = v1.Y * v2.Z - v1.Z * v2.Y;
    result.Y = v1.Z * v2.X - v1.X * v2.Z;
    result.Z = v1.X * v2.Y - v1.Y * v2.X;
    return result;
}

SampleVector4 SampleCreateQuaternion(SampleVector3 v, float w)
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

// TODO: Get rid of the 4x4 here and write a function that convert it to constant buffer format
typedef struct
{
    float m[4][4];
} SampleMatrix3x3;

SampleMatrix3x3 SampleCreateIdentityMatrix()
{
    SampleMatrix3x3 result;

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

    return result;
}

SampleMatrix3x3 SampleCreateRotationMatrix(float angle)
{
    SampleMatrix3x3 result;
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

    return result;
}

SampleMatrix3x3 SampleCreateScaleMatrix(float scale)
{
    SampleMatrix3x3 result;

    result.m[0][0] = scale;
    result.m[0][1] = 0.0f;
    result.m[0][2] = 0.0f;

    result.m[1][0] = 0.0f;
    result.m[1][1] = scale;
    result.m[1][2] = 0.0f;

    result.m[2][0] = 0.0f;
    result.m[2][1] = 0.0f;
    result.m[2][2] = 1.0f;

    return result;
}

SampleMatrix3x3 SampleCreateTranslationMatrix(float tx, float ty)
{
    SampleMatrix3x3 result;

    result.m[0][0] = 1.0f;
    result.m[0][1] = 0.0f;
    result.m[0][2] = tx;

    result.m[1][0] = 0.0f;
    result.m[1][1] = 1.0f;
    result.m[1][2] = ty;

    result.m[2][0] = 0.0f;
    result.m[2][1] = 0.0f;
    result.m[2][2] = 1.0f;

    return result;
}

SampleMatrix3x3 SampleMulMatrix3x3(SampleMatrix3x3 a, SampleMatrix3x3 b)
{
    SampleMatrix3x3 result;

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            result.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
        }
    }

    return result;
}

SampleVector2 SampleTransformPoint(SampleVector2 point, SampleMatrix3x3 m)
{
    SampleVector2 result;

    result.X = m.m[0][0] * point.X + m.m[0][1] * point.Y + m.m[0][2];
    result.Y = m.m[1][0] * point.X + m.m[1][1] * point.Y + m.m[1][2];

    return result;
}
