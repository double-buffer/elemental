#include "Elemental.h"
#include "SampleUtils.h"

// TODO: In the samples, replace every const char* by string spans
// TODO: Get rid of storage in the function parameters and embed the storage pointer in the Value struct?

#define MAX_VALUES 4096

typedef struct
{
    SampleAppSettings AppSettings;
} ApplicationPayload;

typedef uint32_t Value;
struct ValueStorage;

#define VALUE_EMPTY 0

typedef void (*ValueBackwardFunction)(struct ValueStorage* storage, Value operand1, Value operand2, Value result);

typedef struct
{
    float Data;
    float Gradient;
    Value Previous[2];
    uint32_t PreviousCount;
    const char* Label;
    const char* Operation;
    ValueBackwardFunction BackwardFunction;
} ValueData;

typedef struct ValueStorage
{
    ValueData Storage[MAX_VALUES];
    uint32_t Count;
} ValueStorage;

Value CreateValue(ValueStorage* storage, float data, const char* label)
{
    ValueData* valueData = &storage->Storage[storage->Count++];

    *valueData = (ValueData)
    {
        .Data = data, 
        .Label = label 
    };

    return storage->Count;
}

Value CreateValueFromOperation(ValueStorage* storage, float data, const char* label, const char* operation, Value operand1, Value operand2, ValueBackwardFunction backwardFunction)
{
    ValueData* valueData = &storage->Storage[storage->Count++];

    *valueData = (ValueData)
    {
        .Data = data, 
        .Label = label,
        .Operation = operation,
        .BackwardFunction = backwardFunction
    };
    
    if (operand1 != VALUE_EMPTY)
    {
        valueData->Previous[valueData->PreviousCount++] = operand1;
    }

    if (operand2 != VALUE_EMPTY)
    {
        valueData->Previous[valueData->PreviousCount++] = operand2;
    }

    return storage->Count;
}

ValueData* GetValueData(ValueStorage* storage, Value value)
{
    assert(value > 0);
    assert(value <= storage->Count);

    return &storage->Storage[value - 1];
}

void ValueAddBackward(ValueStorage* storage, Value operand1, Value operand2, Value result)
{
    ValueData* operand1Data = GetValueData(storage, operand1);
    ValueData* operand2Data = GetValueData(storage, operand2);
    ValueData* resultData = GetValueData(storage, result);

    operand1Data->Gradient += 1.0f * resultData->Gradient;
    operand2Data->Gradient += 1.0f * resultData->Gradient;
}

Value ValueAdd(ValueStorage* storage, Value a, Value b, const char* label)
{
    ValueData* aData = GetValueData(storage, a);
    ValueData* bData = GetValueData(storage, b);

    float result = aData->Data + bData->Data;
    return CreateValueFromOperation(storage, result, label, "+", a, b, ValueAddBackward);
}

void ValuePrint(ValueStorage* storage, Value value)
{
    ValueData* valueData = GetValueData(storage, value);
    printf("%s = %f (gradient=%f)\n", valueData->Label, valueData->Data, valueData->Gradient);
}

void ValueMulBackward(ValueStorage* storage, Value operand1, Value operand2, Value result)
{
    ValueData* operand1Data = GetValueData(storage, operand1);
    ValueData* operand2Data = GetValueData(storage, operand2);
    ValueData* resultData = GetValueData(storage, result);

    operand1Data->Gradient += operand2Data->Data * resultData->Gradient;
    operand2Data->Gradient += operand1Data->Data * resultData->Gradient;
}

Value ValueMul(ValueStorage* storage, Value a, Value b, const char* label)
{
    ValueData* aData = GetValueData(storage, a);
    ValueData* bData = GetValueData(storage, b);

    float result = aData->Data * bData->Data;
    return CreateValueFromOperation(storage, result, label, "*", a, b, ValueMulBackward);
}

uint32_t colors[] = { 32, 33, 36, 35 }; 

void ChangeConsoleColor(uint32_t color)
{
    printf("\033[%dm", color);
}

void ShowNodeImpl(ValueStorage* storage, Value value, uint32_t level, uint32_t parentColor, const char* prefix, bool isLast)
{
    const char* connector = isLast ? "|_ " : u8"|- ";

    if (level == 0)
    {
        connector = "";
    }

    ChangeConsoleColor(parentColor);
    printf("%s", prefix);

    uint32_t color = colors[level % ARRAYSIZE(colors)];
    ChangeConsoleColor(color);
    
    printf("%s", connector);
    ValuePrint(storage, value);
    
    ValueData* valueData = GetValueData(storage, value);
    
    if (valueData->Operation)
    {
        const char* operationPrefix = isLast ? " " : "|";
        
        char fullPrefix[255];
        memset(fullPrefix, 0, 255);
        strcpy(fullPrefix, prefix);
        strcat(fullPrefix, operationPrefix);

        printf("%s", fullPrefix);

        uint32_t operationColor = colors[(level + 1) % ARRAYSIZE(colors)];
        ChangeConsoleColor(operationColor);
        printf("|- operation: %s\n", valueData->Operation);
    }

    for (uint32_t i = 0; i < valueData->PreviousCount; i++)
    {
        bool childIsLast = (i == valueData->PreviousCount - 1);
        const char* newPrefix = isLast ? " " : "|";

        char fullPrefix[255];
        memset(fullPrefix, 0, 255);
        strcpy(fullPrefix, prefix);
        strcat(fullPrefix, newPrefix);

        ShowNodeImpl(storage, valueData->Previous[i], level + 1, color, fullPrefix, childIsLast);
    }

    if (level == 0)
    {
        ChangeConsoleColor(0);
    }
}

void ShowNode(ValueStorage* storage, Value value)
{
    ShowNodeImpl(storage, value, 0, 32, "", true);
}

void InitSample(void* payload)
{
    ValueStorage storage = {};

    Value a = CreateValue(&storage, 2.0f, "a");
    Value b = CreateValue(&storage, 3.0f, "b");

    Value c = ValueMul(&storage, a, b, "c");
    Value d = CreateValue(&storage, -8.0f, "d");

    Value e = ValueAdd(&storage, c, d, "e");

    ValueData* eData = GetValueData(&storage, e);
    eData->Gradient = 1.0f;
    eData->BackwardFunction(&storage, eData->Previous[0], eData->Previous[1], e);

    ValueData* cData = GetValueData(&storage, c);
    cData->BackwardFunction(&storage, cData->Previous[0], cData->Previous[1], c);

    ShowNode(&storage, e);

    ElemExitApplication(0);
}

void FreeSample(void* payload)
{
}

int main(int argc, const char* argv[]) 
{
    ApplicationPayload payload =
    {
        .AppSettings = SampleParseAppSettings(argc, argv),
    };

    ElemConfigureLogHandler(SampleConsoleAndFileLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "AI Training",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });

}
