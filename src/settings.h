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

#ifndef SETTINGS_H
#define SETTINGS_H

// Window settings
#define SCREEN_WIDTH (1920)
#define SCREEN_HEIGHT (1080)
#define TARGET_FPS (240)
#define WINDOW_TITLE "VoxelX - In Development"

// Player settings
#define PLAYER_SPEEED (50)
#define MOUSE_SENSITIVITY (50)
#define PLAYER_FOV (90)

// Player controls
#define FREE_MOUSE (KEY_F)                // Only works as a keyboard key
#define PLAYER_FORWARD (KEY_W)            // Only works as a keyboard key
#define PLAYER_BACK (KEY_S)               // Only works as a keyboard key
#define PLAYER_LEFT (KEY_A)               // Only works as a keyboard key
#define PLAYER_RIGHT (KEY_D)              // Only works as a keyboard key
#define PLAYER_UP (KEY_SPACE)             // Only works as a keyboard key
#define PLAYER_DOWN (KEY_LEFT_SHIFT)      // Only works as a keyboard key
#define PLAYER_BREAK (MOUSE_BUTTON_LEFT)  // Only works as a mouse button
#define PLAYER_PLACE (MOUSE_BUTTON_RIGHT) // Only works as a mouse button

// World settings
#define CHUNK_SIZE (16)
#define DEFAULT_DRAW_DISTANCE (10)

#endif // SETTINGS_H
