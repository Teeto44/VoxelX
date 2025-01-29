/*******************************************************************************
* VoxelX
*
* The MIT License (MIT)
* Copyright (c) 2025 Tyson Thigpen
*
* Permission is hereby granted, free of charge, to any person obtaining a copy  
* of this software and associated documentation files (the "Software"), to    
* deal in the Software without restriction, including without limitation the    
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or   
* sell copies of the Software, and to permit persons to whom the Software is    
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,      
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE   
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN     
* THE SOFTWARE.
*******************************************************************************/

#include "player.h"
#include "settings.h"
#include "raymath.h"

Vector3 position;
int playerSpeed;

// Player objects
Camera3D playerCamera;

// Variable fetching
Camera3D GetPlayerCamera() { return playerCamera; }
Vector3 GetPlayerPosition() { return position; }

void InitPlayer()
{
	// Initialize player variables
	position = (Vector3){ -3.0f, 2.0f, 0.0f };
	playerSpeed = PLAYER_SPEEED;

	// Initialize camera variables
	playerCamera.position = position;
	playerCamera.target = (Vector3){ position.x + 1.0f, position.y, position.z };
	playerCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	playerCamera.fovy = PLAYER_FOV;
	playerCamera.projection = CAMERA_PERSPECTIVE;
}

void UpdatePlayer(float deltaTime)
{
	Vector3 movement = GetMovement(deltaTime);
  Vector3 deltaRotation = GetMouseMovement(deltaTime);

	// Rework movement for raylib's camera update function
	Vector3 reworkedMovement = (Vector3) { movement.x, movement.z, movement.y };

	UpdateCameraPro(&playerCamera, reworkedMovement, deltaRotation, 0.0f);

	position = playerCamera.position;
}

// Get change in movement since last frame
Vector3 GetMovement(float deltaTime)
{
	// Adjusts movement based on time between frames
	float movementMagnitude = playerSpeed * deltaTime;

	return (Vector3)
	{
		(IsKeyDown(PLAYER_FORWARD) - IsKeyDown(PLAYER_BACK)) * movementMagnitude, 
		(IsKeyDown(PLAYER_UP) - IsKeyDown(PLAYER_DOWN)) * movementMagnitude,
    (IsKeyDown(PLAYER_RIGHT) - IsKeyDown(PLAYER_LEFT)) * movementMagnitude  
	};
}

// Get change in rotation since last frame
Vector3 GetMouseMovement(float deltaTime)
{
	// Dont rotate if cursor isnt active
	if (!IsCursorHidden()) { return Vector3Zero(); };

	Vector2 mouseMovement = GetMouseDelta();

	return (Vector3)
	{
		mouseMovement.x * MOUSE_SENSITIVITY * deltaTime,
		mouseMovement.y * MOUSE_SENSITIVITY * deltaTime,
		0
	};
}
