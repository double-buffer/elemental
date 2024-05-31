#include "Inputs.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"

MemoryArena inputsMemoryArena;
Span<ElemInputEvent> inputEvents;
uint32_t currentInputEventsIndex;
uint32_t currentInputEventsWriteIndex;

Span<ElemInputEvent> deltaInputsToReset;
uint32_t currentDeltaInputsToResetIndex;
uint32_t currentDeltaInputsToResetWriteIndex;
uint32_t previousDeltaInputsToResetWriteIndex;

Span<ElemInputDevice> inputDevices;
uint32_t currentInputDeviceIndex;

SystemDataPool<InputDeviceData, InputDeviceDataFull> inputDevicePool;

void InitInputsMemory()
{
    if (inputsMemoryArena.Storage == nullptr)
    {
        inputsMemoryArena = SystemAllocateMemoryArena();
        inputEvents = SystemPushArray<ElemInputEvent>(inputsMemoryArena, MAX_INPUT_EVENTS * 2);
        deltaInputsToReset = SystemPushArrayZero<ElemInputEvent>(inputsMemoryArena, MAX_INPUT_EVENTS * 2);

        inputDevices = SystemPushArrayZero<ElemInputDevice>(inputsMemoryArena, MAX_INPUT_DEVICES);
        inputDevicePool = SystemCreateDataPool<InputDeviceData, InputDeviceDataFull>(inputsMemoryArena, MAX_INPUT_DEVICES);

        currentInputEventsIndex = 0;
        currentInputEventsWriteIndex = 0;

        currentDeltaInputsToResetIndex = 0;
        currentDeltaInputsToResetWriteIndex = 0;
        previousDeltaInputsToResetWriteIndex = 0;

        currentInputDeviceIndex = 0;
    }
}

InputDeviceData* GetInputDeviceData(ElemInputDevice inputDevice)
{
    return SystemGetDataPoolItem(inputDevicePool, inputDevice);
}

InputDeviceDataFull* GetInputDeviceDataFull(ElemInputDevice inputDevice)
{
    return SystemGetDataPoolItemFull(inputDevicePool, inputDevice);
}

ElemInputDevice AddInputDevice(InputDeviceData* deviceData, InputDeviceDataFull* deviceDataFull)
{
    InitInputsMemory();

    auto handle = SystemAddDataPoolItem(inputDevicePool, *deviceData);
    SystemAddDataPoolItemFull(inputDevicePool, handle, *deviceDataFull);

    inputDevices[currentInputDeviceIndex++] = handle;

    return handle;
}

void RemoveInputDevice(ElemInputDevice inputDevice)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Device Removed %d", inputDevice);

    SystemRemoveDataPoolItem(inputDevicePool, inputDevice);

    for (uint32_t i = 0; i < MAX_INPUT_DEVICES; i++)
    {
        if (inputDevices[i] == inputDevice)
        {
            inputDevices[i] = ELEM_HANDLE_NULL;
            break;
        }
    }
}

void AddInputEvent(ElemInputEvent inputEvent, bool needReset)
{
    InitInputsMemory();

    // TODO: Check boundaries

    if (inputEvent.InputType == ElemInputType_Delta || needReset)
    {
        auto previousIndex = (currentDeltaInputsToResetIndex + 1) % 2;
        auto previousDeltaToReset = &deltaInputsToReset[previousIndex * MAX_INPUT_EVENTS];

        for (uint32_t i = 0; i < previousDeltaInputsToResetWriteIndex; i++)
        {
            if (previousDeltaToReset[i].InputId == inputEvent.InputId)
            {
                previousDeltaToReset[i].InputId = ElemInputId_Unknown;
                break;
            }
        }

        deltaInputsToReset[currentDeltaInputsToResetIndex * MAX_INPUT_EVENTS + currentDeltaInputsToResetWriteIndex++] = inputEvent;
    }

    // TODO: Thread safe !
    inputEvents[currentInputEventsIndex * MAX_INPUT_EVENTS + currentInputEventsWriteIndex++] = inputEvent;
}

ElemAPI ElemInputDeviceInfo ElemGetInputDeviceInfo(ElemInputDevice inputDevice)
{
    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData);

    auto inputDeviceDataFull = GetInputDeviceDataFull(inputDevice);
    SystemAssert(inputDeviceDataFull);

    return
    {
        .Handle = inputDevice,
        .DeviceType = inputDeviceData->InputDeviceType,
    };
}

void ResetInputsFrame()
{
    InitInputsMemory();

    currentInputEventsIndex = (currentInputEventsIndex + 1) % 2;
    currentInputEventsWriteIndex = 0;

    currentDeltaInputsToResetIndex = (currentDeltaInputsToResetIndex + 1) % 2;
    previousDeltaInputsToResetWriteIndex = currentDeltaInputsToResetWriteIndex;
    currentDeltaInputsToResetWriteIndex = 0; 

    SystemPlatformClearMemory(&deltaInputsToReset[currentDeltaInputsToResetIndex * MAX_INPUT_EVENTS], sizeof(ElemInputEvent) * MAX_INPUT_EVENTS);
    auto previousIndex = (currentDeltaInputsToResetIndex + 1) % 2;
    auto previousDeltaToReset = &deltaInputsToReset[previousIndex * MAX_INPUT_EVENTS];

    for (uint32_t i = 0; i < previousDeltaInputsToResetWriteIndex; i++)
    {
        if (previousDeltaToReset[i].InputId != ElemInputId_Unknown)
        {
            auto inputEvent = previousDeltaToReset[i];
            inputEvent.Value = 0.0f;
            inputEvents[currentInputEventsIndex * MAX_INPUT_EVENTS + currentInputEventsWriteIndex++] = inputEvent;
        }
    }
}

ElemAPI ElemInputStream ElemGetInputStream()
{
    InitInputsMemory();

    auto eventSpan = inputEvents.Slice(currentInputEventsIndex * MAX_INPUT_EVENTS, currentInputEventsWriteIndex);

    return 
    {
        .Events =
        {
            .Items = eventSpan.Pointer,
            .Length = (uint32_t)eventSpan.Length
        }
    };
}
