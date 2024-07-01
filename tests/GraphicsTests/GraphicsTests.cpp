#pragma once

#include "GraphicsTests.h"

bool testPrintLogs = true;
bool testForceVulkanApi = false;

bool workingTestHasLogErrors = false;
char workingTestErrorLogs[2048];
uint32_t currentTestErrorLogsIndex;

bool testHasLogErrors = false;
char testErrorLogs[2048];

uint64_t TestMegaBytesToBytes(uint64_t value)
{
    return value * 1024 * 1024;
}

void CopyString(char* destination, uint32_t destinationLength, const char* source, uint32_t sourceLength)
{
    #ifdef _WIN32 
    strncpy_s(destination, destinationLength, source, sourceLength);
    #else
    strncpy(destination, source, sourceLength);
    #endif
}

void GetFullPath(char* destination, const char* path)
{
    memset(destination, 0, MAX_PATH);

    char* pointer = NULL;
    ElemSystemInfo systemInfo = ElemGetSystemInfo();

    CopyString(destination, MAX_PATH, systemInfo.ApplicationPath, strlen(systemInfo.ApplicationPath));
    pointer = destination + strlen(systemInfo.ApplicationPath);

    const char* folderPrefix = "Data/";

    if (systemInfo.Platform == ElemPlatform_MacOS)
    {
        folderPrefix = "../Resources/";
    }
    else if (systemInfo.Platform == ElemPlatform_iOS)
    {
        folderPrefix = "./";
    }

    CopyString(pointer, pointer - destination, folderPrefix, strlen(folderPrefix));
    pointer = pointer + strlen(folderPrefix);

    CopyString(pointer, pointer - destination, path, strlen(path));
}

