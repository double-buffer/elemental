#pragma once

#include "Elemental.h"

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

void SampleRegisterInputActionBinding(InputActionBindingSpan* bindings, ElemInputId inputId, uint32_t index, InputActionBindingType bindingType, float* actionValue)
{
    bindings->Items[bindings->Length++] = (InputActionBinding)
    { 
        .InputId = inputId, 
        .BindingType = bindingType,
        .Index = index,
        .ActionValue = actionValue
    };
}

void SampleUpdateInputActions(InputActionBindingSpan* inputActionBindings)
{
    ElemInputStream inputStream = ElemGetInputStream();

    for (uint32_t i = 0; i < inputActionBindings->Length; i++)
    {
        InputActionBinding binding = inputActionBindings->Items[i];

        if (binding.BindingType == InputActionBindingType_Released)
        {
            *binding.ActionValue = 0.0f;
        }
    }

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        for (uint32_t j = 0; j < inputActionBindings->Length; j++)
        {
            InputActionBinding* binding = &inputActionBindings->Items[j];

            if (inputEvent->InputId == binding->InputId && inputEvent->InputDeviceTypeIndex == binding->Index)
            {
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
                    if ((inputEvent->ElapsedSeconds - binding->LastReleasedTime) > 0.25f)
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
}
