#include "Elemental.h"
#include "SampleUtils.h"
#include <math.h>
#include <time.h>

// TODO: In the samples, replace every const char* by string spans

#define MAX_VALUES 4096 * 4

typedef struct
{
    SampleAppSettings AppSettings;
} ApplicationPayload;

typedef uint32_t Value;

#define VALUE_EMPTY 0

typedef void (*ValueBackwardFunction)(Value operand1, Value operand2, Value result);

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

typedef struct
{
    Value* Items;
    uint32_t Count;
} ValueSpan;

typedef struct
{
    // TODO: Replace that with value span
    Value OrderedList[MAX_VALUES];
    uint32_t Count;
    bool VisitedList[MAX_VALUES];
} ValueOrderedListBuilder;

ValueStorage GlobalValueStorage;

void ValueEmptyBackward(Value operand1, Value operand2, Value result)
{
}

Value CreateValue(float data, const char* label)
{
    ValueData* valueData = &GlobalValueStorage.Storage[GlobalValueStorage.Count++];

    *valueData = (ValueData)
    {
        .Data = data, 
        .Label = label,
        .BackwardFunction = ValueEmptyBackward
    };

    return GlobalValueStorage.Count;
}

Value CreateValueFromOperation(float data, const char* label, const char* operation, Value operand1, Value operand2, ValueBackwardFunction backwardFunction)
{
    ValueData* valueData = &GlobalValueStorage.Storage[GlobalValueStorage.Count++];

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

    return GlobalValueStorage.Count;
}

ValueData* GetValueData(Value value)
{
    assert(value > 0);
    assert(value <= GlobalValueStorage.Count);

    return &GlobalValueStorage.Storage[value - 1];
}

void BuildOrderedList(Value node, ValueOrderedListBuilder* builder)
{
    assert(node != VALUE_EMPTY);

    uint32_t nodeIndex = node - 1;
    
    if (!builder->VisitedList[nodeIndex])
    {
        builder->VisitedList[nodeIndex] = true;

        ValueData* nodeData = GetValueData(node);

        for (uint32_t i = 0; i < nodeData->PreviousCount; i++)
        {
            BuildOrderedList(nodeData->Previous[i], builder);
        }

        builder->OrderedList[builder->Count++] = node;
    }
}

void Backward(Value node)
{
    ValueOrderedListBuilder orderedBuilder = {};

    BuildOrderedList(node, &orderedBuilder);

    ValueData* nodeData = GetValueData(node);
    nodeData->Gradient = 1.0f;

    for (int32_t i = orderedBuilder.Count - 1; i >= 0; i--)
    {
        Value subNode = orderedBuilder.OrderedList[i];
        ValueData* subNodeData = GetValueData(subNode);

        subNodeData->BackwardFunction(subNodeData->Previous[0], subNodeData->Previous[1], subNode);
    }
}

void ValuePrint(Value value)
{
    ValueData* valueData = GetValueData(value);
    printf("%s = %f (gradient=%f)\n", valueData->Label, valueData->Data, valueData->Gradient);
}

uint32_t colors[] = { 32, 33, 36, 35 }; 

void ChangeConsoleColor(uint32_t color)
{
    printf("\033[%dm", color);
}

void ShowNodeImpl(Value value, uint32_t level, uint32_t parentColor, const char* prefix, bool isLast)
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
    ValuePrint(value);
    
    ValueData* valueData = GetValueData(value);
    
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

        ShowNodeImpl(valueData->Previous[i], level + 1, color, fullPrefix, childIsLast);
    }

    if (level == 0)
    {
        ChangeConsoleColor(0);
    }
}

void ShowNode(Value value)
{
    ShowNodeImpl(value, 0, 32, "", true);
}

void ValueAddBackward(Value operand1, Value operand2, Value result)
{
    ValueData* operand1Data = GetValueData(operand1);
    ValueData* operand2Data = GetValueData(operand2);
    ValueData* resultData = GetValueData(result);

    operand1Data->Gradient += 1.0f * resultData->Gradient;
    operand2Data->Gradient += 1.0f * resultData->Gradient;
}

Value ValueAdd(Value a, Value b, const char* label)
{
    ValueData* aData = GetValueData(a);
    ValueData* bData = GetValueData(b);

    float result = aData->Data + bData->Data;
    return CreateValueFromOperation(result, label, "+", a, b, ValueAddBackward);
}

void ValueMulBackward(Value operand1, Value operand2, Value result)
{
    ValueData* operand1Data = GetValueData(operand1);
    ValueData* operand2Data = GetValueData(operand2);
    ValueData* resultData = GetValueData(result);

    operand1Data->Gradient += operand2Data->Data * resultData->Gradient;
    operand2Data->Gradient += operand1Data->Data * resultData->Gradient;
}

Value ValueMul(Value a, Value b, const char* label)
{
    ValueData* aData = GetValueData(a);
    ValueData* bData = GetValueData(b);

    float result = aData->Data * bData->Data;
    return CreateValueFromOperation(result, label, "*", a, b, ValueMulBackward);
}

Value ValueNegate(Value a, const char* label)
{
    Value b = CreateValue(-1.0f, "litteral");
    return ValueMul(a, b, label);
}

