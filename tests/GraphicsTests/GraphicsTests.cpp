#pragma once

#include "GraphicsTests.h"

bool testPrintLogs = true;
bool testForceVulkanApi = false;

bool workingTestHasLogErrors = false;
char workingTestErrorLogs[TESTLOG_LENGTH];
uint32_t currentTestErrorLogsIndex;

char testDebugLogs[TESTLOG_LENGTH];
uint32_t currentTestDebugLogsIndex = 0;

bool testHasLogErrors = false;
char testErrorLogs[TESTLOG_LENGTH];

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
        return
        {
            .Items = NULL,
            .Length = 0
        };
    }

    if (fseek(file, 0, SEEK_END) != 0) 
    {
        fclose(file);

        return
        {
            .Items = NULL,
            .Length = 0
        };
    }

    long fileSize = ftell(file);

    if (fileSize == -1) 
    {
        fclose(file);

        return 
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

        return
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

        return
        {
            .Items = NULL,
            .Length = 0
        };
    }

    fclose(file);

    return
    {
        .Items = buffer,
        .Length = (uint32_t)bytesRead
    };
}

const char* ReplaceShaderFilenameForVulkan(const char* original) 
{
    // Find the position of the dot in the original string
    const char* dot = strrchr(original, '.');
    if (!dot) {
        // If no dot is found, return the original string
        return original;
    }

    // Calculate lengths of different parts
    size_t baseLength = dot - original;
    size_t suffixLength = strlen(dot);
    const char* appendStr = "_vulkan";
    size_t appendStrLength = strlen(appendStr);

    // Allocate memory for the new string
    size_t newLength = baseLength + appendStrLength + suffixLength + 1;
    char* newString = (char*)malloc(newLength);
    if (!newString) {
        // Handle memory allocation failure
        return NULL;
    }

    // Copy the base part
    strncpy(newString, original, baseLength);
    newString[baseLength] = '\0'; // Null-terminate the base part

    // Append the "_vulkan" string
    strcat(newString, appendStr);

    // Append the suffix part
    strcat(newString, dot);

    return newString;
}

void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
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
        CopyString(logCopyDestination, TESTLOG_LENGTH - currentTestErrorLogsIndex, message, strlen(message) + 1);
        currentTestErrorLogsIndex += strlen(message);
    }
    else if (messageType == ElemLogMessageType_Debug)
    {
        char tmpMessage[TESTLOG_LENGTH];
        snprintf(tmpMessage, TESTLOG_LENGTH, "%s\n", message);
        auto tmpMessageLength = strlen(tmpMessage);
        
        char* logCopyDestination = testDebugLogs + currentTestDebugLogsIndex;
        CopyString(logCopyDestination, TESTLOG_LENGTH - currentTestDebugLogsIndex, tmpMessage, tmpMessageLength + 1);
        currentTestDebugLogsIndex += tmpMessageLength;
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
    CopyString(logCopyDestination, TESTLOG_LENGTH, workingTestErrorLogs, strlen(workingTestErrorLogs) + 1);
    currentTestErrorLogsIndex += strlen(workingTestErrorLogs);

    workingTestHasLogErrors = false;
    currentTestErrorLogsIndex = 0u;
    currentTestDebugLogsIndex = 0u;
}

ElemShaderLibrary TestOpenShader(ElemGraphicsDevice graphicsDevice, const char* shader)
{
    if (testForceVulkanApi)
    {
        shader = ReplaceShaderFilenameForVulkan(shader);
    }

    auto shaderData = ReadFile(shader);
    return ElemCreateShaderLibrary(graphicsDevice, shaderData);
}

ElemPipelineState TestOpenComputeShader(ElemGraphicsDevice graphicsDevice, const char* shader, const char* function)
{
    auto shaderLibrary = TestOpenShader(graphicsDevice, shader);

    ElemComputePipelineStateParameters pipelineStateParameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = function
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &pipelineStateParameters);

    if (pipelineState == ELEM_HANDLE_NULL)
    {
        return ELEM_HANDLE_NULL;
    }

    ElemFreeShaderLibrary(shaderLibrary);
    return pipelineState;
}

