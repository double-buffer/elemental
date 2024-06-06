struct ShaderParameters
{
    uint32_t RenderTextureIndex;
    uint32_t TriangleColor;
    float Zoom;
    float3x3 Transform;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

float2 ComplexSquareAdd(float2 current, float2 constant) 
{
    float zr = current.x * current.x - current.y * current.y;
    float zi = 2.0 * current.x * current.y;

    return float2(zr, zi) + constant;
}

float3x3 createTranslationMatrix(float2 translation)
{
    return float3x3(
        1, 0, 0,
        0, 1, 0,
        translation.x, translation.y, 1
    );
}

float3x3 createRotationMatrix(float rotation)
{
    float cosRot = cos(rotation);
    float sinRot = sin(rotation);
    return float3x3(
        cosRot, sinRot, 0,
        -sinRot, cosRot, 0,
        0, 0, 1
    );
}

float3x3 createScalingMatrix(float scale)
{
    return float3x3(
        scale, 0, 0,
        0, scale, 0,
        0, 0, 1
    );
}

#define MAX_ITERATIONS 500

[shader("compute")]
[numthreads(16, 16, 1)]
void Fractal(uint3 threadId: SV_DispatchThreadID)
{
    // TODO: We need to find a way to do that in vulkan
    // The solution for now will be to generate some compatibility code

    #ifndef __spirv__
    RWTexture2D<float4> renderTexture = ResourceDescriptorHeap[parameters.RenderTextureIndex];

    // TODO: This is slower, pass the width and height!
    uint width, height;
    renderTexture.GetDimensions(width, height);

    if (threadId.x < width && threadId.y < height)
    {
        float aspectRatio = (float)width / height;
        float2 uv = (threadId.xy / float2(width, height)) * 2.0 - 1.0;

        // TODO: Refactor with matrices
        uv.x *= aspectRatio;
        uv *= parameters.Zoom;

        uv = mul(float3(uv, 1.0), parameters.Transform);

        float2 Z = uv;
        float2 C = float2(-0.8, 0.153);

        uint iteration;

        for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) 
        {
            Z = ComplexSquareAdd(Z, C);

            if (dot(Z, Z) > 4.0) 
            {
                break;
            }
        }

        float mod = length(Z);
        //float smoothIteration = float(iteration) - log2(max(1.0, log2(mod)));
        float smoothIteration = float(iteration) - log2(log2(dot(Z, Z))) + 4.0; // TODO: 4 seems to be a constant to make more cool color

        float gradient = smoothIteration;// / float(MAX_ITERATIONS);
        //float3 color = gradient.xxx / MAX_ITERATIONS;

        float3 color = 0.5 + 0.5 * cos(3.0 + gradient.xxx * 0.15 + float3(0.0,0.6,1.0));
        renderTexture[threadId.xy] = float4(color, 1.0);
    }
    #endif
}
