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

#ifndef CHUNK_MAP_H
#define CHUNK_MAP_H

#include "dataTypes.h"
#include "map.h"

typedef struct ChunkKey
{
  int chunkX;
  int chunkY;
  int chunkZ;
} ChunkKey;

static size_t ChunkKeyHash(const void* key)
{
  const ChunkKey* chunkKey = key;
  const size_t hashX = MapHashInt(&chunkKey->chunkX);
  const size_t hashY = MapHashInt(&chunkKey->chunkY);
  const size_t hashZ = MapHashInt(&chunkKey->chunkZ);
  return hashX ^ hashY * 31 ^ hashZ * 31;
}

static bool ChunkKeyCompare(const void* key1, const void* key2)
{
  const ChunkKey* ck1 = key1;
  const ChunkKey* ck2 = key2;
  return ck1->chunkX == ck2->chunkX && ck1->chunkY == ck2->chunkY &&
         ck1->chunkZ == ck2->chunkZ;
}

// Global map instance for storing chunks
extern Map* loadedChunks;

// Initializes the chunk map
static void InitializeChunkMap()
{
  loadedChunks =
    MapCreate(sizeof(ChunkKey), sizeof(Chunk*), ChunkKeyHash, ChunkKeyCompare);
}

inline void AddChunkToMap(const int chunkX, const int chunkY, const int chunkZ,
                          Chunk* chunk)
{
  if (chunk == NULL)
  {
    TraceLog(LOG_ERROR, "Attempting to add non-existent chunk to chunk map");
    return;
  }
  if (!loadedChunks) InitializeChunkMap();
  const ChunkKey key = {.chunkX = chunkX, .chunkY = chunkY, .chunkZ = chunkZ};
  MapPut(loadedChunks, &key, &chunk);
}

inline Chunk* GetChunkFromMap(const int chunkX, const int chunkY,
                              const int chunkZ)
{
  if (!loadedChunks)
  {
    TraceLog(LOG_INFO,
             "Attempting to retrieve chunk from uninitialized map, If this "
             "flagged after the first few frames, it is an issue");
    return NULL;
  }
  const ChunkKey key = {.chunkX = chunkX, .chunkY = chunkY, .chunkZ = chunkZ};
  Chunk* output = NULL;
  if (MapGet(loadedChunks, &key, &output)) return output;
  return NULL;
}

inline void RemoveChunkFromMap(const int chunkX, const int chunkY,
                               const int chunkZ)
{
  if (!loadedChunks)
  {
    TraceLog(LOG_ERROR, "Attempting to remove chunk from uninitialized map");
    return;
  }
  const ChunkKey key = {.chunkX = chunkX, .chunkY = chunkY, .chunkZ = chunkZ};
  Chunk* chunk = NULL;
  if (MapGet(loadedChunks, &key, &chunk))
  {
    if (chunk->model.meshCount > 0) { REMOVE_CHUNK_MODEL(chunk) }
    free(chunk);
    MapRemove(loadedChunks, &key);
  }
}

// Clears the chunk map
inline void ClearChunkMap()
{
  if (!loadedChunks)
  {
    TraceLog(LOG_ERROR, "Attempting to clear uninitialized chunk map");
    return;
  }

  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;

  while (MapIteratorNext(&it, &key, &chunk))
  {
    if (chunk->model.meshCount > 0) { REMOVE_CHUNK_MODEL(chunk); }
    free(chunk);
  }

  MapFree(loadedChunks);
  loadedChunks = NULL;
}

#endif // CHUNK_MAP_H
