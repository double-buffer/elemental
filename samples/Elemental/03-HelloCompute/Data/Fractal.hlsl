struct ShaderParameters
{
    uint32_t RenderTextureIndex;
    float Zoom;
    float2 Reserved;
    float4x4 Transform;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

float2 ComplexSquareAdd(float2 current, float2 constant) 
{
    float zr = current.x * current.x - current.y * current.y;
    float zi = 2.0 * current.x * current.y;

    return float2(zr, zi) + constant;
}

float hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453123);
}

float2 random2(float2 uv)
{
    float x = hash(uv + float2(1.0, 0.0));
    float y = hash(uv + float2(0.0, 1.0));
    
    return (float2(x, y) * 2.0 - 1.0) * 0.5;
}

#define MAX_ITERATIONS 500
#define BOUNDARY 256

float ComputeJuliaSet(float2 position)
{
    float2 Z = position;
    //float2 C = float2(-0.8, 0.153);
    float2 C = float2(-0.8, 0.16);

    uint iteration;

    for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) 
    {
        Z = ComplexSquareAdd(Z, C);

        if (dot(Z, Z) > BOUNDARY * BOUNDARY) 
        {
            break;
        }
    }

    return float(iteration) - log2(log2(dot(Z, Z))) + 4.0;
}

float ComputeMandelbrotSet(float2 position)
{
    float2 Z = float2(0.0, 0.0);
    float2 C = position;

    uint iteration;

    for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) 
    {
        Z = ComplexSquareAdd(Z, C);

        if (dot(Z, Z) > BOUNDARY * BOUNDARY) 
        {
            break;
        }
    }

    return float(iteration) - log2(log2(dot(Z, Z))) + 4.0;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void Fractal(uint3 threadId: SV_DispatchThreadID)
{
    RWTexture2D<float4> renderTexture = ResourceDescriptorHeap[parameters.RenderTextureIndex];

    // TODO: This is slower, pass the width and height!
    uint width, height;
    renderTexture.GetDimensions(width, height);

    // BUG: On metal when out of bound, it is not black but gray (like uninit)

    if (threadId.x < width && threadId.y < height)
    {
        float aspectRatio = (float)width / height;
        float2 uv = (threadId.xy / float2(width, height)) * 2.0 - 1.0;
        uv.x *= aspectRatio;
        uv *= parameters.Zoom;

        uv = mul(float4(uv, 0.0, 1.0), parameters.Transform);

        float gradient = ComputeJuliaSet(uv);
        //float gradient = ComputeMandelbrotSet(uv);

        float3 color = 0.5 + 0.5 * cos(2.5 + gradient.xxx * 0.15 + float3(0.0, 0.6, 1.0)).rgb;
        
        renderTexture[threadId.xy] = float4(color, 1.0);
    }
}
