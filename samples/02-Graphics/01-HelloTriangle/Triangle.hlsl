#define RootSignatureDefinitionWithSampler(numParameters, samplerDefinition) \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants="#numParameters", b0)," \
    "DescriptorTable(SRV(t0, space = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(SRV(t0, space = 1, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(UAV(u0, space = 2, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(UAV(u0, space = 3, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE))," \
    samplerDefinition

#define RootSignatureDefinition(numParameters) RootSignatureDefinitionWithSampler(numParameters, "")

#define RootSignatureDef RootSignatureDefinition(2)

struct ShaderParameters
{
    float RotationX;
    float RotationY;
};

float3 rotate(float3 position, float pitch, float roll, float yaw) 
{
    float cosa = cos(yaw);
    float sina = sin(yaw);

    float cosb = cos(pitch);
    float sinb = sin(pitch);

    float cosc = cos(roll);
    float sinc = sin(roll);

    float Axx = cosa*cosb;
    float Axy = cosa*sinb*sinc - sina*cosc;
    float Axz = cosa*sinb*cosc + sina*sinc;

    float Ayx = sina*cosb;
    float Ayy = sina*sinb*sinc + cosa*cosc;
    float Ayz = sina*sinb*cosc - cosa*sinc;

    float Azx = -sinb;
    float Azy = cosb*sinc;
    float Azz = cosb*cosc;

    float px = position.x;
    float py = position.y;
    float pz = position.z;

    float3 result;

    result.x = Axx*px + Axy*py + Axz*pz;
    result.y = Ayx*px + Ayy*py + Ayz*pz;
    result.z = Azx*px + Azy*py + Azz*pz;

    return result;
}

[[vk::push_constant]]
ConstantBuffer<ShaderParameters> parameters : register(b0);

struct VertexOutput
{
    float4 Position: SV_Position;
    float4 Color: TEXCOORD0;
};

static float3 triangleVertices[] =
{
    float3(-0.5, 0.5, 0),
    float3(0.5, 0.5, 0),
    float3(-0.5, -0.5, 0)
};

static float4 triangleColors[] =
{
    float4(1.0, 0.0, 0, 1),
    float4(0.0, 1.0, 0, 1),
    float4(0.0, 0.0, 1, 1)
};

static uint3 rectangleIndices[] =
{
    uint3(0, 1, 2)
};

[OutputTopology("triangle")]
[NumThreads(32, 1, 1)]
void MeshMain(in uint groupId : SV_GroupID, in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[128], out indices uint3 indices[128])
{
    const uint meshVertexCount = 3;
    const uint meshPrimitiveCount = 1;

    SetMeshOutputCounts(meshVertexCount, meshPrimitiveCount);

    if (groupThreadId < meshVertexCount)
    {
        float3 position = triangleVertices[groupThreadId];
        
        position = rotate(position, parameters.RotationY, parameters.RotationX, 0);
        position.z = 0.5;

        vertices[groupThreadId].Position = float4(position, 1);
        vertices[groupThreadId].Color = triangleColors[groupThreadId];
    }

    if (groupThreadId < meshPrimitiveCount)
    {
        indices[groupThreadId] = rectangleIndices[groupThreadId];
    }
}

struct PixelOutput
{
    float4 Color: SV_TARGET0;
};

PixelOutput PixelMain(const VertexOutput input)
{
    PixelOutput output = (PixelOutput)0;

    output.Color = input.Color;
    
    return output; 
}