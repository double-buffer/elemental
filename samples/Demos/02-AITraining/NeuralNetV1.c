#include "Elemental.h"
#include "SampleUtils.h"
#include <math.h>
#include <time.h>

void TrainNeuralNetV1()
{
    srand(time(NULL));

    float xs[][3] = 
    {
        { 2.0f, 3.0f, -1.0f },
        { 3.0f, -1.0f, 0.5f },
        { 0.5f, 1.0f, 1.0f },
        { 1.0f, 1.0f, -1.0f }
    };

    float ys[] = { 1.0f, -1.0f, -1.0f, 1.0f };

    for (uint32_t i = 0; i < 20; i++)
    {
        // Forward pass

        // Backward

        // Update parameters

        printf("Training step %d:\n", i);
    }

    printf("===== Results =====\n");

    for (uint32_t j = 0; j < ARRAYSIZE(xs); j++)
    {
        printf("Output: %f, Expected: %f\n", 0.0f, 0.0f);
    }

    ElemExitApplication(0);
}
