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

    float AngularVelocity;
    float AngularVelocityReleased;
    float AngularVelocityXNegative;
    float AngularVelocityXPositive;
    float AngularVelocityYNegative;
    float AngularVelocityYPositive;
    float AngularVelocityZNegative;
    float AngularVelocityZPositive;

    float AccelerometerZNegative;
    float AccelerometerZPositive;
} SampleInputsModelViewerActions;

typedef struct
{
    ElemVector3 RotationDelta;
    SampleVector2 RotationTouch;
    ElemVector3 CurrentRotationSpeed;
    float PreviousTouchDistance;
    float PreviousTouchAngle;
    float Zoom;
    float Action;
    float InitialAccelerometerZDelta;
} SampleInputsModelViewerState;

typedef struct
{
    SampleInputsModelViewerActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    SampleInputsModelViewerState State;
} SampleInputsModelViewer;

void SampleInputsModelViewerInit(SampleInputsModelViewer* inputs)
{
    // TODO: Review rotation signs

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
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.TouchRotateDown); // TODO: It seems inverted
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseWheelPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseWheelNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseMiddleButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);

    // TODO: Be careful because it can cause side issues with the touch controls
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonX, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AngularVelocity);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonX, 0, SampleInputActionBindingType_Released, &inputs->InputActions.AngularVelocityReleased);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AngularVelocityYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AngularVelocityYNegative);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AngularVelocityYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AngularVelocityYPositive);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AngularVelocityXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AngularVelocityXNegative); // TODO: It seems inverted
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AngularVelocityXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AngularVelocityXPositive);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AngularVelocityZNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AngularVelocityZNegative);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AngularVelocityZPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AngularVelocityZPositive);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AccelerometerZNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AccelerometerZNegative);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_AccelerometerZPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.AccelerometerZPositive);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateDown);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftTrigger, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightTrigger, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftShoulder, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightShoulder, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadDpadDown, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadDpadUp, 0, SampleInputActionBindingType_Value, &inputs->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonA, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);

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

void SampleInputsModelViewerResetTouchParameters(SampleInputsModelViewerState* state) 
{
    state->RotationTouch = V2Zero;
    state->PreviousTouchDistance = 0.0f;
    state->PreviousTouchAngle = 0.0f;   
}

void SampleInputsModelViewerUpdate(ElemInputStream inputStream, SampleInputsModelViewer* inputs, float deltaTimeInSeconds)
{
    // TODO: Review rotation order in left handed
    SampleInputsModelViewerState* state = &inputs->State;
    SampleInputsModelViewerActions* inputActions = &inputs->InputActions;
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
            SampleInputsModelViewerResetTouchParameters(state);

            state->RotationDelta.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
            state->RotationDelta.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        }
    }
    else if (inputActions->TouchRotateSide)
    {
        SampleInputsModelViewerResetTouchParameters(state);

        state->RotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else if (inputActions->TouchReleased && !inputActions->Touch2)
    {
        SampleInputsModelViewerResetTouchParameters(state);

        state->RotationTouch.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        state->RotationTouch.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else if (inputActions->AngularVelocity)
    {
        state->RotationDelta.X = -(inputActions->AngularVelocityXPositive - inputActions->AngularVelocityXNegative) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds; // TODO: Is it inverted?
        state->RotationDelta.Y = -(inputActions->AngularVelocityYPositive - inputActions->AngularVelocityYNegative) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds; // TODO: Is it inverted?
        state->RotationDelta.Z = (inputActions->AngularVelocityZPositive - inputActions->AngularVelocityZNegative) * SAMPLE_MODELVIEWER_ROTATION_TOUCH_SPEED * deltaTimeInSeconds;

        // TODO: Use the accelerometer
        /*float accelerometerZDelta = inputActions->AccelerometerZPositive - inputActions->AccelerometerZNegative;

        if (state->InitialAccelerometerZDelta)
        {
            float finalDelta = accelerometerZDelta - state->InitialAccelerometerZDelta;
            printf("Zoom: final=%f, AccDelta=%f, Initial=%f\n", finalDelta, accelerometerZDelta, state->InitialAccelerometerZDelta);
            state->Zoom += finalDelta * 2.0;
        }

        state->InitialAccelerometerZDelta = accelerometerZDelta;*/
    }
    else if (inputActions->AngularVelocityReleased)
    {
        state->InitialAccelerometerZDelta = 0.0f;
    }
    else
    {
        ElemVector3 direction = SampleNormalizeV3((ElemVector3) 
        { 
            .X = (inputActions->RotateUp - inputActions->RotateDown),
            .Y = (inputActions->RotateLeft - inputActions->RotateRight),
            .Z = (inputActions->RotateSideLeft - inputActions->RotateSideRight)
        });

        if (SampleMagnitudeSquaredV3(direction))
        {
            ElemVector3 acceleration = SampleAddV3(SampleMulScalarV3(direction, SAMPLE_MODELVIEWER_ROTATION_ACCELERATION), SampleMulScalarV3(SampleInverseV3(state->CurrentRotationSpeed), SAMPLE_MODELVIEWER_ROTATION_FRICTION));

            SampleInputsModelViewerResetTouchParameters(state);
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

        state->RotationDelta = SampleAddV3(state->RotationDelta, (ElemVector3){ state->RotationTouch.X, state->RotationTouch.Y, 0.0f });

        SampleVector2 inverse = SampleMulScalarV2(SampleNormalizeV2(SampleInverseV2(state->RotationTouch)), SAMPLE_MODELVIEWER_ROTATION_TOUCH_DECREASE_SPEED);
        state->RotationTouch = SampleAddV2(state->RotationTouch, inverse);

        if (SampleMagnitudeV2(state->RotationTouch) < 0.001f)
        {
            SampleInputsModelViewerResetTouchParameters(state);
        }
    }

    state->Zoom += (inputActions->ZoomIn - inputActions->ZoomOut) * SAMPLE_MODELVIEWER_ROTATION_ZOOM_SPEED * deltaTimeInSeconds; 
    state->Action = inputActions->Action;
}
