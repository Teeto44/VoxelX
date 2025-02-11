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

#define  CIMGUI_DEFINE_ENUMS_AND_STRUCTS

#include "gui.h"

#include <corecrt_math.h>

#include "rlImGui.h"
#include "cimgui.h"
#include "raylib.h"
#include "settings.h"
#include "player.h"
#include "world.h"

bool drawWireFrame = false;

// Variable Fetching
bool GetDrawWireFrame() { return drawWireFrame; }

void InitGui()
{
  // Setup ImGui
  rlImGuiSetup(true);

  // Configure ImGui to not handle the cursor, and not to save any gui settings
  ImGuiIO* io = igGetIO();
  io->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
  io->IniFilename = NULL;
}

void DrawDebugGui()
{
  rlImGuiBegin();

  igSeparatorText("Window Stats");
  igText("FPS %f", 1 / GetFrameTime());
  igText("Target FPS %d", TARGET_FPS);
  igText("Window Size %d, %d", GetScreenWidth(), GetScreenHeight());

  igSeparatorText("Game Stats");

  const Vector3 playerPosition = GetPlayerPosition();
  igText("Player Position %f, %f, %f", playerPosition.x, playerPosition.y,
         playerPosition.z);

  // Determine which chunk the player is in
  const int playerChunkX = (int)floorf(GetPlayerPosition().x / CHUNK_SIZE);
  const int playerChunkZ = (int)floorf(GetPlayerPosition().z / CHUNK_SIZE);
  igText("Player Chunk Position %d, %d, %d", playerChunkX, 0, playerChunkZ);

  igSeparatorText("Debug Options");
  igCheckbox("Wireframe", &drawWireFrame);

  if (igButton("Regenerate Chunks", (ImVec2){150, 20})) { DestroyWorld(); }

  rlImGuiEnd();
}

// De-initialization
void EndGui() { rlImGuiShutdown(); }

// Toggle the cursor visibility
void ToggleCursor()
{
  if (IsCursorHidden()) EnableCursor();
  else DisableCursor();
}
