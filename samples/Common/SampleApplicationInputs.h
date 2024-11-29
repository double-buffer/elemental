#pragma once

#include "Elemental.h"
#include "SampleInputs.h"

typedef struct
{
    float SwitchShowCursor;
    float ExitApp;
} SampleApplicationInputActions;

typedef struct
{
    bool ExitApplication;
    bool ShowCursor;
    bool HideCursor;
    bool IsCursorDisplayed;
} SampleApplicationInputState;

typedef struct
{
    SampleApplicationInputActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    SampleApplicationInputState State;
} SampleApplicationInputs;

void SampleApplicationInputsInit(SampleApplicationInputs* inputs)
{
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyF1, 0, SampleInputActionBindingType_Released, &inputs->InputActions.SwitchShowCursor);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyEscape, 0, SampleInputActionBindingType_Released, &inputs->InputActions.ExitApp);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonB, 0, SampleInputActionBindingType_Released, &inputs->InputActions.ExitApp);

    inputs->State.IsCursorDisplayed = true;
}

void SampleApplicationInputsUpdate(ElemInputStream inputStream, SampleApplicationInputs* inputs, float deltaTimeInSeconds)
{
    SampleApplicationInputState* state = &inputs->State;
    SampleApplicationInputActions* inputActions = &inputs->InputActions;

    SampleUpdateInputActions(&inputs->InputActionBindings, inputStream);

    state->ExitApplication = inputActions->ExitApp;
    state->ShowCursor = false;
    state->HideCursor = false;

    if (inputActions->SwitchShowCursor)
    {
        if (!state->IsCursorDisplayed)
        {
            state->ShowCursor = true;
        }
        else
        {
            state->HideCursor = true;
        } 

        state->IsCursorDisplayed = !state->IsCursorDisplayed;
    }
}
