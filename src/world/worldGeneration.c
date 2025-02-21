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
#include <stdlib.h>
#include "dataTypes.h"
#include "settings.h"

static float PerlinNoise2D(float x, float y);

void GenerateChunk(Chunk* chunk)
{
  if (chunk == NULL)
  {
    TraceLog(LOG_ERROR, "World generation received non-existent chunk");
    return;
  }

  // Allocate a temporary buffer for voxel data.
  const size_t totalVoxels = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
  Voxel* voxelBuffer = calloc(totalVoxels, sizeof(Voxel));
  if (!voxelBuffer)
  {
    TraceLog(LOG_ERROR, "Failed to allocate temporary voxel buffer for chunk");
    return;
  }

  bool nonAirFound = false;

  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int z = 0; z < CHUNK_SIZE; z++)
    {
      // Use Perlin noise to generate a smooth height map.
      const float noise =
        PerlinNoise2D((float)(chunk->position.x * CHUNK_SIZE + x) * 0.1f,
                      (float)(chunk->position.z * CHUNK_SIZE + z) * 0.1f);
      const int height = (int)(noise * 10.0f) + 10;

      for (int y = 0; y < CHUNK_SIZE; y++)
      {
        const int globalY = chunk->position.y * CHUNK_SIZE + y;
        VoxelType voxelType = AIR;

        if (globalY >= 0)
        {
          if (globalY < height - 1)
            voxelType = STONE;
          else if (globalY < height)
            voxelType = DIRT;
          else if (globalY == height)
            voxelType = GRASS;
        }

        voxelBuffer[VOXEL_INDEX(x, y, z)].type = voxelType;
        if (voxelType != AIR) nonAirFound = true;
      }
    }
  }

  // If any non-air voxel exists, move the temporary buffer into the chunk.
  // Otherwise, free the buffer and leave chunk->voxels as NULL.
  if (nonAirFound)
    chunk->voxels = voxelBuffer;
  else
  {
    free(voxelBuffer);
    chunk->voxels = NULL;
  }
}

static float PerlinNoise2D(const float x, const float y)
{
  return (sinf(x) + cosf(y)) * 0.5f;
}
