#pragma once

#include "Elemental.h"
#include "SampleInputs.h"
#include "SampleMath.h"

#define SAMPLE_MODELVIEWER_ROTATION_TOUCH_DECREASE_SPEED 0.001f
#define SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED 4.0f
#define SAMPLE_MODELVIEWER_ROTATION_TOUCH_MAX_DELTA 0.3f
#define SAMPLE_MODELVIEWER_ROTATION_MULTITOUCH_SPEED 200.0f
#define SAMPLE_MODELVIEWER_ROTATION_ACCELERATION 500.0f
#define SAMPLE_MODELVIEWER_ROTATION_FRICTION 60.0f
#define SAMPLE_MODELVIEWER_ROTATION_ZOOM_MULTITOUCH_SPEED 1000.0f
#define SAMPLE_MODELVIEWER_ROTATION_ZOOM_SPEED 5.0f

typedef struct
{
    float RotateLeft;
    float RotateRight;
    float RotateUp;
    float RotateDown;
    float RotateSideLeft;
    float RotateSideRight;
    float ZoomIn;
    float ZoomOut;

    float Touch;
    float TouchReleased;
    float TouchRotateLeft;
    float TouchRotateRight;
    float TouchRotateUp;
    float TouchRotateDown;
    float TouchPositionX;
    float TouchPositionY;

    float Touch2;
    float Touch2PositionX;
    float Touch2PositionY;
    
    float TouchRotateSide;
    float Action;
} SampleModelViewerInputActions;

typedef struct
{
    SampleVector3 RotationDelta;
    SampleVector2 RotationTouch;
    SampleVector3 CurrentRotationSpeed;
    float PreviousTouchDistance;
    float PreviousTouchAngle;
    float Zoom;
    float Action;
} SampleModelViewerInputState;

typedef struct
{
    SampleModelViewerInputActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    SampleModelViewerInputState State;
} SampleModelViewerInputs;

void SampleModelViewerInputsInit(SampleModelViewerInputs* inputs)
{
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyA, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyD, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyW, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyS, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateDown);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyQ, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyE, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyZ, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyX, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeySpacebar, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseLeftButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Touch);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseLeftButton, 0, SampleInputActionBindingType_DoubleReleasedSwitch, &inputs->InputActions.Action);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseLeftButton, 0, SampleInputActionBindingType_Released, &inputs->InputActions.TouchReleased);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseRightButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateSide);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateDown);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseWheelPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseWheelNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseMiddleButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateDown);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputID_GamepadLeftTrigger, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputID_GamepadRightTrigger, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputID_GamepadLeftShoulder, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputID_GamepadRightShoulder, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputID_GamepadButtonA, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_Touch, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Touch);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateDown);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchXAbsolutePosition, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchPositionX);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchYAbsolutePosition, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchPositionY);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_Touch, 1, SampleInputActionBindingType_Value, &inputs->InputActions.Touch2);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchXAbsolutePosition, 1, SampleInputActionBindingType_Value, &inputs->InputActions.Touch2PositionX);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_TouchYAbsolutePosition, 1, SampleInputActionBindingType_Value, &inputs->InputActions.Touch2PositionY);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_Touch, 0, SampleInputActionBindingType_Released, &inputs->InputActions.TouchReleased);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_Touch, 0, SampleInputActionBindingType_DoubleReleasedSwitch, &inputs->InputActions.Action);
}

void SampleModelViewerInputsResetTouchParameters(SampleModelViewerInputState* state) 
{
    state->RotationTouch = V2Zero;
    state->PreviousTouchDistance = 0.0f;
    state->PreviousTouchAngle = 0.0f;   
}

