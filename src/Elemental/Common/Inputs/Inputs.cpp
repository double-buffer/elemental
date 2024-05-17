#include "Inputs.h"
#include "SystemDataPool.h"
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

SystemDataPool<InputDeviceData, InputDeviceDataFull> inputDevicePool;

void InitInputsMemory()
{
    if (inputsMemoryArena.Storage == nullptr)
    {
        inputsMemoryArena = SystemAllocateMemoryArena();
        inputEvents = SystemPushArray<ElemInputEvent>(inputsMemoryArena, MAX_INPUT_EVENTS * 2);
        deltaInputsToReset = SystemPushArrayZero<ElemInputEvent>(inputsMemoryArena, MAX_INPUT_EVENTS * 2);
        inputDevicePool = SystemCreateDataPool<InputDeviceData, InputDeviceDataFull>(inputsMemoryArena, MAX_INPUT_DEVICES);

        currentInputEventsIndex = 0;
        currentInputEventsWriteIndex = 0;

        currentDeltaInputsToResetIndex = 0;
        currentDeltaInputsToResetWriteIndex = 0;
        previousDeltaInputsToResetWriteIndex = 0;
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

    return handle;
}

void AddInputEvent(ElemInputEvent inputEvent)
{
    InitInputsMemory();

    // TODO: Check boundaries

    if (inputEvent.InputType == ElemInputType_Delta)
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


    // TODO: If event is relative register an automatic reset if event is not there next time

    inputEvents[currentInputEventsIndex * MAX_INPUT_EVENTS + currentInputEventsWriteIndex++] = inputEvent;
}

ElemAPI ElemInputStream ElemGetInputStream(ElemGetInputStreamOptions* options)
{
    InitInputsMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();

    // TODO: Reset previous deltas
    auto previousIndex = (currentDeltaInputsToResetIndex + 1) % 2;
    auto previousDeltaToReset = &deltaInputsToReset[previousIndex * MAX_INPUT_EVENTS];

    for (uint32_t i = 0; i < previousDeltaInputsToResetWriteIndex; i++)
    {
        if (previousDeltaToReset[i].InputId != ElemInputId_Unknown)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Reset Delta Input for %d", previousDeltaToReset[i].InputId);

            auto inputEvent = previousDeltaToReset[i];
            inputEvent.Value = 0.0f;
            inputEvents[currentInputEventsIndex * MAX_INPUT_EVENTS + currentInputEventsWriteIndex++] = inputEvent;
        }
    }

    auto eventSpan = inputEvents.Slice(currentInputEventsIndex * MAX_INPUT_EVENTS, currentInputEventsWriteIndex);

    currentInputEventsIndex = (currentInputEventsIndex + 1) % 2;
    currentInputEventsWriteIndex = 0;

    currentDeltaInputsToResetIndex = (currentDeltaInputsToResetIndex + 1) % 2;
    previousDeltaInputsToResetWriteIndex = currentDeltaInputsToResetWriteIndex;
    currentDeltaInputsToResetWriteIndex = 0; 

    SystemPlatformClearMemory(&deltaInputsToReset[currentDeltaInputsToResetIndex * MAX_INPUT_EVENTS], sizeof(ElemInputId) * MAX_INPUT_EVENTS);

    return 
    {
        .Events =
        {
            .Items = eventSpan.Pointer,
            .Length = (uint32_t)eventSpan.Length
        }
    };
}
