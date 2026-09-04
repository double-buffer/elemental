#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#define MAX_PATH 255
#endif

#include "Elemental.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MESH_FILE_VERSION 1u
#define MESH_VERTEX_SIZE_IN_BYTES (sizeof(float) * 12u)
#define MESHLET_SIZE_IN_BYTES (sizeof(uint32_t) * 4u)

#define ROTATION_TOUCH_DECREASE_SPEED 0.001f
#define ROTATION_TOUCH_SPEED 4.0f
#define ROTATION_TOUCH_MAX_DELTA 0.3f
#define ROTATION_MULTITOUCH_SPEED 200.0f
#define ROTATION_ACCELERATION 500.0f
#define ROTATION_FRICTION 60.0f
#define ZOOM_MULTITOUCH_SPEED 1000.0f
#define ZOOM_SPEED 5.0f

typedef struct
{
    float X;
    float Y;
} SampleVector2;

typedef union
{
    struct
    {
        float X;
        float Y;
        float Z;
        float W;
    };

    struct
    {
        ElemVector3 XYZ;
    };
} SampleVector4;

#define V2_ZERO (SampleVector2) { .X = 0.0f, .Y = 0.0f }
#define V3_ZERO (ElemVector3) { .X = 0.0f, .Y = 0.0f, .Z = 0.0f }

typedef struct
{
    bool PreferVulkan;
    bool PreferFullScreen;
    bool GpuDebug;
} AppSettings;

typedef enum
{
    InputActionBindingType_Value,
    InputActionBindingType_Released,
    InputActionBindingType_ReleasedSwitch,
    InputActionBindingType_DoubleReleasedSwitch,
} InputActionBindingType;

typedef struct
{
    ElemInputId InputId;
    InputActionBindingType BindingType;
    uint32_t Index;
    float* ActionValue;
    uint32_t ReleasedCount;
    double LastReleasedTime;
} InputActionBinding;

typedef struct
{
    InputActionBinding Items[255];
    uint32_t Length;
} InputActionBindingSpan;

typedef struct
{
    float RotateLeft;
    float RotateRight;
    float RotateUp;
    float RotateDown;
    float RotateSideLeft;
    float RotateSideRight;
    float ZoomIn;
    float ZoomOut;

    float Touch;
    float TouchReleased;
    float TouchRotateLeft;
    float TouchRotateRight;
    float TouchRotateUp;
    float TouchRotateDown;
    float TouchPositionX;
    float TouchPositionY;

    float Touch2;
    float Touch2PositionX;
    float Touch2PositionY;

    float TouchRotateSide;
    float ShowMeshlets;
    float ShowCursor;
    float ExitApp;
} InputActions;

typedef struct
{
    ElemVector3 RotationDelta;
    SampleVector2 RotationTouch;
    ElemVector3 CurrentRotationSpeed;
    float PreviousTouchDistance;
    float PreviousTouchAngle;
    float Zoom;
} GameState;

typedef struct
{
    char FileId[4];
    uint32_t Version;
    uint32_t MeshBufferSizeInBytes;
    uint32_t VertexSizeInBytes;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t MeshletCount;
} MeshFileHeader;

typedef struct
{
    uint32_t MeshBuffer;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t Reserved1;
    uint32_t Reserved2;
    uint32_t Reserved3;
    SampleVector4 RotationQuaternion;
    float Zoom;
    float AspectRatio;
    uint32_t ShowMeshlets;
    uint32_t MeshletCount;
} ShaderParameters;

typedef struct
{
    double FrameTimeInSeconds;
    uint32_t Fps;
    bool HasNewData;
} FrameMeasurement;

typedef struct
{
    AppSettings AppSettings;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemGraphicsHeap MeshBufferHeap;
    ElemGraphicsResource MeshBuffer;
    ElemGraphicsResourceDescriptor MeshBufferReadDescriptor;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    InputActions InputActions;
    InputActionBindingSpan InputActionBindings;
    GameState GameState;
} ApplicationPayload;

uint64_t GlobalTimerFrequency;
uint64_t GlobalTimerBaseCounter;
double GlobalFrameCpuAverage;
double GlobalFpsTimerStart;
uint32_t GlobalCurrentFpsCounter;
uint32_t GlobalFpsCounter;
double GlobalStartTime;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

uint64_t MegaBytesToBytes(uint64_t value)
{
    return value * 1024 * 1024;
}