void SampleModelViewerInputsUpdate(ElemInputStream inputStream, SampleModelViewerInputs* inputs, float deltaTimeInSeconds)
{
    SampleModelViewerInputState* state = &inputs->State;
    SampleModelViewerInputActions* inputActions = &inputs->InputActions;
    state->RotationDelta = V3Zero; 

    SampleUpdateInputActions(&inputs->InputActionBindings, inputStream);

    if (inputActions->Touch)
    {
        if (inputActions->Touch2)
        {
            SampleVector2 touchPosition = (SampleVector2) { inputActions->TouchPositionX, inputActions->TouchPositionY };
            SampleVector2 touchPosition2 = (SampleVector2) { inputActions->Touch2PositionX, inputActions->Touch2PositionY };

            SampleVector2 diffVector = SampleSubstractV2(touchPosition, touchPosition2);
            float distance = SampleMagnitudeV2(diffVector);
            float angle = atan2(diffVector.X, diffVector.Y);

            if (state->PreviousTouchDistance != 0.0f)
            {
                state->Zoom += (distance - state->PreviousTouchDistance) * SAMPLE_MODELVIEWER_ROTATION_ZOOM_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            if (state->PreviousTouchAngle != 0.0f)
            {
                state->RotationDelta.Z = -SampleNormalizeAngle(angle - state->PreviousTouchAngle) * SAMPLE_MODELVIEWER_ROTATION_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            state->PreviousTouchDistance = distance;
            state->PreviousTouchAngle = angle;
        }
        else 
        {
            SampleModelViewerInputsResetTouchParameters(state);

            state->RotationDelta.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
            state->RotationDelta.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        }
    }
    else if (inputActions->TouchRotateSide)
    {
        SampleModelViewerInputsResetTouchParameters(state);
        state->RotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else if (inputActions->TouchReleased && !inputActions->Touch2)
    {
        SampleModelViewerInputsResetTouchParameters(state);

        state->RotationTouch.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        state->RotationTouch.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else
    {
        SampleVector3 direction = SampleNormalizeV3((SampleVector3) 
        { 
            .X = (inputActions->RotateUp - inputActions->RotateDown),
            .Y = (inputActions->RotateLeft - inputActions->RotateRight),
            .Z = (inputActions->RotateSideLeft - inputActions->RotateSideRight)
        });

        if (SampleMagnitudeSquaredV3(direction))
        {
            SampleVector3 acceleration = SampleAddV3(SampleMulScalarV3(direction, SAMPLE_MODELVIEWER_ROTATION_ACCELERATION), SampleMulScalarV3(SampleInverseV3(state->CurrentRotationSpeed), SAMPLE_MODELVIEWER_ROTATION_FRICTION));

            SampleModelViewerInputsResetTouchParameters(state);
            state->RotationDelta = SampleAddV3(SampleMulScalarV3(acceleration, 0.5f * SamplePow2f(deltaTimeInSeconds)), SampleMulScalarV3(state->CurrentRotationSpeed, deltaTimeInSeconds));
            state->CurrentRotationSpeed = SampleAddV3(SampleMulScalarV3(acceleration, deltaTimeInSeconds), state->CurrentRotationSpeed);
        }
    }

    if (SampleMagnitudeSquaredV2(state->RotationTouch) > 0)
    {
        if (SampleMagnitudeV2(state->RotationTouch) > SAMPLE_MODELVIEWER_ROTATION_TOUCH_MAX_DELTA)
        {
            state->RotationTouch = SampleMulScalarV2(SampleNormalizeV2(state->RotationTouch), SAMPLE_MODELVIEWER_ROTATION_TOUCH_MAX_DELTA);
        }

        state->RotationDelta = SampleAddV3(state->RotationDelta, (SampleVector3){ state->RotationTouch.X, state->RotationTouch.Y, 0.0f });

        SampleVector2 inverse = SampleMulScalarV2(SampleNormalizeV2(SampleInverseV2(state->RotationTouch)), SAMPLE_MODELVIEWER_ROTATION_TOUCH_DECREASE_SPEED);
        state->RotationTouch = SampleAddV2(state->RotationTouch, inverse);

        if (SampleMagnitudeV2(state->RotationTouch) < 0.001f)
        {
            SampleModelViewerInputsResetTouchParameters(state);
        }
    }

    state->Zoom += (inputActions->ZoomIn - inputActions->ZoomOut) * SAMPLE_MODELVIEWER_ROTATION_ZOOM_SPEED * deltaTimeInSeconds; 
    state->Action = inputActions->Action;
}
