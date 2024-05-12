#include "Inputs.h"
#include "SystemMemory.h"

#define MAX_INPUT_EVENTS 1024u

MemoryArena inputsMemoryArena;
Span<ElemInputEvent> inputEvents;
uint32_t currentInputEventsIndex;
uint32_t currentInputEventsWriteIndex;

void InitInputsMemory()
{
    if (inputsMemoryArena.Storage == nullptr)
    {
        inputsMemoryArena = SystemAllocateMemoryArena();
        inputEvents = SystemPushArray<ElemInputEvent>(inputsMemoryArena, MAX_INPUT_EVENTS * 2);

        currentInputEventsIndex = 0;
        currentInputEventsWriteIndex = 0;
    }
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