ElemDataSpan ReadFile(const char* filename) 
{
    char absolutePath[MAX_PATH];
    GetFullPath(absolutePath, filename);

    #ifdef _WIN32
    FILE* file;
    fopen_s(&file, absolutePath, "rb");
    #else
    FILE* file = fopen(absolutePath, "rb");
    #endif

    if (file == NULL) 
    {
        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    if (fseek(file, 0, SEEK_END) != 0) 
    {
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    long fileSize = ftell(file);

    if (fileSize == -1) 
    {
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    rewind(file);

    uint8_t* buffer = (uint8_t*)malloc(fileSize + 1);

    if (buffer == NULL)
    {
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, file);

    if (bytesRead < (size_t)fileSize) 
    {
        free(buffer);
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    fclose(file);

    return (ElemDataSpan) 
    {
        .Items = buffer,
        .Length = (uint32_t)bytesRead
    };
}

static inline void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
{
    if (testPrintLogs)
    {
        printf("[");
        printf("\033[36m");

        if (category == ElemLogMessageCategory_Assert)
        {
            printf("Assert");
        }
        else if (category == ElemLogMessageCategory_Memory)
        {
            printf("Memory");
        }
        else if (category == ElemLogMessageCategory_Application)
        {
            printf("Application");
        }
        else if (category == ElemLogMessageCategory_Graphics)
        {
            printf("Graphics");
        }
        else if (category == ElemLogMessageCategory_Inputs)
        {
            printf("Inputs");
        }

        printf("\033[0m]");
        printf("\033[32m %s", function);

        if (messageType == ElemLogMessageType_Error)
        {
            testHasLogErrors = true;
            printf("\033[31m Error:");
        }
        else if (messageType == ElemLogMessageType_Warning)
        {
            printf("\033[33m Warning:");
        }
        else if (messageType == ElemLogMessageType_Debug)
        {
            printf("\033[0m Debug:");
        }
        else
        {
            printf("\033[0m");
        }

        printf(" %s\033[0m\n", message);
        fflush(stdout);
    }

    if (messageType == ElemLogMessageType_Error)
    {
        workingTestHasLogErrors = true;

        char* logCopyDestination = workingTestErrorLogs + currentTestErrorLogsIndex;
        CopyString(logCopyDestination, 2048, message, strlen(message) + 1);
        currentTestErrorLogsIndex += strlen(message);
    }
}

void TestInitLog()
{
    #ifdef _WIN32
    Sleep(1);
    #else
    usleep(1000);
    #endif

    testHasLogErrors = workingTestHasLogErrors;

    char* logCopyDestination = testErrorLogs;
    CopyString(logCopyDestination, 2048, workingTestErrorLogs, strlen(workingTestErrorLogs) + 1);
    currentTestErrorLogsIndex += strlen(workingTestErrorLogs);

    workingTestHasLogErrors = false;
    currentTestErrorLogsIndex = 0u;
}

ElemShaderLibrary TestOpenShader(ElemGraphicsDevice graphicsDevice, const char* shader)
{
    // TODO: Vulkan shaders on windows
    auto shaderData = ReadFile(shader);
    auto shaderLibrary = ElemCreateShaderLibrary(graphicsDevice, shaderData);
    return shaderLibrary;
}

TestReadBackBuffer TestCreateReadbackBuffer(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes)
{
    ElemGraphicsHeapOptions heapOptions =
    {
        .HeapType = ElemGraphicsHeapType_Readback
    };

    auto readBackGraphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, sizeInBytes, &heapOptions);

    auto bufferInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Uav, nullptr); 
    auto buffer = ElemCreateGraphicsResource(readBackGraphicsHeap, 0, &bufferInfo);
    auto bufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(buffer, ElemGraphicsResourceUsage_Uav, nullptr);

    return
    {
        .GraphicsHeap = readBackGraphicsHeap,
        .Buffer = buffer,
        .Descriptor = bufferWriteDescriptor
    };
}

void TestFreeReadbackBuffer(TestReadBackBuffer readbackBuffer)
{
    ElemFreeGraphicsResourceDescriptor(readbackBuffer.Descriptor, nullptr);
    ElemFreeGraphicsResource(readbackBuffer.Buffer, nullptr);
    ElemFreeGraphicsHeap(readbackBuffer.GraphicsHeap);
}

TestRenderTarget TestCreateRenderTarget(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, ElemGraphicsFormat format)
{
    auto textureInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 16, 16, 1, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget, nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, textureInfo.SizeInBytes, nullptr);
    auto texture = ElemCreateGraphicsResource(graphicsHeap, 0, &textureInfo);
    auto textureReadDescriptor = ElemCreateGraphicsResourceDescriptor(texture, ElemGraphicsResourceUsage_Standard, nullptr);
    auto renderTargetDescriptor = ElemCreateGraphicsResourceDescriptor(texture, ElemGraphicsResourceUsage_RenderTarget, nullptr);

    return
    {
        .GraphicsHeap = graphicsHeap,
        .Texture = texture,
        .ReadDescriptor = textureReadDescriptor,
        .RenderTargetDescriptor = renderTargetDescriptor,
        .Format = format
    };
}

void TestFreeRenderTarget(TestRenderTarget renderTarget)
{
    ElemFreeGraphicsResourceDescriptor(renderTarget.RenderTargetDescriptor, nullptr);
    ElemFreeGraphicsResourceDescriptor(renderTarget.ReadDescriptor, nullptr);
    ElemFreeGraphicsResource(renderTarget.Texture, nullptr);
    ElemFreeGraphicsHeap(renderTarget.GraphicsHeap);
}

template<typename T>
void TestDispatchComputeForReadbackBuffer(ElemGraphicsDevice graphicsDevice, ElemCommandQueue commandQueue, const char* shaderName, const char* function, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, const T* parameters)
{
    auto shaderLibrary = TestOpenShader(graphicsDevice, shaderName);

    ElemComputePipelineStateParameters pipelineStateParameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = function
    };

    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &pipelineStateParameters);

    if (pipelineState == ELEM_HANDLE_NULL)
    {
        return;
    }
    
    ElemBindPipelineState(commandList, pipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)parameters, .Length = sizeof(T) });

    ElemDispatchCompute(commandList, threadGroupSizeX, threadGroupSizeY, threadGroupSizeZ);

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    ElemFreePipelineState(pipelineState);
    ElemFreeShaderLibrary(shaderLibrary);
}
