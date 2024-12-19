#pragma once

#include "Elemental.h"
#include "SampleInputs.h"

typedef struct
{
    float SwitchShowCursor;
    float ExitApp;
} SampleInputsApplicationActions;

typedef struct
{
    bool ExitApplication;
    bool ShowCursor;
    bool HideCursor;
    bool IsCursorDisplayed;
} SampleInputsApplicationState;

typedef struct
{
    SampleInputsApplicationActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    SampleInputsApplicationState State;
} SampleInputsApplication;

void SampleInputsApplicationInit(SampleInputsApplication* inputs)
{
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyF1, 0, SampleInputActionBindingType_Released, &inputs->InputActions.SwitchShowCursor);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyEscape, 0, SampleInputActionBindingType_Released, &inputs->InputActions.ExitApp);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonB, 0, SampleInputActionBindingType_Released, &inputs->InputActions.ExitApp);

    inputs->State.IsCursorDisplayed = true;
}

void SampleInputsApplicationUpdate(ElemInputStream inputStream, SampleInputsApplication* inputs, float deltaTimeInSeconds)
{
    SampleInputsApplicationState* state = &inputs->State;
    SampleInputsApplicationActions* inputActions = &inputs->InputActions;

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