void GetFullPath(char* destination, const char* path, bool prefixData)
{
    memset(destination, 0, MAX_PATH);

    ElemSystemInfo systemInfo = ElemGetSystemInfo();
    strncpy(destination, systemInfo.ApplicationPath, strlen(systemInfo.ApplicationPath));
    char* pointer = destination + strlen(systemInfo.ApplicationPath);

    const char* folderPrefix = "./";

    if (prefixData)
    {
        if (systemInfo.Platform == ElemPlatform_MacOS)
        {
            folderPrefix = "../Resources/";
        }
        else if (systemInfo.Platform == ElemPlatform_iOS)
        {
            folderPrefix = "./";
        }
        else
        {
            folderPrefix = "Data/";
        }
    }

    strncpy(pointer, folderPrefix, strlen(folderPrefix));
    pointer += strlen(folderPrefix);
    strncpy(pointer, path, strlen(path));
}

ElemDataSpan ReadSampleFile(const char* filename, bool prefixData)
{
    char absolutePath[MAX_PATH];
    GetFullPath(absolutePath, filename, prefixData);

    FILE* file = fopen(absolutePath, "rb");

    if (!file || fseek(file, 0, SEEK_END) != 0)
    {
        if (file)
        {
            fclose(file);
        }

        return (ElemDataSpan) {};
    }

    long fileSize = ftell(file);

    if (fileSize < 0)
    {
        fclose(file);
        return (ElemDataSpan) {};
    }

    rewind(file);

    uint8_t* buffer = (uint8_t*)malloc((size_t)fileSize + 1);

    if (!buffer)
    {
        fclose(file);
        return (ElemDataSpan) {};
    }

    memset(buffer, 0, (size_t)fileSize + 1);
    size_t bytesRead = fread(buffer, 1, (size_t)fileSize, file);
    fclose(file);

    if (bytesRead != (size_t)fileSize)
    {
        free(buffer);
        return (ElemDataSpan) {};
    }

    return (ElemDataSpan)
    {
        .Items = buffer,
        .Length = bytesRead
    };
}

const char* GetPlatformLabel(ElemPlatform platform)
{
    switch (platform)
    {
        case ElemPlatform_Windows:
            return "Windows";
        case ElemPlatform_MacOS:
            return "MacOS";
        case ElemPlatform_iOS:
            return "iOS";
        case ElemPlatform_Linux:
            return "Linux";
    }

    return "Unknown";
}

const char* GetGraphicsApiLabel(ElemGraphicsApi graphicsApi)
{
    switch (graphicsApi)
    {
        case ElemGraphicsApi_DirectX12:
            return "DirectX12";
        case ElemGraphicsApi_Vulkan:
            return "Vulkan";
        case ElemGraphicsApi_Metal:
            return "Metal";
    }

    return "Unknown";
}

void FormatMemorySize(uint64_t bytes, char* outputBuffer, size_t bufferSize)
{
    const char* suffixes[] = { "B", "KB", "MB", "GB", "TB" };
    double size = bytes;
    size_t suffixIndex = 0;

    while (size >= 1024.0 && suffixIndex < sizeof(suffixes) / sizeof(suffixes[0]) - 1)
    {
        size /= 1024.0;
        suffixIndex++;
    }

    snprintf(outputBuffer, bufferSize, "%.2f %s", size, suffixes[suffixIndex]);
}

void SetWindowTitle(ElemWindow window, const char* applicationName, ElemGraphicsDevice graphicsDevice, double frameTimeInSeconds, uint32_t fps)
{
    ElemWindowSize renderSize = ElemGetWindowRenderSize(window);
    ElemSystemInfo systemInfo = ElemGetSystemInfo();
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);

    char memoryFormatted[64];
    FormatMemorySize(graphicsDeviceInfo.AvailableMemory, memoryFormatted, sizeof(memoryFormatted));

    char titleFormatted[256];
    snprintf(
        titleFormatted,
        sizeof(titleFormatted),
        "%s FPS: %u / Cpu FrameTime: %.2f (Elemental=%s, RenderSize=%ux%u@%.1f, GraphicsDevice=%s, GraphicsApi=%s, Platform=%s, AvailableMemory=%s)",
        applicationName,
        fps,
        frameTimeInSeconds * 1000.0,
        ELEM_VERSION_LABEL,
        renderSize.Width,
        renderSize.Height,
        renderSize.UIScale,
        graphicsDeviceInfo.DeviceName,
        GetGraphicsApiLabel(graphicsDeviceInfo.GraphicsApi),
        GetPlatformLabel(systemInfo.Platform),
        memoryFormatted);

    ElemSetWindowTitle(window, titleFormatted);
}

void InitTimer(void)
{
#ifdef _WIN32
    QueryPerformanceFrequency((LARGE_INTEGER*)&GlobalTimerFrequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&GlobalTimerBaseCounter);
#else
    struct timespec timeValue;
    clock_gettime(CLOCK_MONOTONIC, &timeValue);
    GlobalTimerBaseCounter = (uint64_t)timeValue.tv_sec * 1000000000 + (uint64_t)timeValue.tv_nsec;
    GlobalTimerFrequency = 1000000;
#endif
}

