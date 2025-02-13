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

#include "worldGeneration.h"
#include <math.h>
#include "dataTypes.h"
#include "settings.h"

static float PerlinNoise2D(float x, float y);

void GenerateChunk(Chunk* chunk)
{
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int z = 0; z < CHUNK_SIZE; z++)
    {
      // Use Perlin noise to generate a smooth height map
      const float noise =
        PerlinNoise2D((float)(chunk->position.x * CHUNK_SIZE + x) * 0.1f,
                      (float)(chunk->position.z * CHUNK_SIZE + z) * 0.1f);
      const int height = (int)(noise * 10.0f) + 10;

      for (int y = 0; y < CHUNK_SIZE; y++)
      {
        if (y < height - 1) { chunk->voxels[x][y][z].type = STONE; }
        else if (y < height) { chunk->voxels[x][y][z].type = DIRT; }
        else if (y == height) { chunk->voxels[x][y][z].type = GRASS; }
        else { chunk->voxels[x][y][z].type = AIR; }
      }
    }
  }
}

// Temporarily generates smooth curves for the surface of the world.
// This will be replaced with a more complex world generation algorithm,
// but I think this is good enough for now.
static float PerlinNoise2D(const float x, const float y)
{
  return (sinf(x) + cosf(y)) * 0.5f;
}
