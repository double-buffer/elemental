#include "Elemental.h"
#include "ElementalTools.h"

const char* shaderSource = 
    "struct Vertex { float3 Position; float4 Color; };"
    "struct VertexOutput { float4 Position: SV_Position; float4 Color: TEXCOORD0; };"

    "static Vertex triangleVertices[] ="
    "{"
    "    { float3(-0.5, 0.5, 0.0), float4(1.0, 0.0, 0.0, 1.0) },"
    "    { float3(0.5, 0.5, 0.0), float4(0.0, 1.0, 0.0, 1.0) },"
    "    { float3(-0.5, -0.5, 0.0), float4(0.0, 0.0, 1.0, 1.0) }"
    "};"

    "[shader(\"mesh\")]"
    "[OutputTopology(\"triangle\")]"
    "[NumThreads(32, 1, 1)]"
    "void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[3], out indices uint3 indices[1])"
    "{"
    "    const uint meshVertexCount = 3;"
    "    SetMeshOutputCounts(meshVertexCount, 1);"

    "    if (groupThreadId < meshVertexCount)"
    "    {"
    "        vertices[groupThreadId].Position = float4(triangleVertices[groupThreadId].Position, 1);"
    "        vertices[groupThreadId].Color = triangleVertices[groupThreadId].Color;"
    "    }"

    "    if (groupThreadId == 0)"
    "    {"
    "        indices[groupThreadId] = uint3(0, 1, 2);"
    "    }"
    "}"

    "[shader(\"pixel\")]"
    "float4 PixelMain(const VertexOutput input) : SV_Target0"
    "{"
    "    return input.Color;"
    "}";

typedef struct
{
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
} ApplicationPayload;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    applicationPayload->Window = ElemCreateWindow(NULL);

    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);
    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .UpdatePayload = payload });

    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);
    ElemSystemInfo systemInfo = ElemGetSystemInfo();
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(applicationPayload->GraphicsDevice);

    ElemShaderSourceData shaderSourceData = { .ShaderLanguage = ElemShaderLanguage_Hlsl, .Data = { .Items = (uint8_t*)shaderSource, .Length = strlen(shaderSource) } };
    ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary((ElemToolsGraphicsApi)graphicsDeviceInfo.GraphicsApi, (ElemToolsPlatform)systemInfo.Platform, &shaderSourceData, NULL);

    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, (ElemDataSpan) { .Items = compilationResult.Data.Items, .Length = compilationResult.Data.Length });

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargetFormats = { .Items = (ElemGraphicsFormat[]) { swapChainInfo.Format }, .Length = 1 }
    });
    
    ElemFreeShaderLibrary(shaderLibrary);
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) {{ 
                .RenderTarget = updateParameters->BackBufferRenderTarget, 
                .LoadAction = ElemRenderPassLoadAction_Clear 
            }},
            .Length = 1
        }
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);
}

int main(int argc, const char* argv[]) 
{
    ElemRunApplication(&(ElemRunApplicationParameters) {
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &(ApplicationPayload){}
    });
}
