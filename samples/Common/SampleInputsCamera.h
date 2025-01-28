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
    float Action2;
    float Action3;
    float IncreaseCounter;
    float DecreaseCounter;
} SampleInputsCameraActions;

typedef struct
{
    ElemVector3 Position;
    ElemVector3 PositionVelocity;
    ElemVector3 Rotation;
    ElemVector3 RotationVelocity;
} SampleCamera;

typedef struct
{
    SampleCamera Camera;
    // TODO: Instead of having a debug camera, we could "freeze" the camera like in AW2
    SampleCamera DebugCamera;
    SampleMatrix4x4 ViewMatrix;
    SampleMatrix4x4 ProjectionMatrix;
    SampleMatrix4x4 ViewProjMatrix;
    SampleMatrix4x4 InverseViewProjMatrix;
    SampleMatrix4x4 InverseViewMatrix;
    SampleMatrix4x4 InverseProjectionMatrix;
    float Action;
    float Action2;
    float Action3;
    float Counter;
    bool HasChanged;
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
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_Key1, 0, SampleInputActionBindingType_Released, &inputs->InputActions.DecreaseCounter);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_Key2, 0, SampleInputActionBindingType_Released, &inputs->InputActions.IncreaseCounter);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeySpacebar, 0, SampleInputActionBindingType_Released, &inputs->InputActions.Action);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyEnter, 0, SampleInputActionBindingType_Released, &inputs->InputActions.Action2);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyF, 0, SampleInputActionBindingType_Released, &inputs->InputActions.Action3);

    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveForward);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftStickYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.MoveBackward);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightStickYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateDown);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadLeftShoulder, 0, SampleInputActionBindingType_Released, &inputs->InputActions.DecreaseCounter);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadRightShoulder, 0, SampleInputActionBindingType_Released, &inputs->InputActions.IncreaseCounter);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonA, 0, SampleInputActionBindingType_Released, &inputs->InputActions.Action);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonX, 0, SampleInputActionBindingType_Released, &inputs->InputActions.Action2);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_GamepadButtonY, 0, SampleInputActionBindingType_Released, &inputs->InputActions.Action3);
    
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseRightButton, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouse);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisXNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseLeft);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisXPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseRight);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisYNegative, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseUp);
    SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_MouseAxisYPositive, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateMouseDown);

    //SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyQ, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideLeft);
    //SampleRegisterInputActionBinding(&inputs->InputActionBindings, ElemInputId_KeyE, 0, SampleInputActionBindingType_Value, &inputs->InputActions.RotateSideRight);

    // TODO: Temporary Init code for now
    SampleCamera* camera = &(inputs->State.Camera);
    camera->Position = (ElemVector3) { 0.0f, 2.0f, -1.0f };
    camera->Rotation = (ElemVector3) { 0.0f, 1.2f, 0 };

    // TODO: Allow the reset of camera to the passed initial position when we press a button
}

ElemVector3 ComputeDeltaAndVelocity(ElemVector3* currentVelocity, ElemVector3 directionVector, float accelerationFactor, float frictionFactor, float deltaTimeInSeconds)
{
    ElemVector3 acceleration = SampleAddV3(SampleMulScalarV3(directionVector, accelerationFactor), SampleMulScalarV3(SampleInverseV3(*currentVelocity), frictionFactor));

    ElemVector3 delta = SampleAddV3(SampleMulScalarV3(acceleration, 0.5f * SamplePow2f(deltaTimeInSeconds)), SampleMulScalarV3(*currentVelocity, deltaTimeInSeconds));
    *currentVelocity = SampleAddV3(SampleMulScalarV3(acceleration, deltaTimeInSeconds), *currentVelocity);

    return delta;
}

