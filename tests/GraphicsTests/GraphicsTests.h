#pragma once

#include "Elemental.h"
#include <initializer_list> 

#define TESTLOG_LENGTH 4096

#ifndef _WIN32
#define MAX_PATH 255
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

#define ASSERT_LOG_NOERROR() { TestInitLog(); ASSERT_FALSE_MSG(testHasLogErrors, testErrorLogs); }
#define ASSERT_LOG_MESSAGE(message) { TestInitLog(); ASSERT_TRUE_MSG(strstr(testErrorLogs, message) != NULL, message); }
#define ASSERT_LOG_MESSAGE_DEBUG(message) { TestInitLog(); ASSERT_TRUE_MSG(strstr(testDebugLogs, message) != NULL, message); }

#define BUFFER_BARRIER(resource, syncBefore, syncAfter, accessBefore, accessAfter) \
    { \
        .Resource = resource, \
        .SyncBefore = syncBefore, \
        .SyncAfter = syncAfter, \
        .AccessBefore = accessBefore, \
        .AccessAfter = accessAfter \
    }

#define TEXTURE_BARRIER(resource, syncBefore, syncAfter, accessBefore, accessAfter, layoutBefore, layoutAfter) \
    { \
        .Resource = resource, \
        .SyncBefore = syncBefore, \
        .SyncAfter = syncAfter, \
        .AccessBefore = accessBefore, \
        .AccessAfter = accessAfter, \
        .LayoutBefore = layoutBefore, \
        .LayoutAfter = layoutAfter \
    }

#define BARRIER_ARRAY_EMPTY() \
    {\
        { .Resource = 0 } \
    }

#define BARRIER_ARRAY(...) \
    {\
        __VA_ARGS__ \
    }

#define BARRIER_ARRAY_IS_EMPTY(array) \
    (array[0].Resource == 0)

#define INIT_ASSERT_BARRIER(name, bufferBarriers, textureBarriers) \
    TestBarrierCheckBuffer test_##name##BufferBarriers[] = bufferBarriers; \
    TestBarrierCheckTexture test_##name##TextureBarriers[] = textureBarriers; \
\
    TestBarrierCheck test_##name##Params = \
    { \
        .BufferBarrierCount = BARRIER_ARRAY_IS_EMPTY(test_##name##BufferBarriers) ? 0u : (uint32_t)ARRAYSIZE(test_##name##BufferBarriers), \
        .BufferBarriers = test_##name##BufferBarriers, \
        .TextureBarrierCount = BARRIER_ARRAY_IS_EMPTY(test_##name##TextureBarriers) ? 0u : (uint32_t)ARRAYSIZE(test_##name##TextureBarriers), \
        .TextureBarriers = test_##name##TextureBarriers \
    }; \
\
    char test_##name##ExpectedMessage[1024]; \
    auto test_##name = TestDebugLogBarrier(&test_##name##Params, test_##name##ExpectedMessage, 1024);

#define ASSERT_BARRIER(name) ASSERT_TRUE_MSG(test_##name, test_##name##ExpectedMessage)

struct TestGpuBuffer
{
    ElemGraphicsHeap GraphicsHeap;
    ElemGraphicsResource Buffer;
    ElemGraphicsResourceDescriptor ReadDescriptor;
    ElemGraphicsResourceDescriptor WriteDescriptor;
};

struct TestGpuTexture
{
    ElemGraphicsHeap GraphicsHeap;
    ElemGraphicsResource Texture;
    ElemGraphicsResourceDescriptor ReadDescriptor;
    ElemGraphicsResourceDescriptor WriteDescriptor;
    ElemGraphicsFormat Format;
};

struct TestBarrierCheckBuffer
{
    ElemGraphicsResource Resource;
    ElemGraphicsResourceBarrierSyncType SyncBefore; 
    ElemGraphicsResourceBarrierSyncType SyncAfter; 
    ElemGraphicsResourceBarrierAccessType AccessBefore;
    ElemGraphicsResourceBarrierAccessType AccessAfter;
};

struct TestBarrierCheckTexture
{
    ElemGraphicsResource Resource;
    ElemGraphicsResourceBarrierSyncType SyncBefore; 
    ElemGraphicsResourceBarrierSyncType SyncAfter; 
    ElemGraphicsResourceBarrierAccessType AccessBefore;
    ElemGraphicsResourceBarrierAccessType AccessAfter;
    ElemGraphicsResourceBarrierLayoutType LayoutBefore;
    ElemGraphicsResourceBarrierLayoutType LayoutAfter;
};

struct TestBarrierCheck
{
    uint32_t BufferBarrierCount;
    TestBarrierCheckBuffer* BufferBarriers;
    uint32_t TextureBarrierCount;
    TestBarrierCheckTexture* TextureBarriers;
};


// TODO: Review
extern bool testPrintLogs;
extern bool testForceVulkanApi;
extern bool testHasLogErrors;
extern char testErrorLogs[TESTLOG_LENGTH];
extern uint32_t currentTestErrorLogsIndex;

uint64_t TestMegaBytesToBytes(uint64_t value);

void CopyString(char* destination, uint32_t destinationLength, const char* source, uint32_t sourceLength);
void GetFullPath(char* destination, const char* path);
ElemDataSpan ReadFile(const char* filename); 
void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message); 

void TestInitLog();

ElemShaderLibrary TestOpenShader(ElemGraphicsDevice graphicsDevice, const char* shader);
ElemPipelineState TestOpenComputeShader(ElemGraphicsDevice graphicsDevice, const char* shader, const char* function);
ElemPipelineState TestOpenMeshShader(ElemGraphicsDevice graphicsDevice, const char* shader, const char* meshShaderFunction, const char* pixelShaderFunction, ElemGraphicsFormat renderTargetFormat);

TestGpuBuffer TestCreateGpuBuffer(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsHeapType heapType = ElemGraphicsHeapType_Gpu);
void TestFreeGpuBuffer(TestGpuBuffer gpuBuffer);

TestGpuTexture TestCreateGpuTexture(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, ElemGraphicsFormat format);
void TestFreeGpuTexture(TestGpuTexture texture);

template<typename T>
void TestDispatchCompute(ElemCommandList commandList, ElemPipelineState pipelineState, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, std::initializer_list<T> parameters);

template<typename T>
void TestDispatchComputeForReadbackBuffer(ElemGraphicsDevice graphicsDevice, ElemCommandQueue commandQueue, const char* shaderName, const char* function, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, const T* parameters);

void TestBeginClearRenderPass(ElemCommandList commandList, ElemGraphicsResource renderTarget, ElemColor clearColor);

void TestBarrierCheckSyncTypeToString(char* destination, ElemGraphicsResourceBarrierSyncType syncType);
void TestBarrierCheckAccessTypeToString(char* destination, ElemGraphicsResourceBarrierAccessType accessType);
bool TestDebugLogBarrier(const TestBarrierCheck* check, char* expectedMessage, uint32_t messageLength);
