struct ShaderParameters
{
    float AspectRatio;
    float RotationX;
    float RotationY;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct Vertex
{
    float3 Position;
    float4 Color;
};

struct VertexOutput
{
    float4 Position: SV_Position;
    float4 Color: TEXCOORD0;
};

static Vertex triangleVertices[] =
{
    { float3(-0.5, 0.5, 0.0), float4(1.0, 0.0, 0.0, 1.0) },
    { float3(0.5, 0.5, 0.0), float4(0.0, 1.0, 0.0, 1.0) },
    { float3(-0.5, -0.5, 0.0), float4(0.0, 0.0, 1.0, 1.0) }
};

float4x4 LookAtLHMatrix(float3 eyePosition, float3 targetPosition, float3 upDirection)
{
    float3 forwardDirection = normalize(targetPosition - eyePosition);
    float3 rightDirection = normalize(cross(upDirection, forwardDirection));
    float3 upDirectionNew = cross(forwardDirection, rightDirection);

    float4 row1 = float4(rightDirection.x, upDirectionNew.x, forwardDirection.x, 0.0f);
    float4 row2 = float4(rightDirection.y, upDirectionNew.y, forwardDirection.y, 0.0f);
    float4 row3 = float4(rightDirection.z, upDirectionNew.z, forwardDirection.z, 0.0f);
    float4 row4 = float4(-dot(rightDirection, eyePosition), -dot(upDirectionNew, eyePosition), -dot(forwardDirection, eyePosition), 1.0f);

    return float4x4(row1, row2, row3, row4);
}

float4x4 PerspectiveProjectionMatrix(float fovY, float aspectRatio, float zNear)
{
    float height = 1.0 / tan(fovY * 0.5);

    float4 row1 = float4(height / aspectRatio, 0.0f, 0.0f, 0.0f);
    float4 row2 = float4(0.0f, height, 0.0f, 0.0f);
    float4 row3 = float4(0.0f, 0.0f, 0, 1.0f);
    float4 row4 = float4(0.0f, 0.0f, zNear, 0.0f);

    return float4x4(row1, row2, row3, row4);
}

float4x4 RotationMatrix(float rotationX, float rotationY, float rotationZ)
{
    float cosX = cos(rotationX);
    float sinX = sin(rotationX);
    float cosY = cos(rotationY);
    float sinY = sin(rotationY);
    float cosZ = cos(rotationZ);
    float sinZ = sin(rotationZ);

    float4 row1 = float4(cosY * cosZ, sinX * sinY * cosZ - cosX * sinZ, cosX * sinY * cosZ + sinX * sinZ, 0.0f);
    float4 row2 = float4(cosY * sinZ, sinX * sinY * sinZ + cosX * cosZ, cosX * sinY * sinZ - sinX * cosZ, 0.0f);
    float4 row3 = float4(-sinY, sinX * cosY, cosX * cosY, 0.0f);
    float4 row4 = float4(0.0f, 0.0f, 0.0f, 1.0f);

    return float4x4(row1, row2, row3, row4);
}

float4x4 TranslationMatrix(float translationX, float translationY, float translationZ)
{
    float4 row1 = float4(1.0f, 0.0f, 0.0f, 0.0f);
    float4 row2 = float4(0.0f, 1.0f, 0.0f, 0.0f);
    float4 row3 = float4(0.0f, 0.0f, 1.0f, 0.0f);
    float4 row4 = float4(translationX, translationY, translationZ, 1.0f);

    return float4x4(row1, row2, row3, row4);
}

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(32, 1, 1)]
void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[3], out indices uint3 indices[1])
{
    const uint meshVertexCount = 3;

    SetMeshOutputCounts(meshVertexCount, 1);

    if (groupThreadId < meshVertexCount)
    {
        float cameraZDistance = parameters.AspectRatio >= 0.75 ? -2.0 : -4.0;

        float4x4 worldMatrix = RotationMatrix(parameters.RotationX, parameters.RotationY, 0.0);
        float4x4 viewMatrix = LookAtLHMatrix(float3(0, 0, cameraZDistance), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 projectionMatrix = PerspectiveProjectionMatrix(0.78, parameters.AspectRatio, 0.001);

        float4x4 worldViewProjectionMatrix = mul(worldMatrix, mul(viewMatrix, projectionMatrix));

        vertices[groupThreadId].Position = mul(float4(triangleVertices[groupThreadId].Position, 1), worldViewProjectionMatrix);
        vertices[groupThreadId].Color = triangleVertices[groupThreadId].Color;
    }

    if (groupThreadId == 0)
    {
        indices[groupThreadId] = uint3(0, 1, 2);
    }
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    return input.Color; 
}