Value ValueSub(Value a, Value b, const char* label)
{
    return ValueAdd(a, ValueNegate(b, "litteral"), label);
}

void ValuePowBackward(Value operand1, Value operand2, Value result)
{
    ValueData* operand1Data = GetValueData(operand1);
    ValueData* operand2Data = GetValueData(operand2);
    ValueData* resultData = GetValueData(result);

    operand1Data->Gradient += (operand2Data->Data * powf(operand1Data->Data, operand2Data->Data - 1)) * resultData->Gradient;
}

Value ValuePow(Value a, Value b, const char* label)
{
    ValueData* aData = GetValueData(a);
    ValueData* bData = GetValueData(b);

    float result = powf(aData->Data, bData->Data);
    return CreateValueFromOperation(result, label, "pow", a, b, ValuePowBackward);
}

Value ValueDiv(Value a, Value b, const char* label)
{
    Value divPart = ValuePow(b, CreateValue(-1, "litteral"), "DivPow");
    return ValueMul(a, divPart, label);
}

void ValueTanhBackward(Value operand1, Value operand2, Value result)
{
    ValueData* operand1Data = GetValueData(operand1);
    ValueData* resultData = GetValueData(result);

    operand1Data->Gradient += (1.0f - powf(resultData->Data, 2)) * resultData->Gradient;
}

Value ValueTanh(Value a, const char* label)
{
    ValueData* aData = GetValueData(a);

    float x = aData->Data;
    float result = 0.0f;

    if (x >= 40.0f) 
    {
        result = 1.0f;
    } 
    else if (x <= -40.0f) 
    {
        result = -1.0f;
    }
    else
    {
        result = (expf(2.0f * x) - 1) / (expf(2.0f * x) + 1);
    }

    return CreateValueFromOperation(result, label, "tanh", a, VALUE_EMPTY, ValueTanhBackward);
}

void ValueExpBackward(Value operand1, Value operand2, Value result)
{
    ValueData* operand1Data = GetValueData(operand1);
    ValueData* resultData = GetValueData(result);

    operand1Data->Gradient += resultData->Data * resultData->Gradient;
}

Value ValueExp(Value a, const char* label)
{
    ValueData* aData = GetValueData(a);
    float result = expf(aData->Data);

    return CreateValueFromOperation(result, label, "exp", a, VALUE_EMPTY, ValueExpBackward);
}

typedef struct
{
    ValueSpan Weights;
    Value Bias;
} Neuron;