ElemPipelineState TestOpenMeshShader(ElemGraphicsDevice graphicsDevice, const char* shader, const char* meshShaderFunction, const char* pixelShaderFunction, ElemGraphicsFormat renderTargetFormat)
{
    auto shaderLibrary = TestOpenShader(graphicsDevice, shader);

    ElemGraphicsPipelineStateParameters pipelineStateParameters =
    {
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = meshShaderFunction,
        .PixelShaderFunction = pixelShaderFunction,
        .RenderTargetFormats = { .Items = &renderTargetFormat, .Length = 1 }
    };

    auto pipelineState = ElemCompileGraphicsPipelineState(graphicsDevice, &pipelineStateParameters);

    if (pipelineState == ELEM_HANDLE_NULL)
    {
        return ELEM_HANDLE_NULL;
    }

    ElemFreeShaderLibrary(shaderLibrary);
    return pipelineState;
}

TestGpuBuffer TestCreateGpuBuffer(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsHeapType heapType)
{
    ElemGraphicsHeapOptions heapOptions =
    {
        .HeapType = heapType
    };

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, sizeInBytes, &heapOptions);

    auto gpuBufferInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Write, nullptr);
    auto gpuBuffer = ElemCreateGraphicsResource(graphicsHeap, 0, &gpuBufferInfo);
    auto gpuBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Read, nullptr);
    auto gpuBufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    return
    {
        .GraphicsHeap = graphicsHeap,
        .Buffer = gpuBuffer,
        .ReadDescriptor = gpuBufferReadDescriptor,
        .WriteDescriptor = gpuBufferWriteDescriptor
    };
}

void TestFreeGpuBuffer(TestGpuBuffer gpuBuffer)
{
    ElemFreeGraphicsResourceDescriptor(gpuBuffer.WriteDescriptor, nullptr);
    ElemFreeGraphicsResourceDescriptor(gpuBuffer.ReadDescriptor, nullptr);
    ElemFreeGraphicsResource(gpuBuffer.Buffer, nullptr);
    ElemFreeGraphicsHeap(gpuBuffer.GraphicsHeap);
}

TestGpuTexture TestCreateGpuTexture(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage)
{
    auto textureInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, width, height, 1, format, usage, nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, textureInfo.SizeInBytes, nullptr);
    auto texture = ElemCreateGraphicsResource(graphicsHeap, 0, &textureInfo);
    auto textureReadDescriptor = ElemCreateGraphicsResourceDescriptor(texture, ElemGraphicsResourceDescriptorUsage_Read, nullptr);

    auto textureWriteDescriptor = -1;

    if (usage & ElemGraphicsResourceUsage_Write)
    {
        textureWriteDescriptor = ElemCreateGraphicsResourceDescriptor(texture, ElemGraphicsResourceDescriptorUsage_Write, nullptr);
    }

    return
    {
        .GraphicsHeap = graphicsHeap,
        .Texture = texture,
        .ReadDescriptor = textureReadDescriptor,
        .WriteDescriptor = textureWriteDescriptor,
        .Format = format
    };
}

void TestFreeGpuTexture(TestGpuTexture texture)
{
    ElemFreeGraphicsResourceDescriptor(texture.ReadDescriptor, nullptr);

    if (texture.WriteDescriptor != -1)
    {
        ElemFreeGraphicsResourceDescriptor(texture.WriteDescriptor, nullptr);
    }

    ElemFreeGraphicsResource(texture.Texture, nullptr);
    ElemFreeGraphicsHeap(texture.GraphicsHeap);
}

template<typename T>
void TestDispatchCompute(ElemCommandList commandList, ElemPipelineState pipelineState, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, std::initializer_list<T> parameters)
{
    ElemBindPipelineState(commandList, pipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)parameters.begin(), .Length = (uint32_t)(parameters.size() * sizeof(T)) });

    ElemDispatchCompute(commandList, threadGroupSizeX, threadGroupSizeY, threadGroupSizeZ);
}

template<typename T>
void TestDispatchComputeForReadbackBuffer(ElemGraphicsDevice graphicsDevice, ElemCommandQueue commandQueue, const char* shaderName, const char* function, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, const T* parameters)
{
    auto pipelineState = TestOpenComputeShader(graphicsDevice, shaderName, function);

    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    
    ElemBindPipelineState(commandList, pipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)parameters, .Length = sizeof(T) });

    ElemDispatchCompute(commandList, threadGroupSizeX, threadGroupSizeY, threadGroupSizeZ);

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    ElemFreePipelineState(pipelineState);
}

void TestBarrierCheckSyncTypeToString(char* destination, ElemGraphicsResourceBarrierSyncType syncType)
{
    switch (syncType) 
    {
        case ElemGraphicsResourceBarrierSyncType_None: snprintf(destination, 255, "None"); break;
        case ElemGraphicsResourceBarrierSyncType_Compute: snprintf(destination, 255, "Compute"); break;
        case ElemGraphicsResourceBarrierSyncType_RenderTarget: snprintf(destination, 255, "RenderTarget"); break;
        default: snprintf(destination, 255, "Unknown"); 
    }
}

