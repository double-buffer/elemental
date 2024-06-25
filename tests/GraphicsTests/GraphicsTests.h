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

// TODO: Review
extern bool testPrintLogs;
extern bool testForceVulkanApi;
extern bool testHasLogErrors;
extern char testErrorLogs[2048];
extern uint32_t currentTestErrorLogsIndex;

extern ElemGraphicsDevice sharedGraphicsDevice;
extern ElemCommandQueue sharedCommandQueue;

extern ElemSystemInfo sharedSystemInfo;
extern ElemGraphicsDeviceInfo sharedGraphicsDeviceInfo;

uint64_t TestMegaBytesToBytes(uint64_t value);

void CopyString(char* destination, uint32_t destinationLength, const char* source, uint32_t sourceLength);
void GetFullPath(char* destination, const char* path);
ElemDataSpan ReadFile(const char* filename); 
static inline void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message); 

void TestInitLog();

ElemGraphicsDevice TestGetSharedGraphicsDevice();
ElemSystemInfo TestGetSharedSystemInfo();
ElemGraphicsDeviceInfo TestGetSharedGraphicsDeviceInfo();

ElemCommandQueue TestGetSharedCommandQueue();

ElemShaderLibrary TestOpenShader(const char* shader);
TestReadBackBuffer TestCreateReadbackBuffer(uint32_t sizeInBytes);
void TestFreeReadbackBuffer(TestReadBackBuffer readbackBuffer);

template<typename T>
void TestDispatchComputeForReadbackBuffer(const char* shaderName, const char* function, uint32_t threadGroupSizeX, uint32_t threadGroupSizeY, uint32_t threadGroupSizeZ, const T* parameters);
