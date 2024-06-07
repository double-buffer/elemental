#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

/**
 * WARNING: This math code is just for demonstration purpose only. All the math functions here focus
 * on readability and not performance. Don't use it in your production code!
 */

float pow2f(float value)
{
    return value * value;
}

float NormalizeAngle(float angle) 
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
} Vector2;

typedef struct
{
    float X, Y, Z;
} Vector3;

typedef union
{
    struct
    {
        float X, Y, Z, W;
    };

    struct
    {
        Vector3 XYZ;
    }; 

    struct
    {
        Vector2 XY;
    }; 
} Vector4;

#define V2Zero (Vector2) { .X = 0.0f, .Y = 0.0f }
#define V3Zero (Vector3) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f }

Vector2 InverseV2(Vector2 v)
{
	Vector2 result;

	result.X = -v.X;
	result.Y = -v.Y;

	return result;
}

Vector2 AddV2(Vector2 v1, Vector2 v2)
{
	Vector2 result;

	result.X = v1.X + v2.X;
	result.Y = v1.Y + v2.Y;

	return result;
}

Vector2 SubstractV2(Vector2 v1, Vector2 v2)
{
    Vector2 result;

    result.X = v1.X - v2.X;
    result.Y = v1.Y - v2.Y;
    
    return result;
}

Vector2 MulScalarV2(Vector2 v, float scalar)
{
	Vector2 result;

	result.X = v.X * scalar;
	result.Y = v.Y * scalar;

	return result;
}

Vector2 DivideScalarV2(Vector2 v, float scalar)
{
	Vector2 result;

	result.X = v.X / scalar;
	result.Y = v.Y / scalar;

	return result;
}

float DotProductV2(Vector2 v1, Vector2 v2)
{
	return v1.X * v2.X + v1.Y * v2.Y;
}

float MagnitudeSquaredV2(Vector2 v)
{
	return DotProductV2(v, v);
}

float MagnitudeV2(Vector2 v)
{
	return sqrtf(MagnitudeSquaredV2(v));
}

Vector2 NormalizeV2(Vector2 v)
{
	float magnitude = MagnitudeV2(v);

	if (magnitude > 0.0f)
	{
		Vector2 result = MulScalarV2(v, (1.0f / MagnitudeV2(v)));
		return result;
	}

	return v;
}

Vector3 InverseV3(Vector3 v)
{
	Vector3 result;

	result.X = -v.X;
	result.Y = -v.Y;
	result.Z = -v.Z;

	return result;
}

Vector3 AddV3(Vector3 v1, Vector3 v2)
{
	Vector3 result;

	result.X = v1.X + v2.X;
	result.Y = v1.Y + v2.Y;
	result.Z = v1.Z + v2.Z;

	return result;
}

Vector3 MulScalarV3(Vector3 v, float scalar)
{
	Vector3 result;

	result.X = v.X * scalar;
	result.Y = v.Y * scalar;
	result.Z = v.Z * scalar;

	return result;
}

Vector3 DivideScalarV3(Vector3 v, float scalar)
{
	Vector3 result;

	result.X = v.X / scalar;
	result.Y = v.Y / scalar;
	result.Z = v.Z / scalar;

	return result;
}

Vector3 MulV3(Vector3 v1, Vector3 v2)
{
	Vector3 result;

	result.X = v1.X * v2.X;
	result.Y = v1.Y * v2.Y;
	result.Z = v1.Z * v2.Z;

	return result;
}

float DotProductV3(Vector3 v1, Vector3 v2)
{
	return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

float MagnitudeSquaredV3(Vector3 v)
{
	return DotProductV3(v, v);
}

float MagnitudeV3(Vector3 v)
{
	return sqrtf(MagnitudeSquaredV3(v));
}

Vector3 NormalizeV3(Vector3 v)
{
	float magnitude = MagnitudeV3(v);

	if (magnitude > 0.0f)
	{
		Vector3 result = MulScalarV3(v, (1.0f / MagnitudeV3(v)));
		return result;
	}

	return v;
}

Vector3 CrossProductV3(Vector3 v1, Vector3 v2) 
{
    Vector3 result;
    result.X = v1.Y * v2.Z - v1.Z * v2.Y;
    result.Y = v1.Z * v2.X - v1.X * v2.Z;
    result.Z = v1.X * v2.Y - v1.Y * v2.X;
    return result;
}

Vector4 CreateQuaternion(Vector3 v, float w)
{
	Vector4 result;
    
	result.X = v.X * sinf(w * 0.5f);
	result.Y = v.Y * sinf(w * 0.5f);
	result.Z = v.Z * sinf(w * 0.5f);
	result.W = cosf(w * 0.5f);

	return result;
}

float DotProductQuat(Vector4 v1, Vector4 v2) 
{
    return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

Vector4 MulQuat(Vector4 q1, Vector4 q2)
{
    float x = q2.X * q1.W + q1.X * q2.W + CrossProductV3(q1.XYZ, q2.XYZ).X;
    float y = q2.Y * q1.W + q1.Y * q2.W + CrossProductV3(q1.XYZ, q2.XYZ).Y;
    float z = q2.Z * q1.W + q1.Z * q2.W + CrossProductV3(q1.XYZ, q2.XYZ).Z;
    
    float w = q1.W * q2.W - DotProductQuat(q1, q2);

    return (Vector4) { .X = x, .Y = y, .Z = z, .W = w };
}

// TODO: Get rid of the 4x4 here and write a function that convert it to constant buffer format
typedef struct
{
    float m[4][4];
} Matrix3x3;

Matrix3x3 CreateIdentityMatrix()
{
    Matrix3x3 result;

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

Matrix3x3 CreateRotationMatrix(float angle)
{
    Matrix3x3 result;
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

Matrix3x3 CreateScaleMatrix(float scale)
{
    Matrix3x3 result;

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

Matrix3x3 CreateTranslationMatrix(float tx, float ty)
{
    Matrix3x3 result;

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

Matrix3x3 MulMatrix3x3(Matrix3x3 a, Matrix3x3 b)
{
    Matrix3x3 result;

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            result.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
        }
    }

    return result;
}

Vector2 TransformPoint(Vector2 point, Matrix3x3 m)
{
    Vector2 result;

    result.X = m.m[0][0] * point.X + m.m[0][1] * point.Y + m.m[0][2];
    result.Y = m.m[1][0] * point.X + m.m[1][1] * point.Y + m.m[1][2];

    return result;
}