double GetTimerValueInMilliseconds(void)
{
#ifdef _WIN32
    uint64_t value;
    QueryPerformanceCounter((LARGE_INTEGER*)&value);
    return ((double)(value - GlobalTimerBaseCounter) / GlobalTimerFrequency) * 1000.0;
#else
    struct timespec timeValue;
    clock_gettime(CLOCK_MONOTONIC, &timeValue);
    uint64_t value = (uint64_t)timeValue.tv_sec * 1000000000 + (uint64_t)timeValue.tv_nsec;
    return (double)(value - GlobalTimerBaseCounter) / GlobalTimerFrequency;
#endif
}

void StartFrameMeasurement(void)
{
    if (GlobalTimerFrequency == 0)
    {
        InitTimer();
    }

    GlobalStartTime = GetTimerValueInMilliseconds();
}

FrameMeasurement EndFrameMeasurement(void)
{
    bool hasNewData = GlobalFpsCounter == 0;
    GlobalFpsCounter++;

    double endTime = GetTimerValueInMilliseconds();
    GlobalFrameCpuAverage = GlobalFrameCpuAverage * 0.95 + (endTime - GlobalStartTime) * 0.05;

    if (endTime - GlobalFpsTimerStart >= 1000.0)
    {
        GlobalCurrentFpsCounter = GlobalFpsCounter - 1;
        GlobalFpsCounter = 1;
        GlobalFpsTimerStart = endTime;
        hasNewData = true;
    }

    return (FrameMeasurement)
    {
        .FrameTimeInSeconds = GlobalFrameCpuAverage / 1000.0,
        .Fps = GlobalCurrentFpsCounter,
        .HasNewData = hasNewData
    };
}

AppSettings ParseAppSettings(int argc, const char* argv[])
{
    AppSettings result = {};

    for (int32_t i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--vulkan") == 0)
        {
            result.PreferVulkan = true;
        }
        else if (strcmp(argv[i], "--fullscreen") == 0)
        {
            result.PreferFullScreen = true;
        }
        else if (strcmp(argv[i], "--gpu-debug") == 0)
        {
            result.GpuDebug = true;
        }
    }

    return result;
}

float Pow2(float value)
{
    return value * value;
}

float NormalizeAngle(float angle)
{
    angle = fmodf(angle + (float)M_PI, 2.0f * (float)M_PI);

    if (angle < 0.0f)
    {
        angle += 2.0f * (float)M_PI;
    }

    return angle - (float)M_PI;
}

SampleVector2 InverseV2(SampleVector2 value)
{
    return (SampleVector2) { -value.X, -value.Y };
}

SampleVector2 AddV2(SampleVector2 value1, SampleVector2 value2)
{
    return (SampleVector2) { value1.X + value2.X, value1.Y + value2.Y };
}

SampleVector2 SubtractV2(SampleVector2 value1, SampleVector2 value2)
{
    return (SampleVector2) { value1.X - value2.X, value1.Y - value2.Y };
}

SampleVector2 MulScalarV2(SampleVector2 value, float scalar)
{
    return (SampleVector2) { value.X * scalar, value.Y * scalar };
}

float MagnitudeSquaredV2(SampleVector2 value)
{
    return value.X * value.X + value.Y * value.Y;
}

float MagnitudeV2(SampleVector2 value)
{
    return sqrtf(MagnitudeSquaredV2(value));
}

SampleVector2 NormalizeV2(SampleVector2 value)
{
    float magnitude = MagnitudeV2(value);
    return magnitude > 0.0f ? MulScalarV2(value, 1.0f / magnitude) : value;
}

ElemVector3 InverseV3(ElemVector3 value)
{
    return (ElemVector3) { -value.X, -value.Y, -value.Z };
}

ElemVector3 AddV3(ElemVector3 value1, ElemVector3 value2)
{
    return (ElemVector3) { value1.X + value2.X, value1.Y + value2.Y, value1.Z + value2.Z };
}

ElemVector3 MulScalarV3(ElemVector3 value, float scalar)
{
    return (ElemVector3) { value.X * scalar, value.Y * scalar, value.Z * scalar };
}

float MagnitudeSquaredV3(ElemVector3 value)
{
    return value.X * value.X + value.Y * value.Y + value.Z * value.Z;
}

float MagnitudeV3(ElemVector3 value)
{
    return sqrtf(MagnitudeSquaredV3(value));
}

ElemVector3 NormalizeV3(ElemVector3 value)
{
    float magnitude = MagnitudeV3(value);
    return magnitude > 0.0f ? MulScalarV3(value, 1.0f / magnitude) : value;
}