float RandomFloat()
{
    return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

Neuron CreateNeuron(uint32_t inputCount)
{
    Value* weights = (Value*)malloc(inputCount * sizeof(Value));

    for (uint32_t i = 0; i < inputCount; i++)
    {
        weights[i] = CreateValue(RandomFloat(), "w");
    }

    return (Neuron)
    {
        .Weights = (ValueSpan) { .Items = weights, .Count = inputCount },
        .Bias = CreateValue(RandomFloat(), "b")
    };
}

Value ExecuteNeuron(const Neuron* neuron, ValueSpan inputs)
{
    assert(neuron->Weights.Count == inputs.Count);

    Value result = CreateValue(0.0f, "sum");

    for (uint32_t i = 0; i < inputs.Count; i++)
    {
        Value xw = ValueMul(inputs.Items[i], neuron->Weights.Items[i], "xw");
        result = ValueAdd(result, xw, "sum");
    }

    result = ValueAdd(result, neuron->Bias, "activation");
    result = ValueTanh(result, "output");

    return result;
}

void UpdateNeuronParameters(const Neuron* neuron, float step)
{
    for (uint32_t i = 0; i < neuron->Weights.Count; i++)
    {
        ValueData* data = GetValueData(neuron->Weights.Items[i]);

        data->Data += -step * data->Gradient;
        data->Gradient = 0.0f;
    }

    ValueData* data = GetValueData(neuron->Bias);

    data->Data += -step * data->Gradient;
    data->Gradient = 0.0f;
}

typedef struct
{
    uint32_t InputCount;
    uint32_t OutputCount;
    Neuron* Neurons;
} NeuronLayer;

NeuronLayer CreateNeuronLayer(uint32_t inputCount, uint32_t outputCount)
{
    // TODO: Review the mallocs here
    Neuron* neurons = malloc(outputCount * sizeof(Neuron));

    for (uint32_t i = 0; i < outputCount; i++)
    {
        neurons[i] = CreateNeuron(inputCount);
    }

    return (NeuronLayer)
    {
        .InputCount = inputCount,
        .OutputCount = outputCount,
        .Neurons = neurons
    };
}

void ExecuteNeuronLayer(const NeuronLayer* layer, ValueSpan inputs, ValueSpan outputs)
{
    assert(inputs.Count == layer->InputCount);
    assert(outputs.Count == layer->OutputCount);

    for (uint32_t i = 0; i < layer->OutputCount; i++)
    {
        outputs.Items[i] = ExecuteNeuron(&layer->Neurons[i], inputs);
    }
}

void UpdateNeuralLayerParameters(const NeuronLayer* layer, float step)
{
    for (uint32_t i = 0; i < layer->OutputCount; i++)
    {
        UpdateNeuronParameters(&layer->Neurons[i], step);
    }
}

typedef struct
{
    NeuronLayer* Layers;
    uint32_t LayerCount;
} NeuralNetwork;

NeuralNetwork CreateNeuralNetwork(uint32_t inputCount, uint32_t* outputsCount, uint32_t layerCount)
{
    NeuronLayer* layers = (NeuronLayer*)malloc(layerCount * sizeof(NeuronLayer));

    for (uint32_t i = 0; i < layerCount; i++)
    {
        uint32_t input = (i == 0) ? inputCount : outputsCount[i - 1]; 
        layers[i] = CreateNeuronLayer(input, outputsCount[i]);
    }

    return (NeuralNetwork)
    {
        .Layers = layers,
        .LayerCount = layerCount
    };
}

void ExecuteNeuralNetwork(const NeuralNetwork* neuralNetwork, ValueSpan inputs, ValueSpan outputs)
{
    Value tempInput[255];
    Value tempOutput[255];

    ValueSpan tempInputSpan = { .Items = tempInput, .Count = inputs.Count };

    for (uint32_t i = 0; i < inputs.Count; i++)
    {
        tempInput[i] = inputs.Items[i];
    }

    for (uint32_t i = 0; i < neuralNetwork->LayerCount; i++)
    {
        NeuronLayer* layer = &neuralNetwork->Layers[i];
        
        ValueSpan tempOutputSpan = { .Items = tempOutput, .Count = layer->OutputCount };

        ExecuteNeuronLayer(layer, tempInputSpan, tempOutputSpan);

        tempInputSpan.Count = tempOutputSpan.Count;

        for (uint32_t i = 0; i < tempOutputSpan.Count; i++)
        {
            tempInput[i] = tempOutput[i];
        }
    }

    for (uint32_t i = 0; i < outputs.Count; i++)
    {
        outputs.Items[i] = tempOutput[i];
    }
}

void UpdateNeuralNetworkParameters(const NeuralNetwork* neuralNetwork, float step)
{
    for (uint32_t i = 0; i < neuralNetwork->LayerCount; i++)
    {
        UpdateNeuralLayerParameters(&neuralNetwork->Layers[i], step);
    }
}

void TrainNeuralNetV0()
{
    srand(time(NULL));
    NeuralNetwork neuralNetwork = CreateNeuralNetwork(3, (uint32_t[]){ 4, 4, 1 }, 3);

    float xs[][3] = 
    {
        { 2.0f, 3.0f, -1.0f },
        { 3.0f, -1.0f, 0.5f },
        { 0.5f, 1.0f, 1.0f },
        { 1.0f, 1.0f, -1.0f }
    };

    Value pow2Value = CreateValue(2.0f, "Pow2Exp");

    Value ys[] = { CreateValue(1.0f, "y"), CreateValue(-1.0f, "y"), CreateValue(-1.0f, "y"), CreateValue(1.0f, "y") };
    
    Value outputArray[3];
    ValueSpan outputs = { .Items = outputArray, .Count = ARRAYSIZE(outputArray) };

    for (uint32_t i = 0; i < 20; i++)
    {
        // Forward pass
        Value loss = CreateValue(0.0f, "loss");

        for (uint32_t j = 0; j < ARRAYSIZE(xs); j++)
        {
            float* x = xs[j];
            Value inputs[3];

            for (uint32_t k = 0; k < ARRAYSIZE(inputs); k++)
            {
                inputs[k] = CreateValue(x[k], "input");
            }

            ExecuteNeuralNetwork(&neuralNetwork, (ValueSpan) { .Items = inputs, .Count = ARRAYSIZE(inputs) }, outputs);

            Value yPrediction = outputs.Items[0];
            Value yDiff = ValueSub(yPrediction, ys[j], "yDiff");
            Value yDiffPow = ValuePow(yDiff, pow2Value, "yDiffPow2");

            loss = ValueAdd(loss, yDiffPow, "loss");
        }

        // Backward
        Backward(loss);

        // Update parameters
        UpdateNeuralNetworkParameters(&neuralNetwork, 0.1f);

        ValueData* lossData = GetValueData(loss);
        printf("Training step %d: %f, %d\n", i, lossData->Data, loss);
    }

    printf("===== Results =====\n");

    for (uint32_t j = 0; j < ARRAYSIZE(xs); j++)
    {
        float* x = xs[j];
        Value inputs[3];

        for (uint32_t k = 0; k < ARRAYSIZE(inputs); k++)
        {
            inputs[k] = CreateValue(x[k], "input");
        }

        ExecuteNeuralNetwork(&neuralNetwork, (ValueSpan) { .Items = inputs, .Count = ARRAYSIZE(inputs) }, outputs);

        Value yPrediction = outputs.Items[0];
        ValueData* yPredictionData = GetValueData(yPrediction);
        ValueData* ysData = GetValueData(ys[j]);

        printf("Output: %f, Expected: %f\n", yPredictionData->Data, ysData->Data);
    }

    ElemExitApplication(0);
}
