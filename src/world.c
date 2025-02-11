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
#include "chunkMeshGeneration.h"
#include "settings.h"
#include "gui.h"
#include <stdlib.h>
#include "chunkMap.h"
#include "player.h"

// Function prototypes
static void DrawChunk(const Chunk* chunk);
static void GenerateChunk(Chunk* chunk);

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

  GenerateChunk(chunk);
  GenerateChunkMesh(chunk);

  chunk->model = LoadModelFromMesh(*chunk->mesh);

  return chunk;
}

// For now this simply fills the chunk with dirt, eventually this function
// will be responsible for generating the random chunk
static void GenerateChunk(Chunk* chunk)
{
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      for (int z = 0; z < CHUNK_SIZE; z++)
      {
        chunk->voxels[x][y][z].type = DIRT;
      }
    }
  }
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
      const float distance = sqrtf(
        (float)(chunkX - playerChunkX) * (float)(chunkX - playerChunkX) + (
          float)(chunkZ - playerChunkZ) * (float)(chunkZ - playerChunkZ));
      if (distance <= RENDER_DISTANCE)
      {
        // Check if the chunk is in the chunk map
        if (!GetChunkFromMap(chunkX, chunkZ))
        {
          Chunk* newChunk = CreateChunk(chunkX, chunkZ);
          AddChunkToMap(chunkX, chunkZ, newChunk);
        }
      }
    }
  }

  // Unload chunks outside the render distance
  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;

  while (MapIteratorNext(&it, &key, &chunk))
  {
    const float distance = sqrtf(
      (float)(key.chunkX - playerChunkX) * (float)(key.chunkX - playerChunkX) +
      (float)(key.chunkZ - playerChunkZ) * (float)(key.chunkZ - playerChunkZ));
    if (distance > RENDER_DISTANCE)
    {
      RemoveChunkFromMap(key.chunkX, key.chunkZ);
    }
  }
}

// Draws all the currently loaded chunks
void DrawChunks()
{
  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;

  while (MapIteratorNext(&it, &key, &chunk)) { DrawChunk(chunk); }
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