ElemVector3 CrossProductV3(ElemVector3 value1, ElemVector3 value2)
{
    return (ElemVector3)
    {
        .X = value1.Y * value2.Z - value1.Z * value2.Y,
        .Y = value1.Z * value2.X - value1.X * value2.Z,
        .Z = value1.X * value2.Y - value1.Y * value2.X
    };
}

SampleVector4 CreateQuaternion(ElemVector3 axis, float angle)
{
    return (SampleVector4)
    {
        .X = axis.X * sinf(angle * 0.5f),
        .Y = axis.Y * sinf(angle * 0.5f),
        .Z = axis.Z * sinf(angle * 0.5f),
        .W = cosf(angle * 0.5f)
    };
}

float DotProductQuaternion(SampleVector4 value1, SampleVector4 value2)
{
    return value1.X * value2.X + value1.Y * value2.Y + value1.Z * value2.Z;
}

SampleVector4 MulQuaternion(SampleVector4 value1, SampleVector4 value2)
{
    ElemVector3 crossProduct = CrossProductV3(value1.XYZ, value2.XYZ);

    return (SampleVector4)
    {
        .X = value2.X * value1.W + value1.X * value2.W + crossProduct.X,
        .Y = value2.Y * value1.W + value1.Y * value2.W + crossProduct.Y,
        .Z = value2.Z * value1.W + value1.Z * value2.W + crossProduct.Z,
        .W = value1.W * value2.W - DotProductQuaternion(value1, value2)
    };
}

void RegisterInputActionBinding(InputActionBindingSpan* bindings, ElemInputId inputId, uint32_t index, InputActionBindingType bindingType, float* actionValue)
{
    bindings->Items[bindings->Length++] = (InputActionBinding)
    {
        .InputId = inputId,
        .BindingType = bindingType,
        .Index = index,
        .ActionValue = actionValue
    };
}

