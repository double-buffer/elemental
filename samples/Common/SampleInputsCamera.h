#pragma once

#include "Elemental.h"
#include "SampleInputs.h"
#include "SampleMath.h"

typedef struct
{
    float MoveLeft;
    float MoveRight;
    float MoveForward;
    float MoveBackward;

    float RotateLeft;
    float RotateRight;
    float RotateUp;
    float RotateDown;
    float RotateSideLeft;
    float RotateSideRight;

    float RotateMouseLeft;
    float RotateMouseRight;
    float RotateMouseUp;
    float RotateMouseDown;
    float RotateMouse;

    float Action;
} SampleInputsCameraActions;

typedef struct
{
    SampleVector3 Position;
    SampleVector3 PositionVelocity;
    SampleVector3 Rotation;
    SampleVector3 RotationVelocity;
} SampleCamera;

typedef struct
{
    SampleCamera Camera;
    SampleCamera DebugCamera;
    SampleMatrix4x4 ProjectionMatrix;
    SampleMatrix4x4 ViewProjMatrix;
    float Action;
} SampleInputsCameraState;

typedef struct
{
    SampleInputsCameraActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    SampleInputsCameraState State;
} SampleInputsCamera;

// TODO: Allow initial camera setup
void SampleInputsCameraInit(SampleInputsCamera* inputs)
{
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyA, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyD, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyW, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveForward);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyS, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveBackward);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyLeftArrow, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyRightArrow, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyUpArrow, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyDownArrow, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateDown);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyQ, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyE, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideRight);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveForward);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveBackward);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateDown);
    
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseRightButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouse);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseDown);

    //SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyQ, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideLeft);
    //SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyE, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideRight);

    // TODO: Remove that one?
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeySpacebar, 0, SampleInputActionBindingType_Value, &inputs->InputActions.Action);

    // TODO: Temporary Init code for now
    SampleCamera* camera = &(inputs->State.Camera);
    camera->Position = (SampleVector3) { 0.0f, 100.0f, -10.0f };
    camera->Rotation = (SampleVector3) { 0.0f, 1.2f, 0 };

    // TODO: Allow the reset of camera to the passed initial position when we press a button
}

SampleVector3 ComputeDeltaAndVelocity(SampleVector3* currentVelocity, SampleVector3 directionVector, float accelerationFactor, float frictionFactor, float deltaTimeInSeconds)
{
    SampleVector3 acceleration = SampleAddV3(SampleMulScalarV3(directionVector, accelerationFactor), SampleMulScalarV3(SampleInverseV3(*currentVelocity), frictionFactor));

    SampleVector3 delta = SampleAddV3(SampleMulScalarV3(acceleration, 0.5f * SamplePow2f(deltaTimeInSeconds)), SampleMulScalarV3(*currentVelocity, deltaTimeInSeconds));
    *currentVelocity = SampleAddV3(SampleMulScalarV3(acceleration, deltaTimeInSeconds), *currentVelocity);

    return delta;
}

SampleMatrix4x4 UpdateCamera(SampleCamera* camera, const SampleInputsCameraActions* inputActions, const ElemSwapChainUpdateParameters* updateParameters)
{
    // TODO: Should we normalize the input direction? It feels snappier without it.
    
    SampleVector3 movementDirection = (SampleVector3) 
    { 
        .X = (inputActions->MoveRight - inputActions->MoveLeft),
        .Y = 0.0f,
        .Z = (inputActions->MoveForward - inputActions->MoveBackward)
    };
    
    SampleVector3 movementDelta = ComputeDeltaAndVelocity(&camera->PositionVelocity, movementDirection, 15000.0f, 40.0f, updateParameters->DeltaTimeInSeconds);
    //printf("Current Pos Vel: %f %f %f\n", camera->PositionVelocity.X, camera->PositionVelocity.Y, camera->PositionVelocity.Z);

    SampleVector3 rotationDirection = (SampleVector3) 
    { 
        .X = (inputActions->RotateUp - inputActions->RotateDown),
        .Y = (inputActions->RotateLeft - inputActions->RotateRight),
        .Z = 0.0f,
    };

    // TODO: Prevent X Rotate above 180 deg
    SampleVector3 rotationDelta = ComputeDeltaAndVelocity(&camera->RotationVelocity, rotationDirection, 80.0f, 40.0f, updateParameters->DeltaTimeInSeconds);

    if (SampleMagnitudeSquaredV3(rotationDelta))
    {
        camera->Rotation = SampleAddV3(camera->Rotation, rotationDelta);
    }

    if (inputActions->RotateMouse)
    {
        SampleVector3 rotationMouseDelta = (SampleVector3) 
        { 
            .X = (inputActions->RotateMouseUp - inputActions->RotateMouseDown),
            .Y = (inputActions->RotateMouseLeft - inputActions->RotateMouseRight),
            .Z = 0.0f,
        };

        if (SampleMagnitudeSquaredV3(rotationMouseDelta))
        {
            camera->Rotation = SampleAddV3(camera->Rotation, SampleMulScalarV3(rotationMouseDelta, 1.0f * updateParameters->DeltaTimeInSeconds));
        }
    }
        
    // TODO: Review this
    SampleVector4 rotationQuaternion = SampleMulQuat(SampleCreateQuaternion((SampleVector3){ 1, 0, 0 }, camera->Rotation.X), 
                                                     SampleMulQuat(SampleCreateQuaternion((SampleVector3){ 0, 0, 1 }, camera->Rotation.Z),
                                                                   SampleCreateQuaternion((SampleVector3){ 0, 1, 0 }, camera->Rotation.Y)));

    if (SampleMagnitudeSquaredV3(movementDelta))
    {
        SampleMatrix4x4 translationTransform = SampleCreateTransformMatrix(rotationQuaternion, V3Zero);
        SampleVector3 rotatedMovementDelta = SampleTransformPointV3(movementDelta, translationTransform);
        camera->Position = SampleAddV3(camera->Position, rotatedMovementDelta);
    }

    SampleMatrix4x4 targetTransform = SampleCreateTransformMatrix(rotationQuaternion, V3Zero);
    SampleVector3 cameraTarget = SampleTransformPointV3((SampleVector3) { 0.0f, 0.0f, 1.0f }, targetTransform);
    cameraTarget = SampleAddV3(cameraTarget, camera->Position);

    return SampleCreateLookAtLHMatrix(camera->Position, cameraTarget, (SampleVector3) { 0.0f, 1.0f, 0.0f });
}

void SampleInputsCameraUpdate(ElemInputStream inputStream, SampleInputsCamera* inputs, const ElemSwapChainUpdateParameters* updateParameters)
{
    SampleInputsCameraState* state = &inputs->State;
    SampleInputsCameraActions* inputActions = &inputs->InputActions;

    SampleUpdateInputActions(&inputs->InputActionBindings, inputStream);

    // TODO: Change speed with a special "run" button
    // TODO: Select debug camera when needed 
    SampleCamera* currentCamera = &state->Camera;
    SampleMatrix4x4 viewMatrix = UpdateCamera(currentCamera, inputActions, updateParameters);

    if (updateParameters->SizeChanged || state->ProjectionMatrix.m[2][3] == 0.0f)
    {
        state->ProjectionMatrix = SampleCreatePerspectiveProjectionMatrix(0.78f, updateParameters->SwapChainInfo.AspectRatio, 0.001f);
    }

    state->ViewProjMatrix = SampleMulMatrix4x4(viewMatrix, state->ProjectionMatrix);
    state->Action = inputActions->Action;
}
