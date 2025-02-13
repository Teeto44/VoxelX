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

#include "world.h"
#include <math.h>
#include <stdlib.h>
#include "chunkMap.h"
#include "chunkMeshGeneration.h"
#include "gui.h"
#include "player.h"
#include "settings.h"
#include "worldGeneration.h"

// Function prototypes
static void DrawChunk(const Chunk* chunk);
static void UpdateNeighboringChunkMeshes(int chunkX, int chunkZ);

Map* loadedChunks = NULL;

// Completely destroys the currently loaded chunks
void DestroyWorld() { ClearChunkMap(); }

// TODO - have random mesh generation, for now it simply fills the chunk
Chunk* CreateChunk(const int chunkX, const int chunkZ)
{
  Chunk* chunk = malloc(sizeof(Chunk));
  chunk->position.x = chunkX;
  chunk->position.y = 0;
  chunk->position.z = chunkZ;
  chunk->mesh = NULL;
  chunk->needsMeshing = true;

  GenerateChunk(chunk);

  // Update neighboring chunk meshes
  UpdateNeighboringChunkMeshes(chunkX, chunkZ);

  return chunk;
}

void LoadChunksInRenderDistance()
{
  // Determine which chunk the player is in
  const int playerChunkX = (int)floorf(GetPlayerPosition().x / CHUNK_SIZE);
  const int playerChunkZ = (int)floorf(GetPlayerPosition().z / CHUNK_SIZE);

  // Check if all chunks in the player's circular render distance are loaded
  for (int chunkX = playerChunkX - RENDER_DISTANCE;
       chunkX <= playerChunkX + RENDER_DISTANCE; chunkX++)
  {
    for (int chunkZ = playerChunkZ - RENDER_DISTANCE;
         chunkZ <= playerChunkZ + RENDER_DISTANCE; chunkZ++)
    {
      // Only load chunks within a circular distance from the player
      const float distance =
        sqrtf((float)(chunkX - playerChunkX) * (float)(chunkX - playerChunkX) +
              (float)(chunkZ - playerChunkZ) * (float)(chunkZ - playerChunkZ));
      if (distance <= RENDER_DISTANCE)
      {
        if (!GetChunkFromMap(chunkX, chunkZ))
        {
          Chunk* newChunk = CreateChunk(chunkX, chunkZ);
          AddChunkToMap(chunkX, chunkZ, newChunk);
        }
      }
    }
  }

  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;
  while (MapIteratorNext(&it, &key, &chunk))
  {
    const float distance = sqrtf(
      (float)(key.chunkX - playerChunkX) * (float)(key.chunkX - playerChunkX) +
      (float)(key.chunkZ - playerChunkZ) * (float)(key.chunkZ - playerChunkZ));

    // Unload chunks outside the render distance
    if (distance > RENDER_DISTANCE)
    {
      RemoveChunkFromMap(key.chunkX, key.chunkZ);
      continue;
    }

    // Re-mesh any chunks that's mesh has been updated
    if (chunk->needsMeshing)
    {
      GenerateChunkMesh(chunk);
      chunk->needsMeshing = false;
    }
  }
}

// Draws all the currently loaded chunks
void DrawChunks()
{
  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;

  while (MapIteratorNext(&it, &key, &chunk))
  {
    DrawChunk(chunk);
  }
}

// Draws a specific chunk
static void DrawChunk(const Chunk* chunk)
{
  if (!chunk->mesh) { return; }
  const Vector3 chunkPosition = {(float)chunk->position.x * CHUNK_SIZE,
                                 (float)chunk->position.y * CHUNK_SIZE,
                                 (float)chunk->position.z * CHUNK_SIZE};

  if (GetDrawWireFrame())
  {
    DrawModelWires(chunk->model, chunkPosition, 1.0f, WHITE);
  }
  else { DrawModel(chunk->model, chunkPosition, 1.0f, WHITE); }
}

void UpdateNeighboringChunkMeshes(const int chunkX, const int chunkZ)
{
  static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

  for (int i = 0; i < 4; i++)
  {
    const int neighborX = chunkX + offsets[i][0];
    const int neighborZ = chunkZ + offsets[i][1];
    Chunk* neighborChunk = GetChunkFromMap(neighborX, neighborZ);
    if (neighborChunk) { neighborChunk->needsMeshing = true; }
  }
}