void RegisterInputBindings(ApplicationPayload* applicationPayload)
{
    InputActionBindingSpan* bindings = &applicationPayload->InputActionBindings;
    InputActions* actions = &applicationPayload->InputActions;

    RegisterInputActionBinding(bindings, ElemInputId_KeyA, 0, InputActionBindingType_Value, &actions->RotateLeft);
    RegisterInputActionBinding(bindings, ElemInputId_KeyD, 0, InputActionBindingType_Value, &actions->RotateRight);
    RegisterInputActionBinding(bindings, ElemInputId_KeyW, 0, InputActionBindingType_Value, &actions->RotateUp);
    RegisterInputActionBinding(bindings, ElemInputId_KeyS, 0, InputActionBindingType_Value, &actions->RotateDown);
    RegisterInputActionBinding(bindings, ElemInputId_KeyQ, 0, InputActionBindingType_Value, &actions->RotateSideLeft);
    RegisterInputActionBinding(bindings, ElemInputId_KeyE, 0, InputActionBindingType_Value, &actions->RotateSideRight);
    RegisterInputActionBinding(bindings, ElemInputId_KeyZ, 0, InputActionBindingType_Value, &actions->ZoomIn);
    RegisterInputActionBinding(bindings, ElemInputId_KeyX, 0, InputActionBindingType_Value, &actions->ZoomOut);
    RegisterInputActionBinding(bindings, ElemInputId_KeySpacebar, 0, InputActionBindingType_Value, &actions->ShowMeshlets);
    RegisterInputActionBinding(bindings, ElemInputId_KeyF1, 0, InputActionBindingType_ReleasedSwitch, &actions->ShowCursor);
    RegisterInputActionBinding(bindings, ElemInputId_KeyEscape, 0, InputActionBindingType_Released, &actions->ExitApp);

    RegisterInputActionBinding(bindings, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Value, &actions->Touch);
    RegisterInputActionBinding(bindings, ElemInputId_MouseLeftButton, 0, InputActionBindingType_DoubleReleasedSwitch, &actions->ShowMeshlets);
    RegisterInputActionBinding(bindings, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Released, &actions->TouchReleased);
    RegisterInputActionBinding(bindings, ElemInputId_MouseRightButton, 0, InputActionBindingType_Value, &actions->TouchRotateSide);
    RegisterInputActionBinding(bindings, ElemInputId_MouseAxisXNegative, 0, InputActionBindingType_Value, &actions->TouchRotateLeft);
    RegisterInputActionBinding(bindings, ElemInputId_MouseAxisXPositive, 0, InputActionBindingType_Value, &actions->TouchRotateRight);
    RegisterInputActionBinding(bindings, ElemInputId_MouseAxisYNegative, 0, InputActionBindingType_Value, &actions->TouchRotateUp);
    RegisterInputActionBinding(bindings, ElemInputId_MouseAxisYPositive, 0, InputActionBindingType_Value, &actions->TouchRotateDown);
    RegisterInputActionBinding(bindings, ElemInputId_MouseWheelPositive, 0, InputActionBindingType_Value, &actions->ZoomIn);
    RegisterInputActionBinding(bindings, ElemInputId_MouseWheelNegative, 0, InputActionBindingType_Value, &actions->ZoomOut);
    RegisterInputActionBinding(bindings, ElemInputId_MouseMiddleButton, 0, InputActionBindingType_Value, &actions->ShowMeshlets);

    RegisterInputActionBinding(bindings, ElemInputId_GamepadLeftStickXNegative, 0, InputActionBindingType_Value, &actions->RotateLeft);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadLeftStickXPositive, 0, InputActionBindingType_Value, &actions->RotateRight);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadLeftStickYPositive, 0, InputActionBindingType_Value, &actions->RotateUp);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadLeftStickYNegative, 0, InputActionBindingType_Value, &actions->RotateDown);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadLeftStickButton, 0, InputActionBindingType_Value, &actions->ShowMeshlets);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadLeftTrigger, 0, InputActionBindingType_Value, &actions->RotateSideLeft);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadRightTrigger, 0, InputActionBindingType_Value, &actions->RotateSideRight);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadLeftShoulder, 0, InputActionBindingType_Value, &actions->ZoomOut);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadRightShoulder, 0, InputActionBindingType_Value, &actions->ZoomIn);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadDpadDown, 0, InputActionBindingType_Value, &actions->ZoomOut);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadDpadUp, 0, InputActionBindingType_Value, &actions->ZoomIn);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadButtonA, 0, InputActionBindingType_Value, &actions->ShowMeshlets);
    RegisterInputActionBinding(bindings, ElemInputId_GamepadButtonB, 0, InputActionBindingType_Released, &actions->ExitApp);

    RegisterInputActionBinding(bindings, ElemInputId_Touch, 0, InputActionBindingType_Value, &actions->Touch);
    RegisterInputActionBinding(bindings, ElemInputId_TouchXNegative, 0, InputActionBindingType_Value, &actions->TouchRotateLeft);
    RegisterInputActionBinding(bindings, ElemInputId_TouchXPositive, 0, InputActionBindingType_Value, &actions->TouchRotateRight);
    RegisterInputActionBinding(bindings, ElemInputId_TouchYNegative, 0, InputActionBindingType_Value, &actions->TouchRotateUp);
    RegisterInputActionBinding(bindings, ElemInputId_TouchYPositive, 0, InputActionBindingType_Value, &actions->TouchRotateDown);
    RegisterInputActionBinding(bindings, ElemInputId_TouchXAbsolutePosition, 0, InputActionBindingType_Value, &actions->TouchPositionX);
    RegisterInputActionBinding(bindings, ElemInputId_TouchYAbsolutePosition, 0, InputActionBindingType_Value, &actions->TouchPositionY);
    RegisterInputActionBinding(bindings, ElemInputId_Touch, 1, InputActionBindingType_Value, &actions->Touch2);
    RegisterInputActionBinding(bindings, ElemInputId_TouchXAbsolutePosition, 1, InputActionBindingType_Value, &actions->Touch2PositionX);
    RegisterInputActionBinding(bindings, ElemInputId_TouchYAbsolutePosition, 1, InputActionBindingType_Value, &actions->Touch2PositionY);
    RegisterInputActionBinding(bindings, ElemInputId_Touch, 0, InputActionBindingType_Released, &actions->TouchReleased);
    RegisterInputActionBinding(bindings, ElemInputId_Touch, 0, InputActionBindingType_DoubleReleasedSwitch, &actions->ShowMeshlets);
}

void UpdateInputActions(InputActionBindingSpan* inputActionBindings)
{
    ElemInputStream inputStream = ElemGetInputStream();

    for (uint32_t i = 0; i < inputActionBindings->Length; i++)
    {
        InputActionBinding* binding = &inputActionBindings->Items[i];

        if (binding->BindingType == InputActionBindingType_Released)
        {
            *binding->ActionValue = 0.0f;
        }
    }

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        for (uint32_t j = 0; j < inputActionBindings->Length; j++)
        {
            InputActionBinding* binding = &inputActionBindings->Items[j];

            if (inputEvent->InputId != binding->InputId || inputEvent->InputDeviceTypeIndex != binding->Index)
            {
                continue;
            }

            if (binding->BindingType == InputActionBindingType_Value)
            {
                *binding->ActionValue = inputEvent->Value;
            }
            else if (binding->BindingType == InputActionBindingType_Released)
            {
                *binding->ActionValue = !inputEvent->Value;
            }
            else if (binding->BindingType == InputActionBindingType_ReleasedSwitch && inputEvent->Value == 0.0f)
            {
                *binding->ActionValue = !*binding->ActionValue;
            }
            else if (binding->BindingType == InputActionBindingType_DoubleReleasedSwitch && inputEvent->Value == 0.0f)
            {
                if (inputEvent->ElapsedSeconds - binding->LastReleasedTime > 0.25f)
                {
                    binding->ReleasedCount = 1;
                }
                else
                {
                    binding->ReleasedCount++;

                    if (binding->ReleasedCount > 1)
                    {
                        *binding->ActionValue = !*binding->ActionValue;
                        binding->ReleasedCount = 0;
                    }
                }

                binding->LastReleasedTime = inputEvent->ElapsedSeconds;
            }
        }
    }
}

