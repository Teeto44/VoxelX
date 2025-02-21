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
#include <stdlib.h>
#include "chunkMap.h"
#include "darray.h"
#include "gui.h"
#include "player.h"
#include "settings.h"
#include "threadPool.h"
#include "voxelTasks.h"

// Function prototypes
static void UpdateNeighboringChunkMeshes(int chunkX, int chunkY, int chunkZ);
static void CheckAndFreeEmptyChunk(Chunk* chunk);

Map* loadedChunks = NULL;
mtx_t loadedChunksMutex;
extern ThreadPool threadPool;

// Helpers
static void WorldToChunkCoords(const Vector3 pos, int* chunkX, int* chunkY,
                               int* chunkZ)
{
  *chunkX = (int)floorf(pos.x / CHUNK_SIZE);
  *chunkY = (int)floorf(pos.y / CHUNK_SIZE);
  *chunkZ = (int)floorf(pos.z / CHUNK_SIZE);
}

static void WorldToLocalCoords(const Vector3 pos, int* localX, int* localY,
                               int* localZ)
{
  *localX = ((int)floorf(pos.x) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  *localY = ((int)floorf(pos.y) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  *localZ = ((int)floorf(pos.z) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
}

// Function to place a block
void PlaceVoxel(const Vector3 position, const VoxelType type)
{
  int chunkX, chunkY, chunkZ;
  WorldToChunkCoords(position, &chunkX, &chunkY, &chunkZ);
  Chunk* chunk = GetChunkFromMap(chunkX, chunkY, chunkZ);
  if (!chunk)
  {
    TraceLog(LOG_ERROR, "Attempting to place voxel in non-existent chunk");
    return;
  }

  int localX, localY, localZ;
  WorldToLocalCoords(position, &localX, &localY, &localZ);
  if (!chunk->voxels)
  {
    chunk->voxels = calloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, sizeof(Voxel));
    if (!chunk->voxels)
    {
      TraceLog(LOG_ERROR, "Failed to allocate voxel data for chunk");
      return;
    }
  }
  chunk->voxels[VOXEL_INDEX(localX, localY, localZ)] = (Voxel){type};
  chunk->needsMeshing = true;
  // Mark neighbors as needing re-mesh in case their visible faces change
  UpdateNeighboringChunkMeshes(chunkX, chunkY, chunkZ);
}

// Function to break a voxel
void BreakVoxel(const Vector3 position)
{
  PlaceVoxel(position, AIR);

  int chunkX, chunkY, chunkZ;
  WorldToChunkCoords(position, &chunkX, &chunkY, &chunkZ);
  Chunk* chunk = GetChunkFromMap(chunkX, chunkY, chunkZ);
  if (chunk) CheckAndFreeEmptyChunk(chunk);
}

Voxel GetVoxel(const Vector3 position)
{
  int chunkX, chunkY, chunkZ;
  WorldToChunkCoords(position, &chunkX, &chunkY, &chunkZ);
  const Chunk* chunk = GetChunkFromMap(chunkX, chunkY, chunkZ);
  if (!chunk || !chunk->voxels) return (Voxel){AIR};
  int localX, localY, localZ;
  WorldToLocalCoords(position, &localX, &localY, &localZ);
  return chunk->voxels[VOXEL_INDEX(localX, localY, localZ)];
}

// Completely destroys the currently loaded chunks
void DestroyWorld() { ClearChunkMap(); }

Chunk* CreateChunk(const int chunkX, const int chunkY, const int chunkZ)
{
  Chunk* chunk = ChunkPoolAcquire();
  if (!chunk)
  {
    TraceLog(LOG_ERROR, "ChunkPoolAcquire failed");
    return NULL;
  }
  chunk->position.x = chunkX;
  chunk->position.y = chunkY;
  chunk->position.z = chunkZ;
  chunk->needsMeshing = true;
  chunk->voxels = NULL;

  ThreadPoolSubmit(&threadPool, ChunkGenerationTask, chunk);
  UpdateNeighboringChunkMeshes(chunkX, chunkY, chunkZ);

  return chunk;
}

void LoadChunksInRenderDistance(void)
{
  const Vector3I playerChunk = GetPlayerChunk();
  const int drawDistance = GetDrawDistance();
  const int drawDistanceSq = drawDistance * drawDistance;

  // Create any missing chunks in render radius
  for (int chunkX = playerChunk.x - drawDistance;
       chunkX <= playerChunk.x + drawDistance; chunkX++)
  {
    for (int chunkY = playerChunk.y - drawDistance;
         chunkY <= playerChunk.y + drawDistance; chunkY++)
    {
      for (int chunkZ = playerChunk.z - drawDistance;
           chunkZ <= playerChunk.z + drawDistance; chunkZ++)
      {
        const int distanceX = chunkX - playerChunk.x;
        const int distanceY = chunkY - playerChunk.y;
        const int distanceZ = chunkZ - playerChunk.z;
        const int distanceSq =
          distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ;
        if (distanceSq <= drawDistanceSq)
        {
          if (!GetChunkFromMap(chunkX, chunkY, chunkZ))
          {
            Chunk* newChunk = CreateChunk(chunkX, chunkY, chunkZ);
            if (newChunk) AddChunkToMap(chunkX, chunkY, chunkZ, newChunk);
          }
        }
      }
    }
  }

  // Determine which chunks should be removed or re-meshed
  DArray* chunksToRemove = DArrayCreate(sizeof(ChunkKey));
  if (!chunksToRemove)
  {
    TraceLog(LOG_ERROR, "Failed to create dynamic array for chunk removal");
    return;
  }

  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;
  while (MapIteratorNext(&it, &key, &chunk))
  {
    const int distanceX = key.chunkX - playerChunk.x;
    const int distanceY = key.chunkY - playerChunk.y;
    const int distanceZ = key.chunkZ - playerChunk.z;
    const int distanceSq =
      distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ;
    if (distanceSq > drawDistanceSq) { DArrayPush(chunksToRemove, &key); }
    else if (chunk->needsMeshing && chunk->voxels != NULL)
    {
      chunk->needsMeshing = false;
      ThreadPoolSubmit(&threadPool, ComputeMeshTask, chunk);
    }
  }

  // Remove out-of-range chunks
  for (size_t i = 0; i < DArraySize(chunksToRemove); i++)
  {
    ChunkKey removeKey;
    DArrayGet(chunksToRemove, i, &removeKey);
    RemoveChunkFromMap(removeKey.chunkX, removeKey.chunkY, removeKey.chunkZ);
  }
  DArrayFree(chunksToRemove);
}

// Draws all the currently loaded chunks
void DrawChunks(void)
{
  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;
  while (MapIteratorNext(&it, &key, &chunk))
  {
    if (chunk && chunk->model.meshCount > 0)
    {
      const Vector3 chunkPos = {(float)chunk->position.x * CHUNK_SIZE,
                                (float)chunk->position.y * CHUNK_SIZE,
                                (float)chunk->position.z * CHUNK_SIZE};
      if (GetDrawWireFrame())
        DrawModelWires(chunk->model, chunkPos, 1.0f, WHITE);
      else
        DrawModel(chunk->model, chunkPos, 1.0f, WHITE);
      if (GetDrawChunkBorders())
      {
        const BoundingBox bounds = {
          chunkPos,
          Vector3Add(chunkPos, (Vector3){CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE})};
        DrawBoundingBox(bounds, RED);
      }
    }
  }
}

static void UpdateNeighboringChunkMeshes(const int chunkX, const int chunkY,
                                         const int chunkZ)
{
  static const int offsets[6][3] = {{-1, 0, 0}, {1, 0, 0},  {0, -1, 0},
                                    {0, 1, 0},  {0, 0, -1}, {0, 0, 1}};

  for (int i = 0; i < 6; i++)
  {
    const int neighborX = chunkX + offsets[i][0];
    const int neighborY = chunkY + offsets[i][1];
    const int neighborZ = chunkZ + offsets[i][2];
    Chunk* neighborChunk = GetChunkFromMap(neighborX, neighborY, neighborZ);
    if (neighborChunk) { neighborChunk->needsMeshing = true; }
  }
}

static void CheckAndFreeEmptyChunk(Chunk* chunk)
{
  if (!chunk->voxels) return;

  const size_t totalVoxels = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
  for (size_t i = 0; i < totalVoxels; i++)
  {
    if (chunk->voxels[i].type != AIR) return;
  }

  if (chunk->model.meshCount > 0) { RemoveChunkModel(chunk); }
  TraceLog(LOG_INFO, "Freeing empty chunk at (%d, %d, %d)", chunk->position.x,
           chunk->position.y, chunk->position.z);
  free(chunk->voxels);
  chunk->voxels = NULL;
}
