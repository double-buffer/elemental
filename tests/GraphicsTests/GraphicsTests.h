#pragma once

#include "Elemental.h"

#define MAX_PATH 255

#define ASSERT_LOG_NOERROR() { TestInitLog(); ASSERT_FALSE_MSG(testHasLogErrors, testErrorLogs); }
#define ASSERT_LOG_MESSAGE(message) { TestInitLog(); ASSERT_TRUE_MSG(strstr(testErrorLogs, message) != NULL, message); }

struct TestReadBackBuffer
{
    ElemGraphicsHeap GraphicsHeap;
    ElemGraphicsResource Buffer;
    ElemGraphicsResourceDescriptor Descriptor;
};

struct TestRenderTarget
{
    ElemGraphicsHeap GraphicsHeap;
    ElemGraphicsResource Texture;
    ElemGraphicsResourceDescriptor ReadDescriptor;
    ElemGraphicsResourceDescriptor RenderTargetDescriptor;
    ElemGraphicsFormat Format;
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
static inline void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message); 

void TestInitLog();

ElemShaderLibrary TestOpenShader(ElemGraphicsDevice graphicsDevice, const char* shader);

TestReadBackBuffer TestCreateReadbackBuffer(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes);
void TestFreeReadbackBuffer(TestReadBackBuffer readbackBuffer);

TestRenderTarget TestCreateRenderTarget(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, ElemGraphicsFormat format);
void TestFreeRenderTarget(TestRenderTarget renderTarget);

template<typename T>
void TestDispatchComputeForReadbackBuffer(ElemGraphicsDevice graphicsDevice, ElemCommandQueue commandQueue, const char* shaderName, const char* function, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, const T* parameters);
