using System.Numerics;
using Elemental;
using Elemental.Graphics;
using Elemental.Tools;

var applicationService = new NativeApplicationService();
using var graphicsService = new GraphicsService();

using var application = applicationService.CreateApplication("Hello Window");
using var window = applicationService.CreateWindow(application);

using var graphicsDevice = graphicsService.CreateGraphicsDevice();
using var commandQueue = graphicsService.CreateCommandQueue(graphicsDevice, CommandQueueType.Graphics);
using var swapChain = graphicsService.CreateSwapChain(window, commandQueue);
var currentRenderSize = applicationService.GetWindowRenderSize(window);

var shaderCode = 
    """
    #define RootSignatureDef "RootFlags(0)"

    struct Vertex { float3 Position; float4 Color; };
    struct VertexOutput { float4 Position: SV_Position; float4 Color: TEXCOORD0; };

    static Vertex triangleVertices[] =
    {
        { float3(-0.5, 0.5, 0.0), float4(1.0, 0.0, 0.0, 1.0) },
        { float3(0.5, 0.5, 0.0), float4(0.0, 1.0, 0.0, 1.0) },
        { float3(-0.5, -0.5, 0.0), float4(0.0, 0.0, 1.0, 1.0) }
    };

    [OutputTopology("triangle")]
    [NumThreads(32, 1, 1)]
    void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[3], out indices uint3 indices[1])
    {
        const uint meshVertexCount = 3;

        SetMeshOutputCounts(meshVertexCount, 1);

        if (groupThreadId < meshVertexCount)
        {
            vertices[groupThreadId].Position = float4(triangleVertices[groupThreadId].Position, 1.0);
            vertices[groupThreadId].Color = triangleVertices[groupThreadId].Color;
        }

        if (groupThreadId == 0)
        {
            indices[groupThreadId] = uint3(0, 1, 2);
        }
    }

    float4 PixelMain(const VertexOutput input) : SV_Target0
    {
        return input.Color; 
    }
    """;

using var shaderCompiler = new ShaderCompiler();

var shaderInputs = new ShaderCompilerInput[]
{
    new() { ShaderCode = shaderCode, Stage = ShaderStage.MeshShader, EntryPoint = "MeshMain", ShaderLanguage = ShaderLanguage.Hlsl },
    new() { ShaderCode = shaderCode, Stage = ShaderStage.PixelShader, EntryPoint = "PixelMain", ShaderLanguage = ShaderLanguage.Hlsl }
};

var shaderCompilationResults = shaderCompiler.CompileShaders(shaderInputs, graphicsService.GetGraphicsDeviceInfo(graphicsDevice).GraphicsApi);
var shaderParts = new ShaderPart[shaderCompilationResults.Length];

for (var i = 0; i < shaderCompilationResults.Length; i++)
{
    shaderParts[i] = new() 
    { 
        Stage = shaderCompilationResults[i].Stage, 
        EntryPoint = shaderCompilationResults[i].EntryPoint, 
        Data = shaderCompilationResults[i].ShaderData, 
        MetaData = shaderCompilationResults[i].MetaData
    };
}

using var shader = graphicsService.CreateShader(graphicsDevice, shaderParts);

applicationService.RunApplication(application, (status) =>
{
    if (status.IsClosing) return false;

    var renderSize = applicationService.GetWindowRenderSize(window);

    if (renderSize != currentRenderSize)
    {
        graphicsService.ResizeSwapChain(swapChain, renderSize.Width, renderSize.Height);
        currentRenderSize = renderSize;
    }

    graphicsService.WaitForSwapChainOnCpu(swapChain);

    var commandList = graphicsService.CreateCommandList(commandQueue);
    using var backbufferTexture = graphicsService.GetSwapChainBackBufferTexture(swapChain);
    graphicsService.BeginRenderPass(commandList, new() { RenderTarget0 = new() { Texture = backbufferTexture, ClearColor = new Vector4(0.0f, 1.0f, 1.0f, 1.0f) } });
    graphicsService.SetShader(commandList, shader);
    graphicsService.DispatchMesh(commandList, 1, 1, 1);
    graphicsService.EndRenderPass(commandList);
    graphicsService.CommitCommandList(commandList);
    graphicsService.ExecuteCommandList(commandQueue, commandList);

    graphicsService.PresentSwapChain(swapChain);
    return true;
});