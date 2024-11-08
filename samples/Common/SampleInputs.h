#pragma once

#include "Elemental.h"

typedef enum
{
    SampleInputActionBindingType_Value,
    SampleInputActionBindingType_Released,
    SampleInputActionBindingType_ReleasedSwitch,
    SampleInputActionBindingType_DoubleReleasedSwitch,
} SampleInputActionBindingType;

typedef struct
{
    ElemInputId InputId;
    SampleInputActionBindingType BindingType;
    uint32_t Index;
    float* ActionValue;
    uint32_t ReleasedCount;
    double LastReleasedTime;
} SampleInputActionBinding;

typedef struct
{
    SampleInputActionBinding Items[255];
    uint32_t Length;
} SampleInputActionBindingSpan;

// TODO: Extract ModelView into a separate file
typedef struct
{
    float TranslateLeft;
    float TranslateRight;
    float TranslateUp;
    float TranslateDown;
    float RotateSideLeft;
    float RotateSideRight;
    float ZoomIn;
    float ZoomOut;

    float Touch;
    float TouchReleased;
    float TouchTranslateLeft;
    float TouchTranslateRight;
    float TouchTranslateUp;
    float TouchTranslateDown;
    float TouchPositionX;
    float TouchPositionY;

    float Touch2;
    float Touch2PositionX;
    float Touch2PositionY;
    
    float TouchRotateSide;
    float Action1;
    float SwitchShowCursor;
    float ExitApp;
} SampleStandardInputActions;

void SampleRegisterInputActionBinding(SampleInputActionBindingSpan* bindings, ElemInputId inputId, uint32_t index, SampleInputActionBindingType bindingType, float* actionValue)
{
    bindings->Items[bindings->Length++] = (SampleInputActionBinding)
    { 
        .InputId = inputId, 
        .BindingType = bindingType,
        .Index = index,
        .ActionValue = actionValue
    };
}