void ResetTouchParameters(GameState* gameState)
{
    gameState->RotationTouch = V2_ZERO;
    gameState->PreviousTouchDistance = 0.0f;
    gameState->PreviousTouchAngle = 0.0f;
}

void UpdateGameState(GameState* gameState, InputActions* inputActions, float deltaTimeInSeconds)
{
    gameState->RotationDelta = V3_ZERO;

    if (inputActions->Touch)
    {
        if (inputActions->Touch2)
        {
            SampleVector2 touchPosition = { inputActions->TouchPositionX, inputActions->TouchPositionY };
            SampleVector2 touchPosition2 = { inputActions->Touch2PositionX, inputActions->Touch2PositionY };
            SampleVector2 difference = SubtractV2(touchPosition, touchPosition2);
            float distance = MagnitudeV2(difference);
            float angle = atan2f(difference.X, difference.Y);

            if (gameState->PreviousTouchDistance != 0.0f)
            {
                gameState->Zoom += (distance - gameState->PreviousTouchDistance) * ZOOM_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            if (gameState->PreviousTouchAngle != 0.0f)
            {
                gameState->RotationDelta.Z = -NormalizeAngle(angle - gameState->PreviousTouchAngle) * ROTATION_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            gameState->PreviousTouchDistance = distance;
            gameState->PreviousTouchAngle = angle;
        }
        else
        {
            ResetTouchParameters(gameState);
            gameState->RotationDelta.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
            gameState->RotationDelta.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        }
    }
    else if (inputActions->TouchRotateSide)
    {
        ResetTouchParameters(gameState);
        gameState->RotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else if (inputActions->TouchReleased && !inputActions->Touch2)
    {
        ResetTouchParameters(gameState);
        gameState->RotationTouch.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        gameState->RotationTouch.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else
    {
        ElemVector3 direction = NormalizeV3((ElemVector3)
        {
            .X = inputActions->RotateUp - inputActions->RotateDown,
            .Y = inputActions->RotateLeft - inputActions->RotateRight,
            .Z = inputActions->RotateSideLeft - inputActions->RotateSideRight
        });

        if (MagnitudeSquaredV3(direction))
        {
            ElemVector3 acceleration = AddV3(
                MulScalarV3(direction, ROTATION_ACCELERATION),
                MulScalarV3(InverseV3(gameState->CurrentRotationSpeed), ROTATION_FRICTION));

            ResetTouchParameters(gameState);
            gameState->RotationDelta = AddV3(
                MulScalarV3(acceleration, 0.5f * Pow2(deltaTimeInSeconds)),
                MulScalarV3(gameState->CurrentRotationSpeed, deltaTimeInSeconds));
            gameState->CurrentRotationSpeed = AddV3(
                MulScalarV3(acceleration, deltaTimeInSeconds),
                gameState->CurrentRotationSpeed);
        }
    }

    if (MagnitudeSquaredV2(gameState->RotationTouch) > 0.0f)
    {
        if (MagnitudeV2(gameState->RotationTouch) > ROTATION_TOUCH_MAX_DELTA)
        {
            gameState->RotationTouch = MulScalarV2(NormalizeV2(gameState->RotationTouch), ROTATION_TOUCH_MAX_DELTA);
        }

        gameState->RotationDelta = AddV3(
            gameState->RotationDelta,
            (ElemVector3) { gameState->RotationTouch.X, gameState->RotationTouch.Y, 0.0f });

        SampleVector2 inverse = MulScalarV2(NormalizeV2(InverseV2(gameState->RotationTouch)), ROTATION_TOUCH_DECREASE_SPEED);
        gameState->RotationTouch = AddV2(gameState->RotationTouch, inverse);

        if (MagnitudeV2(gameState->RotationTouch) < 0.001f)
        {
            ResetTouchParameters(gameState);
        }
    }

    gameState->Zoom += (inputActions->ZoomIn - inputActions->ZoomOut) * ZOOM_SPEED * deltaTimeInSeconds;
}

bool LoadMesh(ApplicationPayload* applicationPayload, const char* path)
{
    ElemDataSpan meshFileData = ReadSampleFile(path, true);

    if (!meshFileData.Items || meshFileData.Length < sizeof(MeshFileHeader))
    {
        printf("Unable to read mesh: %s\n", path);
        return false;
    }

    MeshFileHeader* header = (MeshFileHeader*)meshFileData.Items;
    uint32_t meshPayloadSizeInBytes = meshFileData.Length - sizeof(MeshFileHeader);

    bool validHeader =
        memcmp(header->FileId, "MESH", 4) == 0 &&
        header->Version == MESH_FILE_VERSION &&
        header->MeshBufferSizeInBytes == meshPayloadSizeInBytes &&
        header->VertexSizeInBytes == MESH_VERTEX_SIZE_IN_BYTES &&
        header->VertexBufferOffset == 0 &&
        header->MeshletOffset <= header->MeshletVertexIndexOffset &&
        header->MeshletVertexIndexOffset <= header->MeshletTriangleIndexOffset &&
        header->MeshletTriangleIndexOffset <= header->MeshBufferSizeInBytes &&
        header->MeshletCount > 0 &&
        header->MeshletOffset + header->MeshletCount * MESHLET_SIZE_IN_BYTES <= header->MeshletVertexIndexOffset;

    if (!validHeader)
    {
        printf("Invalid or unsupported mesh file: %s\n", path);
        free(meshFileData.Items);
        return false;
    }

    ElemGraphicsResourceInfo meshBufferInfo = ElemCreateGraphicsBufferResourceInfo(
        applicationPayload->GraphicsDevice,
        header->MeshBufferSizeInBytes,
        ElemGraphicsResourceUsage_Read,
        &(ElemGraphicsResourceInfoOptions) { .DebugName = "MeshBuffer" });

    applicationPayload->MeshBufferHeap = ElemCreateGraphicsHeap(
        applicationPayload->GraphicsDevice,
        meshBufferInfo.SizeInBytes,
        &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_GpuUpload });

    applicationPayload->MeshBuffer = ElemCreateGraphicsResource(applicationPayload->MeshBufferHeap, 0, &meshBufferInfo);
    applicationPayload->MeshBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(
        applicationPayload->MeshBuffer,
        ElemGraphicsResourceDescriptorUsage_Read,
        NULL);

    ElemUploadGraphicsBufferData(
        applicationPayload->MeshBuffer,
        0,
        (ElemDataSpan)
        {
            .Items = meshFileData.Items + sizeof(MeshFileHeader),
            .Length = header->MeshBufferSizeInBytes
        });

    applicationPayload->ShaderParameters.MeshBuffer = applicationPayload->MeshBufferReadDescriptor;
    applicationPayload->ShaderParameters.VertexBufferOffset = header->VertexBufferOffset;
    applicationPayload->ShaderParameters.MeshletOffset = header->MeshletOffset;
    applicationPayload->ShaderParameters.MeshletVertexIndexOffset = header->MeshletVertexIndexOffset;
    applicationPayload->ShaderParameters.MeshletTriangleIndexOffset = header->MeshletTriangleIndexOffset;
    applicationPayload->ShaderParameters.MeshletCount = header->MeshletCount;

    free(meshFileData.Items);
    return true;
}

void CreateDepthBuffer(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->DepthBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    }

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(
        applicationPayload->GraphicsDevice,
        width,
        height,
        1,
        ElemGraphicsFormat_D32_FLOAT,
        ElemGraphicsResourceUsage_DepthStencil,
        &(ElemGraphicsResourceInfoOptions) { .DebugName = "DepthBuffer" });

    applicationPayload->DepthBuffer = ElemCreateGraphicsResource(applicationPayload->DepthBufferHeap, 0, &resourceInfo);
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions)
    {
        .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal
    });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions)
    {
        .EnableDebugLayer = applicationPayload->AppSettings.GpuDebug,
        .EnableGpuValidation = false,
        .EnableDebugBarrierInfo = false,
        .PreferVulkan = applicationPayload->AppSettings.PreferVulkan
    });

    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);
    applicationPayload->CommandQueue = ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain = ElemCreateSwapChain(
        applicationPayload->CommandQueue,
        applicationPayload->Window,
        UpdateSwapChain,
        &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });

    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(
        applicationPayload->GraphicsDevice,
        MegaBytesToBytes(64),
        &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);

    if (!LoadMesh(applicationPayload, "kitten.mesh"))
    {
        ElemExitApplication(1);
        return;
    }

    ElemDataSpan shaderData = ReadSampleFile(
        !applicationPayload->AppSettings.PreferVulkan ? "RenderMesh.shader" : "RenderMesh_vulkan.shader",
        true);
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(
        applicationPayload->GraphicsDevice,
        &(ElemGraphicsPipelineStateParameters)
        {
            .DebugName = "RenderMesh PSO",
            .ShaderLibrary = shaderLibrary,
            .MeshShaderFunction = "MeshMain",
            .PixelShaderFunction = "PixelMain",
            .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = swapChainInfo.Format }}, .Length = 1 },
            .DepthStencil =
            {
                .Format = ElemGraphicsFormat_D32_FLOAT,
                .DepthCompareFunction = ElemGraphicsCompareFunction_Greater
            }
        });

    free(shaderData.Items);
    ElemFreeShaderLibrary(shaderLibrary);

    applicationPayload->ShaderParameters.RotationQuaternion = (SampleVector4) { .X = 0, .Y = 0, .Z = 0, .W = 1 };
    applicationPayload->InputActions.ShowCursor = applicationPayload->AppSettings.PreferFullScreen ? 0.0f : 1.0f;
    RegisterInputBindings(applicationPayload);

    if (applicationPayload->AppSettings.PreferFullScreen)
    {
        ElemHideWindowCursor(applicationPayload->Window);
    }

    StartFrameMeasurement();
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    if (applicationPayload->LastExecutionFence.CommandQueue != ELEM_HANDLE_NULL)
    {
        ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);
    }

    if (applicationPayload->GraphicsPipeline != ELEM_HANDLE_NULL)
    {
        ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    }

    if (applicationPayload->MeshBufferReadDescriptor != -1)
    {
        ElemFreeGraphicsResourceDescriptor(applicationPayload->MeshBufferReadDescriptor, NULL);
    }

    if (applicationPayload->MeshBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->MeshBuffer, NULL);
    }

    if (applicationPayload->MeshBufferHeap != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsHeap(applicationPayload->MeshBufferHeap);
    }

    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);

    if (applicationPayload->DepthBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    }

    if (applicationPayload->DepthBufferHeap != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);
    }

    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    if (updateParameters->SizeChanged)
    {
        CreateDepthBuffer(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    UpdateInputActions(&applicationPayload->InputActionBindings);
    InputActions* inputActions = &applicationPayload->InputActions;

    if (inputActions->ExitApp)
    {
        ElemExitApplication(0);
    }

    if (inputActions->ShowCursor)
    {
        ElemShowWindowCursor(applicationPayload->Window);
    }
    else
    {
        ElemHideWindowCursor(applicationPayload->Window);
    }

    GameState* gameState = &applicationPayload->GameState;
    UpdateGameState(gameState, inputActions, updateParameters->DeltaTimeInSeconds);

    if (MagnitudeSquaredV3(gameState->RotationDelta))
    {
        SampleVector4 rotationQuaternion = MulQuaternion(
            CreateQuaternion((ElemVector3) { 1, 0, 0 }, gameState->RotationDelta.X),
            MulQuaternion(
                CreateQuaternion((ElemVector3) { 0, 0, 1 }, gameState->RotationDelta.Z),
                CreateQuaternion((ElemVector3) { 0, 1, 0 }, gameState->RotationDelta.Y)));

        applicationPayload->ShaderParameters.RotationQuaternion = MulQuaternion(
            rotationQuaternion,
            applicationPayload->ShaderParameters.RotationQuaternion);
    }

    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;
    float maxZoom = applicationPayload->ShaderParameters.AspectRatio >= 0.75 ? 1.5f : 3.5f;
    applicationPayload->ShaderParameters.Zoom = fminf(maxZoom, gameState->Zoom);
    applicationPayload->ShaderParameters.ShowMeshlets = inputActions->ShowMeshlets;

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL);

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters)
    {
        .RenderTargets =
        {
            .Items = (ElemRenderPassRenderTarget[])
            {
                {
                    .RenderTarget = updateParameters->BackBufferRenderTarget,
                    .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
                }
            },
            .Length = 1
        },
        .DepthStencil =
        {
            .DepthStencil = applicationPayload->DepthBuffer
        }
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline);
    ElemPushPipelineStateConstants(
        commandList,
        0,
        (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
    ElemDispatchMesh(commandList, applicationPayload->ShaderParameters.MeshletCount, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);
    ElemPresentSwapChain(applicationPayload->SwapChain);

    FrameMeasurement frameMeasurement = EndFrameMeasurement();

    if (frameMeasurement.HasNewData)
    {
        SetWindowTitle(
            applicationPayload->Window,
            "HelloMesh",
            applicationPayload->GraphicsDevice,
            frameMeasurement.FrameTimeInSeconds,
            frameMeasurement.Fps);
    }

    StartFrameMeasurement();
}

int main(int argc, const char* argv[])
{
    ApplicationPayload payload =
    {
        .AppSettings = ParseAppSettings(argc, argv),
        .MeshBufferReadDescriptor = -1
    };

    ElemConfigureLogHandler(ElemConsoleLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello Mesh",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
