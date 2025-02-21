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

#ifndef MESH_GENERATION_HELPERS_H
#define MESH_GENERATION_HELPERS_H

#include "dataTypes.h"
#include "raylib.h"

typedef struct Vertex
{
  Vector3 position;
  Color color;
} Vertex;

typedef struct
{
  Vector3 vertices[6];
  float shadeFactor;
} FaceData;

// Extern declaration for face data array.
extern const FaceData faces[6];
extern const Color voxelColors[];

Color ApplyShading(Color base, float factor);
bool IsFaceExposed(const Chunk* chunk, int x, int y, int z, Face face);

#endif // MESH_GENERATION_HELPERS_H
