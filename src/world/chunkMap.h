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

#include "chunkPool.h"
#include "dataTypes.h"
#include "map.h"
#include "tinycthreadWrapper.h"

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
  const ChunkKey* chunkKey = key1;
  const ChunkKey* chunkKey2 = key2;
  return chunkKey->chunkX == chunkKey2->chunkX &&
         chunkKey->chunkY == chunkKey2->chunkY &&
         chunkKey->chunkZ == chunkKey2->chunkZ;
}

// Global map instance for storing chunks
extern Map* loadedChunks;
extern mtx_t loadedChunksMutex;

// Initializes the chunk map
static void InitializeChunkMap()
{
  TraceLog(LOG_ERROR, "1");
  mtx_lock(&loadedChunksMutex);
  TraceLog(LOG_ERROR, "2");
  if (!loadedChunks)
  {
    TraceLog(LOG_ERROR, "3");
    loadedChunks = MapCreate(sizeof(ChunkKey), sizeof(Chunk*), ChunkKeyHash,
                             ChunkKeyCompare);
  }
  TraceLog(LOG_ERROR, "4");
  mtx_unlock(&loadedChunksMutex);
}

static void AddChunkToMap(const int chunkX, const int chunkY, const int chunkZ,
                          Chunk* chunk)
{
  if (!chunk)
  {
    TraceLog(LOG_ERROR, "Attempting to add non-existent chunk to chunk map");
    return;
  }
  mtx_lock(&loadedChunksMutex);
  if (!loadedChunks) InitializeChunkMap();
  const ChunkKey key = {chunkX, chunkY, chunkZ};
  MapPut(loadedChunks, &key, &chunk);
  mtx_unlock(&loadedChunksMutex);
}

static Chunk* GetChunkFromMap(const int chunkX, const int chunkY,
                              const int chunkZ)
{
  mtx_lock(&loadedChunksMutex);
  if (!loadedChunks)
  {
    TraceLog(LOG_INFO, "Attempting to retrieve chunk from uninitialized map, "
                       "if flagged after first few frames, it is an issue");
    mtx_unlock(&loadedChunksMutex);
    return NULL;
  }
  const ChunkKey key = {chunkX, chunkY, chunkZ};
  Chunk* chunk = NULL;
  const bool found = MapGet(loadedChunks, &key, &chunk);
  mtx_unlock(&loadedChunksMutex);
  return found ? chunk : NULL;
}

static void RemoveChunkFromMap(const int chunkX, const int chunkY,
                               const int chunkZ)
{
  mtx_lock(&loadedChunksMutex);
  if (!loadedChunks)
  {
    TraceLog(LOG_ERROR, "Attempting to remove chunk from uninitialized map");
    mtx_unlock(&loadedChunksMutex);
    return;
  }
  const ChunkKey key = {chunkX, chunkY, chunkZ};
  Chunk* chunk = NULL;
  if (MapGet(loadedChunks, &key, &chunk))
  {
    if (chunk->model.meshCount > 0) { RemoveChunkModel(chunk); }
    ChunkPoolRelease(chunk);
    MapRemove(loadedChunks, &key);
  }
  mtx_unlock(&loadedChunksMutex);
}

// Clears the chunk map
static void ClearChunkMap()
{
  mtx_lock(&loadedChunksMutex);
  if (!loadedChunks)
  {
    TraceLog(LOG_ERROR, "Attempting to clear uninitialized chunk map");
    mtx_unlock(&loadedChunksMutex);
    return;
  }
  MapIterator it = MapIteratorCreate(loadedChunks);
  ChunkKey key;
  Chunk* chunk;
  while (MapIteratorNext(&it, &key, &chunk))
  {
    if (chunk->model.meshCount > 0) { RemoveChunkModel(chunk); }
    ChunkPoolRelease(chunk);
  }
  MapFree(loadedChunks);
  loadedChunks = NULL;
  mtx_unlock(&loadedChunksMutex);
}

#endif // CHUNK_MAP_H
