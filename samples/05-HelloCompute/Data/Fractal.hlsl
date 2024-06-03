struct ShaderParameters
{
    float4 Translation;
    uint32_t RenderTextureIndex;
    float Zoom;
    float AspectRatio;
    uint32_t TriangleColor;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

float2 ComplexSquareAdd(float2 current, float2 constant) 
{
    float zr = current.x * current.x - current.y * current.y;
    float zi = 2.0 * current.x * current.y;

    return float2(zr, zi) + constant;
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

        uv.x *= aspectRatio;

        uv *= parameters.Zoom;
        uv += parameters.Translation;

        float2 Z = uv;
        float2 C = float2(-0.8, 0.15);

        uint iteration;

        for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) 
        {
            Z = ComplexSquareAdd(Z, C);

            if (dot(Z, Z) > 4.0) 
            {
                break;
            }
        }

        // TODO: HDR

        float mod = length(Z);
        float smoothIteration = float(iteration) - log2(max(1.0, log2(mod)));

        float color = smoothIteration / float(MAX_ITERATIONS);
        renderTexture[threadId.xy] = float4(color.xxx, 1.0f);
    }
    #endif
}
