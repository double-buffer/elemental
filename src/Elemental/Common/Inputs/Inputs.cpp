#include "Inputs.h"
#include "SystemDataPool.h"
#include "SystemMemory.h"

MemoryArena inputsMemoryArena;
Span<ElemInputEvent> inputEvents;
uint32_t currentInputEventsIndex;
uint32_t currentInputEventsWriteIndex;

SystemDataPool<InputDeviceData, InputDeviceDataFull> inputDevicePool;

void InitInputsMemory()
{
    if (inputsMemoryArena.Storage == nullptr)
    {
        inputsMemoryArena = SystemAllocateMemoryArena();
        inputEvents = SystemPushArray<ElemInputEvent>(inputsMemoryArena, MAX_INPUT_EVENTS * 2);
        inputDevicePool = SystemCreateDataPool<InputDeviceData, InputDeviceDataFull>(inputsMemoryArena, MAX_INPUT_DEVICES);

        currentInputEventsIndex = 0;
        currentInputEventsWriteIndex = 0;
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

    inputEvents[currentInputEventsIndex * MAX_INPUT_EVENTS + currentInputEventsWriteIndex++] = inputEvent;
}

ElemAPI ElemInputStream ElemGetInputStream(ElemGetInputStreamOptions* options)
{
    InitInputsMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto eventSpan = inputEvents.Slice(currentInputEventsIndex * MAX_INPUT_EVENTS, currentInputEventsWriteIndex);

    currentInputEventsIndex = (currentInputEventsIndex + 1) % 2;
    currentInputEventsWriteIndex = 0;

    // TODO: Filter

    return 
    {
        .Events =
        {
            .Items = eventSpan.Pointer,
            .Length = (uint32_t)eventSpan.Length
        }
    };
}