void TestBarrierCheckAccessTypeToString(char* destination, ElemGraphicsResourceBarrierAccessType accessType)
{
    switch (accessType) 
    {
        case ElemGraphicsResourceBarrierAccessType_NoAccess: snprintf(destination, 255, "NoAccess"); break;
        case ElemGraphicsResourceBarrierAccessType_Read: snprintf(destination, 255, "Read"); break;
        case ElemGraphicsResourceBarrierAccessType_Write: snprintf(destination, 255, "Write"); break;
        case ElemGraphicsResourceBarrierAccessType_RenderTarget: snprintf(destination, 255, "RenderTarget"); break;
        default: snprintf(destination, 255, "Unknown"); 
    }
}

void TestBarrierCheckLayoutTypeToString(char* destination, ElemGraphicsResourceBarrierLayoutType layoutType)
{
    switch (layoutType) 
    {
        case ElemGraphicsResourceBarrierLayoutType_Read: snprintf(destination, 255, "Read"); break;
        case ElemGraphicsResourceBarrierLayoutType_Write: snprintf(destination, 255, "Write"); break;
        case ElemGraphicsResourceBarrierLayoutType_RenderTarget: snprintf(destination, 255, "RenderTarget"); break;
        default: snprintf(destination, 255, "Undefined"); 
    }
}

bool TestDebugLogBarrier(const TestBarrierCheck* check, char* expectedMessage, uint32_t messageLength)
{
    auto currentDestination = expectedMessage;
    snprintf(currentDestination, messageLength, "BarrierCommand: Buffer=%d, Texture=%d\n", check->BufferBarrierCount, check->TextureBarrierCount);

    for (uint32_t i = 0; i < check->BufferBarrierCount; i++)
    {
        currentDestination = expectedMessage + strlen(expectedMessage);
        auto bufferBarrier = check->BufferBarriers[i];

        char syncBefore[255];
        TestBarrierCheckSyncTypeToString(syncBefore, bufferBarrier.SyncBefore);

        char syncAfter[255];
        TestBarrierCheckSyncTypeToString(syncAfter, bufferBarrier.SyncAfter);

        char accessBefore[255];
        TestBarrierCheckAccessTypeToString(accessBefore, bufferBarrier.AccessBefore);

        char accessAfter[255];
        TestBarrierCheckAccessTypeToString(accessAfter, bufferBarrier.AccessAfter);

        snprintf(currentDestination, messageLength, "  BarrierBuffer: Resource=%u, SyncBefore=%s, SyncAfter=%s, AccessBefore=%s, AccessAfter=%s\n",
                (uint32_t)bufferBarrier.Resource,
                syncBefore,
                syncAfter,
                accessBefore,
                accessAfter);
    }

    for (uint32_t i = 0; i < check->TextureBarrierCount; i++)
    {
        currentDestination = expectedMessage + strlen(expectedMessage);
        auto textureBarrier = check->TextureBarriers[i];

        char syncBefore[255];
        TestBarrierCheckSyncTypeToString(syncBefore, textureBarrier.SyncBefore);

        char syncAfter[255];
        TestBarrierCheckSyncTypeToString(syncAfter, textureBarrier.SyncAfter);

        char accessBefore[255];
        TestBarrierCheckAccessTypeToString(accessBefore, textureBarrier.AccessBefore);

        char accessAfter[255];
        TestBarrierCheckAccessTypeToString(accessAfter, textureBarrier.AccessAfter);

        char layoutBefore[255];
        TestBarrierCheckLayoutTypeToString(layoutBefore, textureBarrier.LayoutBefore);

        char layoutAfter[255];
        TestBarrierCheckLayoutTypeToString(layoutAfter, textureBarrier.LayoutAfter);

        snprintf(currentDestination, messageLength, "  BarrierTexture: Resource=%u, SyncBefore=%s, SyncAfter=%s, AccessBefore=%s, AccessAfter=%s, LayoutBefore=%s, LayoutAfter=%s\n",
                (uint32_t)textureBarrier.Resource,
                syncBefore,
                syncAfter,
                accessBefore,
                accessAfter,
                layoutBefore,
                layoutAfter);
    }

    auto result = (strstr(testDebugLogs, expectedMessage) != NULL);
    currentTestDebugLogsIndex = 0u;

    return result;
}
