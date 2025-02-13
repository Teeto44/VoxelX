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

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdbool.h>
#include "raylib.h"

#include "settings.h"

#define uint uint32_t
#define ushort uint16_t

// Essential datatypes
typedef struct Vector3I
{
  int x;
  int y;
  int z;
} Vector3I;

typedef enum VoxelType
{
  AIR,
  DIRT,
  GRASS,
  STONE,
} VoxelType;

typedef enum Face
{
  TOP,
  BOTTOM,
  LEFT,
  RIGHT,
  FRONT,
  BACK,
} Face;

typedef struct Voxel
{
  VoxelType type;
} Voxel;

typedef struct Chunk
{
  Vector3I position;
  Voxel voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
  bool needsMeshing;
  Mesh* mesh;
  Model model;
} Chunk;

// Smaller ways to refer to the hashing functions
#define MHVI MapHashVector3I
#define MCV3 MapCompareVector3I

static size_t MapHashVector3I(const void* key)
{
  const Vector3I* vec = key;

  // Simple but effective hash combining x,y,z
  size_t hash = 5381;
  hash = (hash << 5) + hash + vec->x;
  hash = (hash << 5) + hash + vec->y;
  hash = (hash << 5) + hash + vec->z;
  return hash;
}

static bool MapCompareVector3I(const void* key1, const void* key2)
{
  const Vector3I* vec1 = key1;
  const Vector3I* vec2 = key2;
  return vec1->x == vec2->x && vec1->y == vec2->y && vec1->z == vec2->z;
}

#endif // DATA_TYPES_H
