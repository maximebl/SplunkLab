//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DirectXMath.h>
#include "SimpleCamera.h"

SimpleCamera::SimpleCamera():
    InitialPosition(0, 0, 0),
    Position(InitialPosition),
    Yaw(XM_PI),
    Pitch(0.0f),
    LookDirection(0, 0, -1),
    UpDirection(0, 1, 0),
    MoveSpeed(20.0f),
    TurnSpeed(XM_PIDIV2),
    KeysPressed{}
{
}

void SimpleCamera::Init(XMFLOAT3 position)
{
    InitialPosition = position;
    Reset();
}

void SimpleCamera::SetMoveSpeed(float unitsPerSecond)
{
    MoveSpeed = unitsPerSecond;
}

void SimpleCamera::SetTurnSpeed(float radiansPerSecond)
{
    TurnSpeed = radiansPerSecond;
}

void SimpleCamera::Reset()
{
    Position = InitialPosition;
    Yaw = XM_PI;
    Pitch = 0.0f;
    LookDirection = { 0, 0, -1 };
}

void SimpleCamera::Update(float elapsedSeconds)
{
    // Calculate the move vector in camera space.
    XMFLOAT3 move(0, 0, 0);

    if (KeysPressed.a)
        move.x -= 1.0f;
    if (KeysPressed.d)
        move.x += 1.0f;
    if (KeysPressed.w)
        move.z -= 1.0f;
    if (KeysPressed.s)
        move.z += 1.0f;

    if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f)
    {
        XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
        move.x = XMVectorGetX(vector);
        move.z = XMVectorGetZ(vector);
    }

    float moveInterval = MoveSpeed * elapsedSeconds;
    float rotateInterval = TurnSpeed * elapsedSeconds;

    if (KeysPressed.left)
        Yaw += rotateInterval;
    if (KeysPressed.right)
        Yaw -= rotateInterval;
    if (KeysPressed.up)
        Pitch += rotateInterval;
    if (KeysPressed.down)
        Pitch -= rotateInterval;

    // Prevent looking too far up or down.
    Pitch = min(Pitch, XM_PIDIV4);
    Pitch = max(-XM_PIDIV4, Pitch);

    // Move the camera in model space.
    float x = move.x * -cosf(Yaw) - move.z * sinf(Yaw);
    float z = move.x * sinf(Yaw) - move.z * cosf(Yaw);
    Position.x += x * moveInterval;
    Position.z += z * moveInterval;

    // Determine the look direction.
    float r = cosf(Pitch);
    LookDirection.x = r * sinf(Yaw);
    LookDirection.y = sinf(Pitch);
    LookDirection.z = r * cosf(Yaw);
}

XMMATRIX SimpleCamera::GetViewMatrix()
{
    return XMMatrixLookToRH(XMLoadFloat3(&Position), XMLoadFloat3(&LookDirection), XMLoadFloat3(&UpDirection));
}

XMMATRIX SimpleCamera::GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    return XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane);
}

void SimpleCamera::OnKeyDown(WPARAM key)
{
    switch (key)
    {
    case 'W':
        KeysPressed.w = true;
        break;
    case 'A':
        KeysPressed.a = true;
        break;
    case 'S':
        KeysPressed.s = true;
        break;
    case 'D':
        KeysPressed.d = true;
        break;
    case VK_LEFT:
        KeysPressed.left = true;
        break;
    case VK_RIGHT:
        KeysPressed.right = true;
        break;
    case VK_UP:
        KeysPressed.up = true;
        break;
    case VK_DOWN:
        KeysPressed.down = true;
        break;
    case VK_ESCAPE:
        Reset();
        break;
    }
}

void SimpleCamera::OnKeyUp(WPARAM key)
{
    switch (key)
    {
    case 'W':
        KeysPressed.w = false;
        break;
    case 'A':
        KeysPressed.a = false;
        break;
    case 'S':
        KeysPressed.s = false;
        break;
    case 'D':
        KeysPressed.d = false;
        break;
    case VK_LEFT:
        KeysPressed.left = false;
        break;
    case VK_RIGHT:
        KeysPressed.right = false;
        break;
    case VK_UP:
        KeysPressed.up = false;
        break;
    case VK_DOWN:
        KeysPressed.down = false;
        break;
    }
}
