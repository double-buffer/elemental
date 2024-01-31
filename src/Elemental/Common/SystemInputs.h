#pragma once

#include "SystemLogging.h"
#include "Elemental.h"
#include "SystemFunctions.h"

#define MAX_QUEUE_INPUTSVALUES 255


// TODO: Put that in an input folder like vulkan




struct NativeInputsQueue
{
    InputsValue InputsValues[255];
    uint32_t ReadIndex;
    uint32_t WriteIndex;
    uint32_t Count;
};

NativeInputsQueue* CreateNativeInputsQueue();
void FreeNativeInputsQueue(NativeInputsQueue* nativeInputsQueue);
void AddNativeInputsQueueItem(NativeInputsQueue* nativeInputsQueue, InputsValue inputsValue);

typedef void (*ConvertHidInputDeviceDataFuncPtr)(InputState* inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes);
ConvertHidInputDeviceDataFuncPtr GetConvertHidInputDeviceDataFuncPtr(uint32_t vendorId, uint32_t productId);





// TODO: To remove
void CreateInputObject(InputObjectKey inputObjectKey, InputObjectType inputObjectType);
void InitGamepad(int32_t gamePadIndex);
InputState* InitInputState();
void FreeInputState(InputState* inputState);
