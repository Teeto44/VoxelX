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

#include "engine.h"
#include "raylib.h"
#include "settings.h"
#include "player.h"
#include "gui.h"

// Initialize the engine
void Initialize()
{
	// Intialize window
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(TARGET_FPS);
	DisableCursor();	

	InitGUI();
	InitPlayer();	
}

void Update()
{
	// Update world
	UpdatePlayer(GetFrameTime());

	// Todo - Move this to an actual input handler file
	if (IsKeyPressed(KEY_F)) ToggleCursor();

	Draw();
}

// Draw the frame
void Draw()
{
	// Begin drawing
	BeginDrawing();
	ClearBackground(BLACK);

	Draw3D();
	Draw2D();
	
	// End drawing
	EndDrawing();
}

// Draw 3D elements
void Draw3D()
{
	// Begins drawing 3D from player camera
	BeginMode3D(GetPlayerCamera());

	// Temporary debug grid
	DrawGrid(10, 1.0f);

	// End of drawing 3D
	EndMode3D();
}

// Draw 2D elements
void Draw2D()
{
	// Temporary debug GUI
	DrawDebugGUI();
}

// Deconstruct the engine
void Deconstruct()
{
	// Cleaning up GUI
	EndGUI();

	// Cleaning up window
	CloseWindow();
}