SampleMatrix4x4 UpdateCamera(SampleCamera* camera, const SampleInputsCameraActions* inputActions, const ElemSwapChainUpdateParameters* updateParameters)
{
    ElemVector3 movementDirection = (ElemVector3) 
    { 
        .X = (inputActions->MoveRight - inputActions->MoveLeft),
        .Y = 0.0f,
        .Z = (inputActions->MoveForward - inputActions->MoveBackward)
    };
    
    ElemVector3 movementDelta = ComputeDeltaAndVelocity(&camera->PositionVelocity, movementDirection, 150.0f, 40.0f, updateParameters->DeltaTimeInSeconds);
    //printf("Current Pos Vel: %f %f %f\n", camera->PositionVelocity.X, camera->PositionVelocity.Y, camera->PositionVelocity.Z);

    ElemVector3 rotationDirection = (ElemVector3) 
    { 
        .X = (inputActions->RotateUp - inputActions->RotateDown),
        .Y = (inputActions->RotateLeft - inputActions->RotateRight),
        .Z = 0.0f,
    };

    // TODO: Prevent X Rotate above 180 deg
    ElemVector3 rotationDelta = ComputeDeltaAndVelocity(&camera->RotationVelocity, rotationDirection, 80.0f, 40.0f, updateParameters->DeltaTimeInSeconds);

    if (SampleMagnitudeSquaredV3(rotationDelta))
    {
        camera->Rotation = SampleAddV3(camera->Rotation, rotationDelta);
    }

    if (inputActions->RotateMouse)
    {
        ElemVector3 rotationMouseDelta = (ElemVector3) 
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
    SampleVector4 rotationQuaternion = SampleMulQuat(SampleCreateQuaternion((ElemVector3){ 1, 0, 0 }, camera->Rotation.X), 
                                                     SampleMulQuat(SampleCreateQuaternion((ElemVector3){ 0, 0, 1 }, camera->Rotation.Z),
                                                                   SampleCreateQuaternion((ElemVector3){ 0, 1, 0 }, camera->Rotation.Y)));

    if (SampleMagnitudeSquaredV3(movementDelta))
    {
        SampleMatrix4x4 translationTransform = SampleCreateTransformMatrix(rotationQuaternion, V3Zero);
        ElemVector3 rotatedMovementDelta = SampleTransformPointV3(movementDelta, translationTransform);
        camera->Position = SampleAddV3(camera->Position, rotatedMovementDelta);
    }

    SampleMatrix4x4 targetTransform = SampleCreateTransformMatrix(rotationQuaternion, V3Zero);
    ElemVector3 cameraTarget = SampleTransformPointV3((ElemVector3) { 0.0f, 0.0f, 1.0f }, targetTransform);
    cameraTarget = SampleAddV3(cameraTarget, camera->Position);

    return SampleCreateLookAtLHMatrix(camera->Position, cameraTarget, (ElemVector3) { 0.0f, 1.0f, 0.0f });
}

void SampleInputsCameraUpdate(ElemInputStream inputStream, SampleInputsCamera* inputs, const ElemSwapChainUpdateParameters* updateParameters)
{
    SampleInputsCameraState* state = &inputs->State;
    SampleInputsCameraActions* inputActions = &inputs->InputActions;

    SampleUpdateInputActions(&inputs->InputActionBindings, inputStream);

    // TODO: Change speed with a special "run" button
    // TODO: Select debug camera when needed 
    SampleCamera* currentCamera = &state->Camera;
    state->HasChanged = false;
    bool success = false;
    
    SampleMatrix4x4 viewMatrix = UpdateCamera(currentCamera, inputActions, updateParameters);

    for (uint32_t i = 0; i < 4; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            if (state->ViewMatrix.m[i][j] != viewMatrix.m[i][j])
            {
                state->HasChanged = true;
                break;
            }
        }
    }

    state->ViewMatrix = viewMatrix;

    if (updateParameters->SizeChanged || state->ProjectionMatrix.m[2][3] == 0.0f)
    {
        state->ProjectionMatrix = SampleCreatePerspectiveProjectionMatrix(0.78f, updateParameters->SwapChainInfo.AspectRatio, 0.001f);
        state->InverseProjectionMatrix = SampleInvertMatrix4x4(state->ProjectionMatrix, &success);
        assert(success);
        state->HasChanged = true;
    }
   
    if (state->Action != inputActions->Action)
    {
        state->Action = inputActions->Action;
        state->HasChanged = true;
    }

    if (state->Action2 != inputActions->Action2)
    {
        state->Action2 = inputActions->Action2;
        state->HasChanged = true;
    }

    if (state->Action3 != inputActions->Action3)
    {
        state->Action3 = inputActions->Action3;
        state->HasChanged = true;
    }

    if (inputActions->IncreaseCounter)
    {
        state->HasChanged = true;
        state->Counter = 1;
    }
    else if (inputActions->DecreaseCounter)
    {
        state->HasChanged = true;
        state->Counter = -1;
    }
    else
    {
        state->Counter = 0;
    }

    state->InverseViewMatrix = SampleInvertMatrix4x4(viewMatrix, &success);
    assert(success);

    if (state->HasChanged)
    {
        state->ViewProjMatrix = SampleMulMatrix4x4(viewMatrix, state->ProjectionMatrix);
    }
}
