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

#include "meshGenerationHelpers.h"
#include "chunkMap.h"

// Face definitions.
const FaceData faces[6] = {
  // TOP (+Y)
  {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {0, 1, 0}, {1, 1, 1}, {1, 1, 0}}, 1.0f},
  // BOTTOM (-Y)
  {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 0}, {1, 0, 1}, {0, 0, 1}}, 0.5f},
  // LEFT (-X)
  {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 0, 0}, {0, 1, 1}, {0, 1, 0}}, 0.7f},
  // RIGHT (+X)
  {{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 0}, {1, 1, 1}, {1, 0, 1}}, 0.75f},
  // FRONT (+Z)
  {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 0, 1}, {1, 1, 1}, {0, 1, 1}}, 0.75f},
  // BACK (-Z)
  {{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 0}, {1, 1, 0}, {1, 0, 0}}, 0.75f}};

Color ApplyShading(const Color base, const float factor)
{
  return (Color){(unsigned char)((float)base.r * factor),
                 (unsigned char)((float)base.g * factor),
                 (unsigned char)((float)base.b * factor), base.a};
}

const Color voxelColors[] = {
  {0, 0, 0, 0},        // AIR = 0
  {150, 75, 0, 255},   // DIRT = 1
  {46, 125, 50, 255},  // GRASS = 2
  {100, 100, 100, 255} // STONE = 3
};

bool IsFaceExposed(const Chunk* chunk, const int x, const int y, const int z,
                   const Face face)
{
  int neighborX = x + (face == RIGHT) - (face == LEFT);
  int neighborY = y + (face == TOP) - (face == BOTTOM);
  int neighborZ = z + (face == FRONT) - (face == BACK);

  if (neighborX >= 0 && neighborX < CHUNK_SIZE && neighborY >= 0 &&
      neighborY < CHUNK_SIZE && neighborZ >= 0 && neighborZ < CHUNK_SIZE)
  {
    // This should never ever happen, however it is sometimes happening
    // and I have no idea why. This is a temporary fix.
    if (chunk->voxels == NULL) { return true; }

    return chunk->voxels[VOXEL_INDEX(neighborX, neighborY, neighborZ)].type ==
           AIR;
  }

  const int chunkOffsetX = neighborX < 0 ? -1 : neighborX >= CHUNK_SIZE ? 1 : 0;
  const int chunkOffsetY = neighborY < 0 ? -1 : neighborY >= CHUNK_SIZE ? 1 : 0;
  const int chunkOffsetZ = neighborZ < 0 ? -1 : neighborZ >= CHUNK_SIZE ? 1 : 0;

  const Chunk* neighbor = GetChunkFromMap(chunk->position.x + chunkOffsetX,
                                          chunk->position.y + chunkOffsetY,
                                          chunk->position.z + chunkOffsetZ);
  if (!neighbor) return true;

  neighborX = (neighborX + CHUNK_SIZE) % CHUNK_SIZE;
  neighborY = (neighborY + CHUNK_SIZE) % CHUNK_SIZE;
  neighborZ = (neighborZ + CHUNK_SIZE) % CHUNK_SIZE;

  if (!neighbor->voxels) return true;
  return neighbor->voxels[VOXEL_INDEX(neighborX, neighborY, neighborZ)].type ==
         AIR;
}
