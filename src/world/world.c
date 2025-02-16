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
#include "chunkMeshGeneration.h"
#include "darray.h"
#include "gui.h"
#include "player.h"
#include "settings.h"
#include "worldGeneration.h"

// Function prototypes
static void DrawChunk(const Chunk* chunk);
static void UpdateNeighboringChunkMeshes(int chunkX, int chunkY, int chunkZ);

Map* loadedChunks = NULL;

// Function to place a block
void PlaceVoxel(const Vector3 position, const VoxelType type)
{
  const int chunkX = (int)floorf(position.x / CHUNK_SIZE);
  const int chunkY = (int)floorf(position.y / CHUNK_SIZE);
  const int chunkZ = (int)floorf(position.z / CHUNK_SIZE);

  Chunk* chunk = GetChunkFromMap(chunkX, chunkY, chunkZ);
  if (chunk == NULL)
  {
    TraceLog(LOG_ERROR, "Attempting to place voxel from non-existent chunk");
    return;
  }
  const int localX =
    ((int)floorf(position.x) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  const int localY =
    ((int)floorf(position.y) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  const int localZ =
    ((int)floorf(position.z) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

  chunk->voxels[localX][localY][localZ].type = type;
  chunk->needsMeshing = true;
  UpdateNeighboringChunkMeshes(chunkX, chunkY, chunkZ);
}

// Function to break a voxel
void BreakVoxel(const Vector3 position) { PlaceVoxel(position, AIR); }

Voxel GetVoxel(const Vector3 position)
{
  const int chunkX = (int)floorf(position.x / CHUNK_SIZE);
  const int chunkY = (int)floorf(position.y / CHUNK_SIZE);
  const int chunkZ = (int)floorf(position.z / CHUNK_SIZE);

  const Chunk* chunk = GetChunkFromMap(chunkX, chunkY, chunkZ);
  if (chunk == NULL || chunk->voxels == NULL)
  {
    TraceLog(
      LOG_ERROR,
      "Attempting to retrieve voxel from non-existent or uninitialized chunk");
    return (Voxel){AIR};
  }

  const int localX =
    ((int)floorf(position.x) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  const int localY =
    ((int)floorf(position.y) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  const int localZ =
    ((int)floorf(position.z) % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

  if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 ||
      localY >= CHUNK_SIZE || localZ < 0 || localZ >= CHUNK_SIZE)
  {
    TraceLog(LOG_ERROR,
             "Attempting to retrieve voxel from out-of-bounds position");
    return (Voxel){AIR};
  }

  return chunk->voxels[localX][localY][localZ];
}

// Completely destroys the currently loaded chunks
void DestroyWorld() { ClearChunkMap(); }

Chunk* CreateChunk(const int chunkX, const int chunkY, const int chunkZ)
{
  Chunk* chunk = malloc(sizeof(Chunk));
  chunk->position.x = chunkX;
  chunk->position.y = chunkY;
  chunk->position.z = chunkZ;
  chunk->needsMeshing = true;

  GenerateChunk(chunk);
  UpdateNeighboringChunkMeshes(chunkX, chunkY, chunkZ);

  return chunk;
}

void LoadChunksInRenderDistance()
{
  const Vector3I playerChunk = GetPlayerChunk();
  const int drawDistance = GetDrawDistance();

  for (int chunkX = playerChunk.x - drawDistance;
       chunkX <= playerChunk.x + drawDistance; chunkX++)
  {
    for (int chunkY = playerChunk.y - drawDistance;
         chunkY <= playerChunk.y + drawDistance; chunkY++)
    {
      for (int chunkZ = playerChunk.z - drawDistance;
           chunkZ <= playerChunk.z + drawDistance; chunkZ++)
      {
        const float distance = sqrtf(
          (float)(chunkX - playerChunk.x) * (float)(chunkX - playerChunk.x) +
          (float)(chunkY - playerChunk.y) * (float)(chunkY - playerChunk.y) +
          (float)(chunkZ - playerChunk.z) * (float)(chunkZ - playerChunk.z));
        if (distance <= (float)drawDistance)
        {
          if (!GetChunkFromMap(chunkX, chunkY, chunkZ))
          {
            Chunk* newChunk = CreateChunk(chunkX, chunkY, chunkZ);
            AddChunkToMap(chunkX, chunkY, chunkZ, newChunk);
          }
        }
      }
    }
  }

  DArray* chunksToRemove = DArrayCreate(sizeof(ChunkKey));

  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;
  while (MapIteratorNext(&it, &key, &chunk))
  {
    const float distance = sqrtf((float)pow(key.chunkX - playerChunk.x, 2) +
                                 (float)pow(key.chunkY - playerChunk.y, 2) +
                                 (float)pow(key.chunkZ - playerChunk.z, 2));
    if (distance > (float)drawDistance) { DArrayPush(chunksToRemove, &key); }
    else if (chunk->needsMeshing)
    {
      GenerateChunkMesh(chunk);
      chunk->needsMeshing = false;
    }
  }

  // Remove chunks after iteration is complete
  for (size_t i = 0; i < DArraySize(chunksToRemove); i++)
  {
    ChunkKey removeKey;
    DArrayGet(chunksToRemove, i, &removeKey);
    RemoveChunkFromMap(removeKey.chunkX, removeKey.chunkY, removeKey.chunkZ);
  }
  DArrayFree(chunksToRemove);
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
  // Why check if it is less than 0? Because for some reason models instantiate
  // with -842150451 meshes. Explain that to me.
  if (chunk->model.meshCount <= 0) { return; }

  const Vector3 chunkPosition = {(float)chunk->position.x * CHUNK_SIZE,
                                 (float)chunk->position.y * CHUNK_SIZE,
                                 (float)chunk->position.z * CHUNK_SIZE};

  if (GetDrawWireFrame())
  {
    DrawModelWires(chunk->model, chunkPosition, 1.0f, WHITE);
  }
  else { DrawModel(chunk->model, chunkPosition, 1.0f, WHITE); }

  if (GetDrawChunkBorders())
  {
    const BoundingBox chunkBounds = {
      .min = chunkPosition,
      .max = Vector3Add(chunkPosition,
                        (Vector3){CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE})};
    DrawBoundingBox(chunkBounds, RED);
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
