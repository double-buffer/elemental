#pragma once

#include "Elemental.h"
#include <initializer_list> 

#ifndef _WIN32
#define MAX_PATH 255
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

#define ASSERT_LOG_NOERROR() { TestInitLog(); ASSERT_FALSE_MSG(testHasLogErrors, testErrorLogs); }
#define ASSERT_LOG_MESSAGE(message) { TestInitLog(); ASSERT_TRUE_MSG(strstr(testErrorLogs, message) != NULL, message); }

#define BUFFER_BARRIER(resource, syncBefore, syncAfter, accessBefore, accessAfter) \
    { \
        .Resource = resource, \
        .SyncBefore = syncBefore, \
        .SyncAfter = syncAfter, \
        .AccessBefore = accessBefore, \
        .AccessAfter = accessAfter \
    }

#define BARRIER_ARRAY(...) \
    {\
        __VA_ARGS__ \
    }

#define INIT_ASSERT_BARRIER(name, bufferBarriers) \
    TestBarrierCheckBuffer test_##name##BufferBarriers[] = bufferBarriers; \
\
    TestBarrierCheck test_##name##Params = \
    { \
        .BufferBarrierCount = ARRAYSIZE(test_##name##BufferBarriers), \
        .BufferBarriers = test_##name##BufferBarriers \
    }; \
\
    char test_##name##ExpectedMessage[1024]; \
    auto test_##name = TestDebugLogBarrier(&test_##name##Params, test_##name##ExpectedMessage, 1024);

#define ASSERT_BARRIER(name) ASSERT_TRUE_MSG(test_##name, test_##name##ExpectedMessage)

enum TestBarrierCheckSyncType
{
    SyncType_None,
    SyncType_Compute
};

enum TestBarrierCheckAccessType
{
    AccessType_NoAccess,
    AccessType_Read,
    AccessType_Write
};

struct TestGpuBuffer
{
    ElemGraphicsHeap GraphicsHeap;
    ElemGraphicsResource Buffer;
    ElemGraphicsResourceDescriptor ReadDescriptor;
    ElemGraphicsResourceDescriptor WriteDescriptor;
};

struct TestRenderTarget
{
    ElemGraphicsHeap GraphicsHeap;
    ElemGraphicsResource Texture;
    ElemGraphicsResourceDescriptor ReadDescriptor;
    ElemGraphicsFormat Format;
};

struct TestBarrierCheckBuffer
{
    ElemGraphicsResource Resource;
    TestBarrierCheckSyncType SyncBefore; 
    TestBarrierCheckSyncType SyncAfter; 
    TestBarrierCheckAccessType AccessBefore;
    TestBarrierCheckAccessType AccessAfter;
};

struct TestBarrierCheckTexture
{
    ElemGraphicsResource Resource;
    TestBarrierCheckSyncType SyncBefore; 
    TestBarrierCheckSyncType SyncAfter; 
    TestBarrierCheckAccessType AccessBefore;
    TestBarrierCheckAccessType AccessAfter;
    // TODO: Layouts
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
extern char testErrorLogs[2048];
extern uint32_t currentTestErrorLogsIndex;

uint64_t TestMegaBytesToBytes(uint64_t value);

void CopyString(char* destination, uint32_t destinationLength, const char* source, uint32_t sourceLength);
void GetFullPath(char* destination, const char* path);
ElemDataSpan ReadFile(const char* filename); 
void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message); 

void TestInitLog();

ElemShaderLibrary TestOpenShader(ElemGraphicsDevice graphicsDevice, const char* shader);
ElemPipelineState TestOpenComputeShader(ElemGraphicsDevice graphicsDevice, const char* shader, const char* function);

TestGpuBuffer TestCreateGpuBuffer(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsHeapType heapType = ElemGraphicsHeapType_Gpu);
void TestFreeGpuBuffer(TestGpuBuffer gpuBuffer);

TestRenderTarget TestCreateRenderTarget(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, ElemGraphicsFormat format);
void TestFreeRenderTarget(TestRenderTarget renderTarget);

template<typename T>
void TestDispatchCompute(ElemCommandList commandList, ElemPipelineState pipelineState, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, std::initializer_list<T> parameters);

template<typename T>
void TestDispatchComputeForReadbackBuffer(ElemGraphicsDevice graphicsDevice, ElemCommandQueue commandQueue, const char* shaderName, const char* function, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, const T* parameters);

void TestBarrierCheckSyncTypeToString(char* destination, TestBarrierCheckSyncType syncType);
void TestBarrierCheckAccessTypeToString(char* destination, TestBarrierCheckAccessType accessType);
bool TestDebugLogBarrier(const TestBarrierCheck* check, char* expectedMessage, uint32_t messageLength);
