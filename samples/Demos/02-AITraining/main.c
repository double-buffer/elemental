#include "Elemental.h"
#include "NeuralNetV0.c"
#include "NeuralNetV1.c"

void InitSample(void* payload)
{
    printf("============ NeuralNetV0 ============\n");
    TrainNeuralNetV0();

    printf("============ NeuralNetV1 ============\n");
    TrainNeuralNetV1();
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