void SampleUpdateInputActions(SampleInputActionBindingSpan* inputActionBindings, ElemInputStream inputStream)
{
    for (uint32_t i = 0; i < inputActionBindings->Length; i++)
    {
        SampleInputActionBinding binding = inputActionBindings->Items[i];

        if (binding.BindingType == SampleInputActionBindingType_Released)
        {
            *binding.ActionValue = 0.0f;
        }
    }

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        printf("Input Event: %d, %f\n", inputEvent->InputId, inputEvent->Value);

        for (uint32_t j = 0; j < inputActionBindings->Length; j++)
        {
            SampleInputActionBinding* binding = &inputActionBindings->Items[j];

            if (inputEvent->InputId == binding->InputId && inputEvent->InputDeviceTypeIndex == binding->Index)
            {
                if (binding->BindingType == SampleInputActionBindingType_Value)
                {
                    *binding->ActionValue = inputEvent->Value;
                }
                else if (binding->BindingType == SampleInputActionBindingType_Released)
                {
                    *binding->ActionValue = !inputEvent->Value;
                }
                else if (binding->BindingType == SampleInputActionBindingType_ReleasedSwitch && inputEvent->Value == 0.0f)
                {
                    *binding->ActionValue = !*binding->ActionValue;
                }
                else if (binding->BindingType == SampleInputActionBindingType_DoubleReleasedSwitch && inputEvent->Value == 0.0f)
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

void SampleRegisterStandardInputBindings(SampleInputActionBindingSpan* inputActionBindings, SampleStandardInputActions* inputActions)
{
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyA, 0, SampleInputActionBindingType_Value, &inputActions->TranslateLeft);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyD, 0, SampleInputActionBindingType_Value, &inputActions->TranslateRight);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyW, 0, SampleInputActionBindingType_Value, &inputActions->TranslateUp);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyS, 0, SampleInputActionBindingType_Value, &inputActions->TranslateDown);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyQ, 0, SampleInputActionBindingType_Value, &inputActions->RotateSideLeft);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyE, 0, SampleInputActionBindingType_Value, &inputActions->RotateSideRight);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyZ, 0, SampleInputActionBindingType_Value, &inputActions->ZoomIn);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyX, 0, SampleInputActionBindingType_Value, &inputActions->ZoomOut);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeySpacebar, 0, SampleInputActionBindingType_Value, &inputActions->Action1);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyF1, 0, SampleInputActionBindingType_ReleasedSwitch, &inputActions->SwitchShowCursor);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_KeyEscape, 0, SampleInputActionBindingType_Released, &inputActions->ExitApp);

    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseLeftButton, 0, SampleInputActionBindingType_Value, &inputActions->Touch);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseLeftButton, 0, SampleInputActionBindingType_DoubleReleasedSwitch, &inputActions->Action1);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseLeftButton, 0, SampleInputActionBindingType_Released, &inputActions->TouchReleased);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseRightButton, 0, SampleInputActionBindingType_Value, &inputActions->TouchRotateSide);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseAxisXNegative, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateLeft);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseAxisXPositive, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateRight);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseAxisYNegative, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateUp);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseAxisYPositive, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateDown);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseWheelPositive, 0, SampleInputActionBindingType_Value, &inputActions->ZoomIn);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseWheelNegative, 0, SampleInputActionBindingType_Value, &inputActions->ZoomOut);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_MouseMiddleButton, 0, SampleInputActionBindingType_Value, &inputActions->Action1);

    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_GamepadLeftStickXNegative, 0, SampleInputActionBindingType_Value, &inputActions->TranslateLeft);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_GamepadLeftStickXPositive, 0, SampleInputActionBindingType_Value, &inputActions->TranslateRight);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_GamepadLeftStickYPositive, 0, SampleInputActionBindingType_Value, &inputActions->TranslateUp);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_GamepadLeftStickYNegative, 0, SampleInputActionBindingType_Value, &inputActions->TranslateDown);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_GamepadLeftStickButton, 0, SampleInputActionBindingType_Value, &inputActions->Action1);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputID_GamepadLeftTrigger, 0, SampleInputActionBindingType_Value, &inputActions->RotateSideLeft);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputID_GamepadRightTrigger, 0, SampleInputActionBindingType_Value, &inputActions->RotateSideRight);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputID_GamepadLeftShoulder, 0, SampleInputActionBindingType_Value, &inputActions->ZoomOut);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputID_GamepadRightShoulder, 0, SampleInputActionBindingType_Value, &inputActions->ZoomIn);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputID_GamepadButtonA, 0, SampleInputActionBindingType_Value, &inputActions->Action1);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputID_GamepadButtonB, 0, SampleInputActionBindingType_Released, &inputActions->ExitApp);

    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_Touch, 0, SampleInputActionBindingType_Value, &inputActions->Touch);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchXNegative, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateLeft);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchXPositive, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateRight);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchYNegative, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateUp);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchYPositive, 0, SampleInputActionBindingType_Value, &inputActions->TouchTranslateDown);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchXAbsolutePosition, 0, SampleInputActionBindingType_Value, &inputActions->TouchPositionX);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchYAbsolutePosition, 0, SampleInputActionBindingType_Value, &inputActions->TouchPositionY);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_Touch, 1, SampleInputActionBindingType_Value, &inputActions->Touch2);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchXAbsolutePosition, 1, SampleInputActionBindingType_Value, &inputActions->Touch2PositionX);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_TouchYAbsolutePosition, 1, SampleInputActionBindingType_Value, &inputActions->Touch2PositionY);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_Touch, 0, SampleInputActionBindingType_Released, &inputActions->TouchReleased);
    SampleRegisterInputActionBinding(inputActionBindings, ElemInputId_Touch, 0, SampleInputActionBindingType_DoubleReleasedSwitch, &inputActions->Action1);
}

